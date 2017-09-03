 /* BoCA - BonkEnc Component Architecture
  * Copyright (C) 2007-2017 Robert Kausch <robert.kausch@freac.org>
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License as
  * published by the Free Software Foundation, either version 2 of
  * the License, or (at your option) any later version.
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth.h>
#include <smooth/dll.h>

#include "faac.h"
#include "config.h"

using namespace smooth::IO;

const String &BoCA::EncoderFAAC::GetComponentSpecs()
{
	static String	 componentSpecs;

	if (faacdll != NIL)
	{
		componentSpecs = "							\
											\
		  <?xml version=\"1.0\" encoding=\"UTF-8\"?>				\
		  <component>								\
		    <name>FAAC MP4/AAC Encoder %VERSION% (SuperFast)</name>		\
		    <version>1.0</version>						\
		    <id>superfaac-enc</id>						\
		    <type>encoder</type>						\
		    <replace>voaacenc-enc</replace>					\
											\
		";

		if (mp4v2dll != NIL)
		{
			componentSpecs.Append("						\
											\
			    <format>							\
			      <name>MPEG-4 AAC Files</name>				\
			      <extension>m4a</extension>				\
			      <extension>m4b</extension>				\
			      <extension>m4r</extension>				\
			      <extension>mp4</extension>				\
			      <tag id=\"mp4-tag\" mode=\"other\">MP4 Metadata</tag>	\
			    </format>							\
											\
			");
		}

		componentSpecs.Append("							\
											\
		    <format>								\
		      <name>Raw AAC Files</name>					\
		      <extension>aac</extension>					\
		      <tag id=\"id3v2-tag\" mode=\"prepend\">ID3v2</tag>		\
		    </format>								\
		  </component>								\
											\
		");

		unsigned long	 samplesSize;
		unsigned long	 bufferSize;

		faacEncHandle	 faac = ex_faacEncOpen(44100, 2, &samplesSize, &bufferSize);

		componentSpecs.Replace("%VERSION%", String("v").Append(ex_faacEncGetCurrentConfiguration(faac)->name));

		ex_faacEncClose(faac);
	}

	return componentSpecs;
}

Void smooth::AttachDLL(Void *instance)
{
	LoadFAACDLL();
	LoadMP4v2DLL();
}

Void smooth::DetachDLL()
{
	FreeFAACDLL();
	FreeMP4v2DLL();
}

BoCA::EncoderFAAC::EncoderFAAC()
{
	configLayer  = NIL;

	mp4File	     = NIL;

	mp4Track     = -1;
	sampleId     = 0;

	frameSize    = 0;

	blockSize    = 128;
	overlap	     = 8;

	totalSamples = 0;

	nextWorker   = 0;
}

BoCA::EncoderFAAC::~EncoderFAAC()
{
	if (configLayer != NIL) Object::DeleteObject(configLayer);
}

Bool BoCA::EncoderFAAC::Activate()
{
	const Format	&format = track.GetFormat();

	if (format.channels > 6)
	{
		errorString = "This encoder does not support more than 6 channels!";
		errorState  = True;

		return False;
	}

	if (GetSampleRateIndex(format.rate) == -1)
	{
		errorString = "Bad sampling rate! The selected sampling rate is not supported.";
		errorState  = True;

		return False;
	}

	const Config	*config = GetConfiguration();

	unsigned long	 samplesSize	= 0;
	unsigned long	 bufferSize	= 0;

	faacEncHandle		 handle	 = ex_faacEncOpen(format.rate, format.channels, &samplesSize, &bufferSize);
	faacEncConfigurationPtr	 fConfig = ex_faacEncGetCurrentConfiguration(handle);

	fConfig->mpegVersion	= config->GetIntValue("FAAC", "MP4Container", True) ? MPEG4 : config->GetIntValue("FAAC", "MPEGVersion", 0);
	fConfig->aacObjectType	= LOW;
	fConfig->allowMidside	= config->GetIntValue("FAAC", "AllowJS", True);
	fConfig->useTns		= config->GetIntValue("FAAC", "UseTNS", False);
	fConfig->bandWidth	= config->GetIntValue("FAAC", "BandWidth", 22050);

	if (config->GetIntValue("FAAC", "MP4Container", True)) fConfig->outputFormat = 0; // Raw AAC frame headers

	if (config->GetIntValue("FAAC", "SetQuality", True))   fConfig->quantqual    = config->GetIntValue("FAAC", "AACQuality", 100);
	else						       fConfig->bitRate	     = config->GetIntValue("FAAC", "Bitrate", 96) * 1000;

	if (format.bits ==  8) fConfig->inputFormat = FAAC_INPUT_16BIT;
	if (format.bits == 16) fConfig->inputFormat = FAAC_INPUT_16BIT;
	if (format.bits == 24) fConfig->inputFormat = FAAC_INPUT_32BIT;
	if (format.bits == 32) fConfig->inputFormat = FAAC_INPUT_32BIT;

	ex_faacEncSetConfiguration(handle, fConfig);

	if (config->GetIntValue("FAAC", "MP4Container", True))
	{
		mp4File	 = ex_MP4CreateEx(Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out"), 0, 1, 1, NIL, 0, NIL, 0);
		mp4Track = ex_MP4AddAudioTrack(mp4File, format.rate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);

		ex_MP4SetAudioProfileLevel(mp4File, 0x0F);

		unsigned char	*buffer = NIL;

		ex_faacEncGetDecoderSpecificInfo(handle, &buffer, &bufferSize);

		ex_MP4SetTrackESConfiguration(mp4File, mp4Track, (uint8_t *) buffer, bufferSize);

		totalSamples = 0;
	}

	frameSize = samplesSize / format.channels;

	ex_faacEncClose(handle);

	/* Write ID3v2 tag if requested.
	 */
	if (!config->GetIntValue("FAAC", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue("FAAC", "AllowID3v2", False))
	{
		const Info	&info = track.GetInfo();

		if (info.HasBasicInfo() || (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True)))
		{
			AS::Registry		&boca = AS::Registry::Get();
			AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("id3v2-tag");

			if (tagger != NIL)
			{
				Buffer<unsigned char>	 id3Buffer;

				tagger->SetConfiguration(GetConfiguration());
				tagger->RenderBuffer(id3Buffer, track);

				driver->WriteData(id3Buffer, id3Buffer.Size());

				boca.DeleteComponent(tagger);
			}
		}
	}

	/* Get number of threads to use.
	 */
	Bool	 enableParallel	 = config->GetIntValue("Resources", "EnableParallelConversions", True);
	Int	 numberOfThreads = enableParallel ? config->GetIntValue("Resources", "NumberOfConversionThreads", 0) : 1;

	if (enableParallel && numberOfThreads <= 1) numberOfThreads = CPU().GetNumCores() + (CPU().GetNumLogicalCPUs() - CPU().GetNumCores()) / 2;

	/* Disable overlap if we use only one thread.
	 */
	if (numberOfThreads == 1) overlap = 0;
	else			  overlap = 8;

	/* Start up worker threads.
	 */
	for (Int i = 0; i < numberOfThreads; i++) workers.Add(new SuperWorker(config, format));

	foreach (SuperWorker *worker, workers) worker->Start();

	return True;
}

