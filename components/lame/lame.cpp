 /* BoCA - BonkEnc Component Architecture
  * Copyright (C) 2007-2018 Robert Kausch <robert.kausch@freac.org>
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

#include "lame.h"
#include "config.h"

const String &BoCA::EncoderLAME::GetComponentSpecs()
{
	static String	 componentSpecs;

	if (lamedll != NIL)
	{
		componentSpecs = "								\
												\
		  <?xml version=\"1.0\" encoding=\"UTF-8\"?>					\
		  <component>									\
		    <name>LAME MP3 Encoder %VERSION% (SuperFast)</name>				\
		    <version>1.0</version>							\
		    <id>superlame-enc</id>							\
		    <type>encoder</type>							\
		    <format>									\
		      <name>MPEG 1 Audio Layer 3</name>						\
		      <extension>mp3</extension>						\
		      <tag id=\"id3v1-tag\" mode=\"append\">ID3v1</tag>				\
		      <tag id=\"id3v2-tag\" mode=\"prepend\">ID3v2</tag>			\
		    </format>									\
		    <input bits=\"16\" channels=\"1-2\"						\
			   rate=\"8000,11025,12000,16000,22050,24000,32000,44100,48000\"/>	\
		  </component>									\
												\
		";

		componentSpecs.Replace("%VERSION%", String("v").Append(ex_get_lame_short_version()));
	}

	return componentSpecs;
}

Void smooth::AttachDLL(Void *instance)
{
	LoadLAMEDLL();
}

Void smooth::DetachDLL()
{
	FreeLAMEDLL();
}

BoCA::EncoderLAME::EncoderLAME()
{
	configLayer  = NIL;

	dataOffset   = 0;
	frameSize    = 0;

	blockSize    = 128;
	overlap	     = 4;

	nextWorker   = 0;

	totalSamples = 0;

	repacker     = NIL;
}

BoCA::EncoderLAME::~EncoderLAME()
{
	if (configLayer != NIL) Object::DeleteObject(configLayer);
}

Bool BoCA::EncoderLAME::Activate()
{
	const Config	*config = GetConfiguration();

	const Format	&format = track.GetFormat();
	const Info	&info	= track.GetInfo();

	/* Get configuration.
	 */
	Int	 preset		  = config->GetIntValue(ConfigureLAME::ConfigID, "Preset", 2);
	Int	 vbrMode	  = config->GetIntValue(ConfigureLAME::ConfigID, "VBRMode", 4);
	Bool	 setBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "SetBitrate", 1);
	Int	 bitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "Bitrate", 192);
	Int	 ratio		  = config->GetIntValue(ConfigureLAME::ConfigID, "Ratio", 1100);
	Int	 abrBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "ABRBitrate", 192);
	Int	 vbrQuality	  = config->GetIntValue(ConfigureLAME::ConfigID, "VBRQuality", 50);
	Bool	 setMinVBRBitrate = config->GetIntValue(ConfigureLAME::ConfigID, "SetMinVBRBitrate", 0);
	Bool	 setMaxVBRBitrate = config->GetIntValue(ConfigureLAME::ConfigID, "SetMaxVBRBitrate", 0);
	Int	 minVBRBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "MinVBRBitrate", 128);
	Int	 maxVBRBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "MaxVBRBitrate", 256);
	Bool	 strictISO	  = config->GetIntValue(ConfigureLAME::ConfigID, "StrictISO", 0);
	Int	 stereoMode	  = config->GetIntValue(ConfigureLAME::ConfigID, "StereoMode", 0);

	/* Create and configure LAME encoder.
	 */
	lame_t	 context = ex_lame_init();

	ex_lame_set_in_samplerate(context, format.rate);
	ex_lame_set_num_channels(context, format.channels);

	switch (preset)
	{
		case 0:
			ex_lame_set_strict_ISO(context, strictISO);

			/* Set bitrate.
			 */
			if (vbrMode == vbr_off)
			{
				if (setBitrate) ex_lame_set_brate(context, bitrate);
				else		ex_lame_set_compression_ratio(context, ratio / 100.0);
			}

			/* Set Stereo mode.
			 */
			if	(stereoMode == 1) ex_lame_set_mode(context, MONO);
			else if (stereoMode == 2) ex_lame_set_mode(context, STEREO);
			else if (stereoMode == 3) ex_lame_set_mode(context, JOINT_STEREO);
			else			  ex_lame_set_mode(context, NOT_SET);

			/* Set VBR mode.
			 */
			switch (vbrMode)
			{
				default:
				case vbr_off:
					break;
				case vbr_abr:
					ex_lame_set_VBR(context, vbr_abr);
					ex_lame_set_VBR_mean_bitrate_kbps(context, abrBitrate);
					break;
				case vbr_rh:
					ex_lame_set_VBR(context, vbr_rh);
					ex_lame_set_VBR_quality(context, vbrQuality / 10.0);
					break;
				case vbr_mtrh:
					ex_lame_set_VBR(context, vbr_mtrh);
					ex_lame_set_VBR_quality(context, vbrQuality / 10.0);
					break;
			}

			if (vbrMode != vbr_off && setMinVBRBitrate) ex_lame_set_VBR_min_bitrate_kbps(context, minVBRBitrate);
			if (vbrMode != vbr_off && setMaxVBRBitrate) ex_lame_set_VBR_max_bitrate_kbps(context, maxVBRBitrate);

			break;
		case 1:
			ex_lame_set_preset(context, MEDIUM_FAST);
			break;
		case 2:
			ex_lame_set_preset(context, STANDARD_FAST);
			break;
		case 3:
			ex_lame_set_preset(context, EXTREME_FAST);
			break;
		case 4:
			ex_lame_set_preset(context, abrBitrate);
			break;
	}

	if (ex_lame_init_params(context) < 0)
	{
		errorString = "Bad LAME encoder settings!\n\nPlease check your encoder settings in the\nconfiguration dialog.";
		errorState  = True;

		return False;
	}

	frameSize  = ex_lame_get_framesize(context);

	ex_lame_close(context);

	dataOffset = 0;

	/* Write ID3v2 tag if requested.
	 */
	if (config->GetIntValue("Tags", "EnableID3v2", True) && (info.HasBasicInfo() || (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True))))
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

			dataOffset = id3Buffer.Size();
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
	else			  overlap = 4;

	/* Start up worker threads.
	 */
	for (Int i = 0; i < numberOfThreads; i++) workers.Add(new SuperWorker(config, format));

	foreach (SuperWorker *worker, workers) worker->Start();

	/* Create repacker instance.
	 */
	repacker = new SuperRepacker(driver);

	return True;
}

