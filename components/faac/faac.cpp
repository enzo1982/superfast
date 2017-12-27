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
		componentSpecs = "										\
														\
		  <?xml version=\"1.0\" encoding=\"UTF-8\"?>							\
		  <component>											\
		    <name>FAAC MP4/AAC Encoder %VERSION% (SuperFast)</name>					\
		    <version>1.0</version>									\
		    <id>superfaac-enc</id>									\
		    <type>encoder</type>									\
		    <replace>voaacenc-enc</replace>								\
														\
		";

		if (mp4v2dll != NIL)
		{
			componentSpecs.Append("									\
														\
			    <format>										\
			      <name>MPEG-4 AAC Files</name>							\
			      <extension>m4a</extension>							\
			      <extension>m4b</extension>							\
			      <extension>m4r</extension>							\
			      <extension>mp4</extension>							\
			      <tag id=\"mp4-tag\" mode=\"other\">MP4 Metadata</tag>				\
			    </format>										\
														\
			");
		}

		componentSpecs.Append("										\
														\
		    <format>											\
		      <name>Raw AAC Files</name>								\
		      <extension>aac</extension>								\
		      <tag id=\"id3v2-tag\" mode=\"prepend\">ID3v2</tag>					\
		    </format>											\
		    <input bits=\"16\" channels=\"1-6\"								\
			   rate=\"8000,11025,12000,16000,22050,24000,32000,44100,48000,64000,88200,96000\"/>	\
		  </component>											\
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
	const Config	*config = GetConfiguration();

	const Format	&format = track.GetFormat();

	/* Get configuration.
	 */
	Bool	 mp4Container = config->GetIntValue(ConfigureFAAC::ConfigID, "MP4Container", True);
	Int	 mpegVersion  = config->GetIntValue(ConfigureFAAC::ConfigID, "MPEGVersion", 0);

	/* Create FAAC encoder.
	 */
	unsigned long	 samplesSize = 0;
	unsigned long	 bufferSize  = 0;

	faacEncHandle	 handle = ex_faacEncOpen(format.rate, format.channels, &samplesSize, &bufferSize);

	/* Set encoder parameters.
	 */
	faacEncConfigurationPtr	 fConfig = ex_faacEncGetCurrentConfiguration(handle);

	fConfig->mpegVersion	= mp4Container ? MPEG4 : mpegVersion;
	fConfig->aacObjectType	= LOW;

	ex_faacEncSetConfiguration(handle, fConfig);

	frameSize = samplesSize / format.channels;

	/* Create MP4 container.
	 */
	if (mp4Container)
	{
		mp4File	 = ex_MP4CreateEx(String(track.outfile).Append(".out").ConvertTo("UTF-8"), 0, 1, 1, NIL, 0, NIL, 0);
		mp4Track = ex_MP4AddAudioTrack(mp4File, format.rate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);

		ex_MP4SetAudioProfileLevel(mp4File, 0x0F);

		unsigned char	*buffer = NIL;

		ex_faacEncGetDecoderSpecificInfo(handle, &buffer, &bufferSize);

		ex_MP4SetTrackESConfiguration(mp4File, mp4Track, (uint8_t *) buffer, bufferSize);

		totalSamples = 0;
	}

	ex_faacEncClose(handle);

	/* Write ID3v2 tag if requested.
	 */
	if (mp4File == NIL && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue(ConfigureFAAC::ConfigID, "AllowID3v2", False))
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
	if (mp4File != NIL)
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
					tagger->RenderStreamInfo(String(track.outfile).Append(".out"), track);

					boca.DeleteComponent(tagger);
				}
			}
		}
		else
		{
			/* Optimize file even when no tags are written.
			 */
			ex_MP4Optimize(String(track.outfile).Append(".out").ConvertTo("UTF-8"), NIL);
		}

		/* Stream contents of created MP4 file to output driver
		 */
		InStream		 in(STREAM_FILE, String(track.outfile).Append(".out"), IS_READ);
		Buffer<UnsignedByte>	 buffer(1024);
		Int64			 bytesLeft = in.Size();

		while (bytesLeft)
		{
			in.InputData(buffer, Math::Min(Int64(1024), bytesLeft));

			driver->WriteData(buffer, Math::Min(Int64(1024), bytesLeft));

			bytesLeft -= Math::Min(Int64(1024), bytesLeft);
		}

		in.Close();

		File(String(track.outfile).Append(".out")).Delete();
	}

	/* Write ID3v1 tag if requested.
	 */
	if (mp4File == NIL && config->GetIntValue("Tags", "EnableID3v1", False))
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
	if (mp4File == NIL && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue(ConfigureFAAC::ConfigID, "AllowID3v2", False))
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
	const Format	&format	= track.GetFormat();

	/* Change to AAC channel order.
	 */
	if	(format.channels == 3) Utilities::ChangeChannelOrder(data, format, Channel::Default_3_0, Channel::AAC_3_0);
	else if (format.channels == 5) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_0, Channel::AAC_5_0);
	else if (format.channels == 6) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_1, Channel::AAC_5_1);

	/* Copy data to samples buffer.
	 */
	Int	 samples = data.Size() / 2;

	samplesBuffer.Resize(samplesBuffer.Size() + samples);

	memcpy(samplesBuffer + samplesBuffer.Size() - samples, data, data.Size());

	/* Output samples to encoder.
	 */
	return EncodeFrames(False);
}

Int BoCA::EncoderFAAC::EncodeFrames(Bool flush)
{
	const Format	&format = track.GetFormat();

	/* Pad end of stream with empty samples.
	 */
	if (flush && samplesBuffer.Size() > 0)
	{
		Int	 nullSamples = frameSize - (samplesBuffer.Size() / format.channels) % frameSize;

		samplesBuffer.Resize(samplesBuffer.Size() + nullSamples * format.channels);

		memset(samplesBuffer + samplesBuffer.Size() - nullSamples * format.channels, 0, sizeof(int16_t) * nullSamples * format.channels);

		totalSamples += samplesBuffer.Size() / format.channels - nullSamples;
	}

	/* Pass samples to workers.
	 */
	Int	 framesToProcess = blockSize;
	Int	 framesProcessed = 0;
	Int	 dataLength	 = 0;

	Int	 samplesPerFrame = frameSize * format.channels;

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

	memmove(samplesBuffer, samplesBuffer + framesProcessed * samplesPerFrame, sizeof(int16_t) * (samplesBuffer.Size() - framesProcessed * samplesPerFrame));

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
		config->SetIntValue(ConfigureFAAC::ConfigID, "MP4Container", True);
		config->SetIntValue(ConfigureFAAC::ConfigID, "MPEGVersion", 0);
	}
	else
	{
		config->SetIntValue(ConfigureFAAC::ConfigID, "MP4Container", False);
	}

	return True;
}

String BoCA::EncoderFAAC::GetOutputFileExtension() const
{
	const Config	*config = GetConfiguration();

	if (config->GetIntValue(ConfigureFAAC::ConfigID, "MP4Container", True))
	{
		switch (config->GetIntValue(ConfigureFAAC::ConfigID, "MP4FileExtension", 0))
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