Bool BoCA::EncoderFAAC::Deactivate()
{
	const Config	*config = GetConfiguration();

	/* Output remaining samples to encoder.
	 */
	EncodeFrames(True);

	/* Tear down worker threads.
	 */
	foreach (SuperWorker *worker, workers) worker->Quit();
	foreach (SuperWorker *worker, workers) worker->Wait();
	foreach (SuperWorker *worker, workers) delete worker;

	workers.RemoveAll();

	/* Finish MP4 writing.
	 */
	if (config->GetIntValue("FAAC", "MP4Container", True))
	{
		/* Write iTunes metadata with gapless information.
		 */
		MP4ItmfItem	*item  = ex_MP4ItmfItemAlloc("----", 1);
		String		 value = String().Append(" 00000000")
						 .Append(" ").Append(Number((Int64) frameSize).ToHexString(8).ToUpper())
						 .Append(" ").Append(Number((Int64) frameSize - totalSamples % frameSize).ToHexString(8).ToUpper())
						 .Append(" ").Append(Number((Int64) totalSamples).ToHexString(16).ToUpper())
						 .Append(" 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000");

		item->mean = (char *) "com.apple.iTunes";
		item->name = (char *) "iTunSMPB";

		item->dataList.elements[0].typeCode  = MP4_ITMF_BT_UTF8;
		item->dataList.elements[0].value     = (uint8_t *) value.ConvertTo("UTF-8");
		item->dataList.elements[0].valueSize = value.Length();

		ex_MP4ItmfAddItem(mp4File, item);

		item->mean = NIL;
		item->name = NIL;

		item->dataList.elements[0].typeCode  = MP4_ITMF_BT_IMPLICIT;
		item->dataList.elements[0].value     = NIL;
		item->dataList.elements[0].valueSize = 0;

		ex_MP4ItmfItemFree(item);

		ex_MP4Close(mp4File, 0);

		/* Write metadata to file
		 */
		if (config->GetIntValue("Tags", "EnableMP4Metadata", True))
		{
			const Info	&info = track.GetInfo();

			if (info.HasBasicInfo() || (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True)))
			{
				AS::Registry		&boca = AS::Registry::Get();
				AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("mp4-tag");

				if (tagger != NIL)
				{
					tagger->SetConfiguration(GetConfiguration());
					tagger->RenderStreamInfo(Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out"), track);

					boca.DeleteComponent(tagger);
				}
			}
		}
		else
		{
			/* Optimize file even when no tags are written.
			 */
			ex_MP4Optimize(Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out"), NIL);
		}

		/* Stream contents of created MP4 file to output driver
		 */
		InStream		 in(STREAM_FILE, Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out"), IS_READ);
		Buffer<UnsignedByte>	 buffer(1024);
		Int64			 bytesLeft = in.Size();

		while (bytesLeft)
		{
			in.InputData(buffer, Math::Min(Int64(1024), bytesLeft));

			driver->WriteData(buffer, Math::Min(Int64(1024), bytesLeft));

			bytesLeft -= Math::Min(Int64(1024), bytesLeft);
		}

		in.Close();

		File(Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out")).Delete();
	}

	/* Write ID3v1 tag if requested.
	 */
	if (!config->GetIntValue("FAAC", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v1", False))
	{
		const Info	&info = track.GetInfo();

		if (info.HasBasicInfo())
		{
			AS::Registry		&boca = AS::Registry::Get();
			AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("id3v1-tag");

			if (tagger != NIL)
			{
				Buffer<unsigned char>	 id3Buffer;

				tagger->SetConfiguration(GetConfiguration());
				tagger->RenderBuffer(id3Buffer, track);

				driver->WriteData(id3Buffer, id3Buffer.Size());

				boca.DeleteComponent(tagger);
			}
		}
	}

	/* Update ID3v2 tag with correct chapter marks.
	 */
	if (!config->GetIntValue("FAAC", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue("FAAC", "AllowID3v2", False))
	{
		if (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True))
		{
			AS::Registry		&boca = AS::Registry::Get();
			AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("id3v2-tag");

			if (tagger != NIL)
			{
				Buffer<unsigned char>	 id3Buffer;

				tagger->SetConfiguration(GetConfiguration());
				tagger->RenderBuffer(id3Buffer, track);

				driver->Seek(0);
				driver->WriteData(id3Buffer, id3Buffer.Size());

				boca.DeleteComponent(tagger);
			}
		}
	}

	return True;
}

Int BoCA::EncoderFAAC::WriteData(Buffer<UnsignedByte> &data)
{
	static Endianness	 endianness = CPU().GetEndianness();

	const Format	&format	= track.GetFormat();

	/* Change to AAC channel order.
	 */
	if	(format.channels == 3) Utilities::ChangeChannelOrder(data, format, Channel::Default_3_0, Channel::AAC_3_0);
	else if (format.channels == 5) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_0, Channel::AAC_5_0);
	else if (format.channels == 6) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_1, Channel::AAC_5_1);

	/* Convert samples to 16 or 24 bit.
	 */
	Int	 samples = data.Size() / format.channels / (format.bits / 8);
	Int	 offset	 = samplesBuffer.Size();

	if (format.bits <= 16) samplesBuffer.Resize(samplesBuffer.Size() + samples * format.channels / 2);
	else		       samplesBuffer.Resize(samplesBuffer.Size() + samples * format.channels);

	for (Int i = 0; i < samples * format.channels; i++)
	{
		if	(format.bits ==  8				) ((short *) (int32_t *) samplesBuffer)[2 * offset + i] = (				 data [i] - 128) * 256;
		else if (format.bits == 16				) ((short *) (int32_t *) samplesBuffer)[2 * offset + i] = ((short *)   (unsigned char *) data)[i];
		else if (format.bits == 32				)			 samplesBuffer [    offset + i] = ((int32_t *) (unsigned char *) data)[i]	 / 256;

		else if (format.bits == 24 && endianness == EndianLittle) samplesBuffer[offset + i] = (data[3 * i + 2] << 24 | data[3 * i + 1] << 16 | data[3 * i    ] << 8) / 256;
		else if (format.bits == 24 && endianness == EndianBig	) samplesBuffer[offset + i] = (data[3 * i    ] << 24 | data[3 * i + 1] << 16 | data[3 * i + 2] << 8) / 256;
	}

	/* Output samples to encoder.
	 */
	return EncodeFrames(False);
}

Int BoCA::EncoderFAAC::EncodeFrames(Bool flush)
{
	const Format	&format = track.GetFormat();

	/* Pad end of stream with empty samples.
	 */
	Int	 nullSamples = 0;

	if (flush && samplesBuffer.Size() > 0)
	{
		nullSamples = frameSize - (samplesBuffer.Size() / format.channels * (format.bits <= 16 ? 2 : 1)) % frameSize;

		samplesBuffer.Resize(samplesBuffer.Size() + nullSamples * format.channels / (format.bits <= 16 ? 2 : 1));

		memset(((int32_t *) samplesBuffer) + samplesBuffer.Size() - nullSamples * format.channels / (format.bits <= 16 ? 2 : 1), 0, sizeof(int32_t) * nullSamples * format.channels / (format.bits <= 16 ? 2 : 1));

		totalSamples += samplesBuffer.Size() / format.channels * (format.bits <= 16 ? 2 : 1) - nullSamples;
	}

	/* Pass samples to workers.
	 */
	Int	 framesToProcess = blockSize;
	Int	 framesProcessed = 0;
	Int	 dataLength	 = 0;

	Int	 samplesPerFrame = frameSize * format.channels / (format.bits <= 16 ? 2 : 1);

	if (flush) framesToProcess = Math::Floor(samplesBuffer.Size() / samplesPerFrame);

	while (samplesBuffer.Size() - framesProcessed * samplesPerFrame >= samplesPerFrame * framesToProcess)
	{
		SuperWorker	*workerToUse = workers.GetNth(nextWorker % workers.Length());

		while (!workerToUse->IsReady()) S::System::System::Sleep(1);

		workerToUse->Lock();

		/* See if the worker has some packets for us.
		 */
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), nextWorker == workers.Length());

		/* Pass new frames to worker.
		 */
		workerToUse->Encode(samplesBuffer, framesProcessed * samplesPerFrame, samplesPerFrame * framesToProcess, flush);
		workerToUse->Release();

		if (!flush) totalSamples += frameSize * (framesToProcess - overlap);

		framesProcessed += framesToProcess - overlap;

		nextWorker++;

		if (flush) break;
	}

	memmove((int32_t *) samplesBuffer, ((int32_t *) samplesBuffer) + framesProcessed * samplesPerFrame, sizeof(int32_t) * (samplesBuffer.Size() - framesProcessed * samplesPerFrame));

	samplesBuffer.Resize(samplesBuffer.Size() - framesProcessed * samplesPerFrame);

	if (!flush) return dataLength;

	/* Wait for workers to finish and process packets.
	 */
	for (Int i = 0; i < workers.Length(); i++)
	{
		SuperWorker	*workerToUse = workers.GetNth(nextWorker % workers.Length());

		while (!workerToUse->IsReady()) S::System::System::Sleep(1);

		workerToUse->Lock();

		/* See if the worker has some packets for us.
		 */
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), nextWorker == workers.Length());

		workerToUse->Release();

		nextWorker++;
	}

	return dataLength;
}