Bool BoCA::EncoderLAME::Deactivate()
{
	const Config	*config = GetConfiguration();
	const Info	&info = track.GetInfo();

	/* Output remaining samples to encoder.
	 */
	EncodeFrames(True);

	/* Write Xing or Info header.
	 */
	Buffer<UnsignedByte>	 buffer;

	workers.GetFirst()->GetInfoTag(buffer);

	repacker->UpdateInfoTag(buffer, totalSamples);

	driver->Seek(dataOffset);
	driver->WriteData(buffer, buffer.Size());

	/* Delete repacker instance.
	 */
	delete repacker;

	/* Tear down worker threads.
	 */
	foreach (SuperWorker *worker, workers) worker->Quit();
	foreach (SuperWorker *worker, workers) worker->Wait();
	foreach (SuperWorker *worker, workers) delete worker;

	workers.RemoveAll();

	/* Write ID3v1 tag if requested.
	 */
	if (config->GetIntValue("Tags", "EnableID3v1", False) && info.HasBasicInfo())
	{
		AS::Registry		&boca = AS::Registry::Get();
		AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("id3v1-tag");

		if (tagger != NIL)
		{
			Buffer<unsigned char>	 id3Buffer;

			tagger->SetConfiguration(GetConfiguration());
			tagger->RenderBuffer(id3Buffer, track);

			driver->Seek(driver->GetSize());
			driver->WriteData(id3Buffer, id3Buffer.Size());

			boca.DeleteComponent(tagger);
		}
	}

	/* Update ID3v2 tag with correct chapter marks.
	 */
	if (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True) && config->GetIntValue("Tags", "EnableID3v2", True))
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

	return True;
}

Int BoCA::EncoderLAME::WriteData(Buffer<UnsignedByte> &data)
{
	const Format	&format = track.GetFormat();

	/* Copy data to samples buffer.
	 */
	Int	 samples = data.Size() / 2;

	samplesBuffer.Resize(samplesBuffer.Size() + samples);

	memcpy(samplesBuffer + samplesBuffer.Size() - samples, data, data.Size());

	/* Output samples to encoder.
	 */
	totalSamples += data.Size() / format.channels / (format.bits / 8);

	return EncodeFrames(False);
}

Int BoCA::EncoderLAME::EncodeFrames(Bool flush)
{
	const Format	&format = track.GetFormat();

	/* Pad end of stream with empty samples.
	 */
	if (flush && samplesBuffer.Size() > 0)
	{
		Int	 nullSamples = frameSize - (samplesBuffer.Size() / format.channels) % frameSize;

		samplesBuffer.Resize(samplesBuffer.Size() + nullSamples * format.channels);

		memset(((signed short *) samplesBuffer) + samplesBuffer.Size() - nullSamples * format.channels, 0, sizeof(signed short) * nullSamples * format.channels);
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

		framesProcessed += framesToProcess - overlap;

		nextWorker++;

		if (flush) break;
	}

	memmove((signed short *) samplesBuffer, ((signed short *) samplesBuffer) + framesProcessed * samplesPerFrame, sizeof(signed short) * (samplesBuffer.Size() - framesProcessed * samplesPerFrame));

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

	/* Flush repacker.
	 */
	repacker->Flush();

	return dataLength;
}

Int BoCA::EncoderLAME::ProcessPackets(const Buffer<unsigned char> &data, const Array<Int> &chunkSizes, Bool first)
{
	Buffer<UnsignedByte>	 packets;
	Array<Int>		 packetSizes;

	repacker->UnpackFrames(data, packets, packetSizes);

	Int	 offset	    = 0;
	Int	 dataLength = 0;

	if (!first) for (Int i = 0; i < overlap; i++) offset += packetSizes.GetNth(i);

	for (Int i = 0; i < packetSizes.Length(); i++)
	{
		if (i <	overlap && !first)	continue;
		if (packetSizes.GetNth(i) == 0) continue;

		repacker->WriteFrame(packets + offset, packetSizes.GetNth(i));

		offset	   += packetSizes.GetNth(i);
		dataLength += packetSizes.GetNth(i);
	}

	return dataLength;
}

ConfigLayer *BoCA::EncoderLAME::GetConfigurationLayer()
{
	if (configLayer == NIL) configLayer = new ConfigureLAME();

	return configLayer;
}