Int BoCA::EncoderFAAC::ProcessPackets(const Buffer<unsigned char> &packets, const Array<Int> &packetSizes, Bool first)
{
	Int	 offset	    = 0;
	Int	 dataLength = 0;

	if (!first) for (Int i = 0; i < overlap; i++) offset += packetSizes.GetNth(i);

	for (Int i = 0; i < packetSizes.Length(); i++)
	{
		if (i <	overlap && !first)	continue;
		if (packetSizes.GetNth(i) == 0) continue;

		if (mp4File != NIL) ex_MP4WriteSample(mp4File, mp4Track, (uint8_t *) (unsigned char *) packets + offset, packetSizes.GetNth(i), frameSize, 0, true);
		else		    driver->WriteData(packets + offset, packetSizes.GetNth(i));

		offset	   += packetSizes.GetNth(i);
		dataLength += packetSizes.GetNth(i);
	}

	return dataLength;
}

Int BoCA::EncoderFAAC::GetSampleRateIndex(Int sampleRate) const
{
	Int	 sampleRates[12] = { 96000, 88200, 64000, 48000, 44100, 32000,
				     24000, 22050, 16000, 12000, 11025,  8000 };

	for (Int i = 0; i < 12; i++)
	{
		if (sampleRate == sampleRates[i]) return i;
	}

	return -1;
}

Bool BoCA::EncoderFAAC::SetOutputFormat(Int n)
{
	Config	*config = Config::Get();

	if (n == 0 && mp4v2dll != NIL)
	{
		config->SetIntValue("FAAC", "MP4Container", True);
		config->SetIntValue("FAAC", "MPEGVersion", 0);
	}
	else
	{
		config->SetIntValue("FAAC", "MP4Container", False);
	}

	return True;
}

String BoCA::EncoderFAAC::GetOutputFileExtension() const
{
	const Config	*config = GetConfiguration();

	if (config->GetIntValue("FAAC", "MP4Container", True))
	{
		switch (config->GetIntValue("FAAC", "MP4FileExtension", 0))
		{
			default:
			case  0: return "m4a";
			case  1: return "m4b";
			case  2: return "m4r";
			case  3: return "mp4";
		}
	}

	return "aac";
}

ConfigLayer *BoCA::EncoderFAAC::GetConfigurationLayer()
{
	if (configLayer == NIL) configLayer = new ConfigureFAAC();

	return configLayer;
}
