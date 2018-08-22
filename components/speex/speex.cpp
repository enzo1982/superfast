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

#include <time.h>
#include <stdlib.h>

#include "speex.h"
#include "config.h"

const String &BoCA::EncoderSpeex::GetComponentSpecs()
{
	static String	 componentSpecs;

	if (oggdll != NIL && speexdll != NIL)
	{
		componentSpecs = "									\
													\
		  <?xml version=\"1.0\" encoding=\"UTF-8\"?>						\
		  <component>										\
		    <name>Speex Speech Encoder %VERSION% (SuperFast)</name>				\
		    <version>1.0</version>								\
		    <id>superspeex-enc</id>								\
		    <type>encoder</type>								\
		    <format>										\
		      <name>Speex Files</name>								\
		      <extension>spx</extension>							\
		      <tag id=\"vorbis-tag\" mode=\"other\">Vorbis Comment</tag>			\
		    </format>										\
		    <input bits=\"16\" channels=\"1-2\" rate=\"8000,16000,32000\"/>			\
		    <parameters>									\
		      <range name=\"Bitrate\" argument=\"--bitrate %VALUE\" default=\"32\">		\
			<min>4</min>									\
			<max>64</max>									\
		      </range>										\
		      <range name=\"Quality\" argument=\"--quality %VALUE\" default=\"8\">		\
			<min alias=\"worst\">0</min>							\
			<max alias=\"best\">10</max>							\
		      </range>										\
		      <range name=\"Encoding complexity\" argument=\"--comp %VALUE\" default=\"3\">	\
			<min alias=\"fastest\">0</min>							\
			<max alias=\"slowest\">10</max>							\
		      </range>										\
 		      <switch name=\"Use VBR encoding\" argument=\"--vbr\"/>				\
 		      <range name=\"Use ABR encoding\" argument=\"--abr %VALUE\" default=\"32\">	\
			<min>4</min>									\
			<max>64</max>									\
		      </range>										\
		    </parameters>									\
		  </component>										\
													\
		";

		const char	*speexVersion = NIL;

		ex_speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, &speexVersion);

		componentSpecs.Replace("%VERSION%", String("v").Append(speexVersion));
	}

	return componentSpecs;
}

Void smooth::AttachDLL(Void *instance)
{
	LoadOggDLL();
	LoadSpeexDLL();
}

Void smooth::DetachDLL()
{
	FreeOggDLL();
	FreeSpeexDLL();
}

BoCA::EncoderSpeex::EncoderSpeex()
{
	configLayer  = NIL;

	frameSize    = 0;
	lookAhead    = 0;

	blockSize    = 256;
	overlap	     = 16;

	numPackets   = 0;
	totalSamples = 0;

	nextWorker   = 0;

	config	     = Config::Copy(GetConfiguration());

	ConvertArguments(config);

	memset(&os, 0, sizeof(os));
	memset(&og, 0, sizeof(og));
	memset(&op, 0, sizeof(op));
}

BoCA::EncoderSpeex::~EncoderSpeex()
{
	Config::Free(config);

	if (configLayer != NIL) Object::DeleteObject(configLayer);
}

Bool BoCA::EncoderSpeex::Activate()
{
	const Format	&format = track.GetFormat();
	const Info	&info	= track.GetInfo();

	/* Init Ogg stream.
	 */
	srand(clock());

	ex_ogg_stream_init(&os, rand());

	/* Get Speex mode ID.
	 */
	Int	 modeID = config->GetIntValue(ConfigureSpeex::ConfigID, "Mode", -1);

	if (modeID == -1)
	{
		/* Automatically select Speex mode
		 * depending on sampling rate.
		 */
		if	(format.rate <= 12500) modeID = SPEEX_MODEID_NB;
		else if	(format.rate <= 25000) modeID = SPEEX_MODEID_WB;
		else			       modeID = SPEEX_MODEID_UWB;
	}

	/* Get frame size and look-ahead.
	 */
	void	*encoder = ex_speex_encoder_init(ex_speex_lib_get_mode(modeID));

	ex_speex_encoder_ctl(encoder, SPEEX_GET_FRAME_SIZE, &frameSize);
	ex_speex_encoder_ctl(encoder, SPEEX_GET_LOOKAHEAD, &lookAhead);

	ex_speex_encoder_destroy(encoder);

	totalSamples = 0;
	numPackets   = 0;

	/* Create Speex header.
	 */
	SpeexHeader	 speex_header;

	ex_speex_init_header(&speex_header, format.rate, format.channels, ex_speex_lib_get_mode(modeID));

	speex_header.frames_per_packet = 1;
	speex_header.vbr	       = config->GetIntValue(ConfigureSpeex::ConfigID, "VBR", 0);

	/* Write Speex header.
	 */
	int		 bytes;
	unsigned char	*buffer = (unsigned char *) ex_speex_header_to_packet(&speex_header, &bytes);
	ogg_packet	 header = { buffer, bytes, 1, 0, 0, numPackets++ };

	ex_ogg_stream_packetin(&os, &header);

	ex_speex_header_free(buffer);

	/* Write Vorbis comment header.
	 */
	{
		char	*speexVersion = NIL;

		ex_speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, &speexVersion);

		Buffer<unsigned char>	 vcBuffer;

		/* Render actual Vorbis comment tag.
		 *
		 * An empty tag containing only the vendor string
		 * is rendered if Vorbis comments are disabled.
		 */
		AS::Registry		&boca = AS::Registry::Get();
		AS::TaggerComponent	*tagger = (AS::TaggerComponent *) boca.CreateComponentByID("vorbis-tag");

		if (tagger != NIL)
		{
			tagger->SetConfiguration(config);
			tagger->SetVendorString(String("Encoded with Speex ").Append(speexVersion));

			if (config->GetIntValue("Tags", "EnableVorbisComment", True) && (info.HasBasicInfo() || (track.tracks.Length() > 0 && config->GetIntValue("Tags", "WriteChapters", True)))) tagger->RenderBuffer(vcBuffer, track);
			else																					    tagger->RenderBuffer(vcBuffer, Track());

			boca.DeleteComponent(tagger);
		}

		ogg_packet	 header_comm = { vcBuffer, vcBuffer.Size(), 0, 0, 0, numPackets++ };

		ex_ogg_stream_packetin(&os, &header_comm);
	}

	WriteOggPackets(True);

	/* Get number of threads to use.
	 */
	Bool	 enableParallel	 = config->GetIntValue("Resources", "EnableParallelConversions", True);
	Bool	 enableSuperFast = config->GetIntValue("Resources", "EnableSuperFastMode", False);
	Int	 numberOfThreads = enableParallel && enableSuperFast ? config->GetIntValue("Resources", "NumberOfConversionThreads", 0) : 1;

	if (enableParallel && enableSuperFast && numberOfThreads <= 1) numberOfThreads = CPU().GetNumCores() + (CPU().GetNumLogicalCPUs() - CPU().GetNumCores()) / 2;

	/* Disable overlap if we use only one thread.
	 */
	if (numberOfThreads == 1) overlap = 0;
	else			  overlap = 16;

	/* Start up worker threads.
	 */
	for (Int i = 0; i < numberOfThreads; i++) workers.Add(new SuperWorker(config, format));

	foreach (SuperWorker *worker, workers) worker->Start();

	return True;
}

Bool BoCA::EncoderSpeex::Deactivate()
{
	/* Output remaining samples to encoder.
	 */
	EncodeFrames(True);

	/* Write any remaining Ogg packets.
	 */
	WriteOggPackets(True);

	ex_ogg_stream_clear(&os);

	/* Tear down worker threads.
	 */
	foreach (SuperWorker *worker, workers) worker->Quit();
	foreach (SuperWorker *worker, workers) worker->Wait();
	foreach (SuperWorker *worker, workers) delete worker;

	workers.RemoveAll();

	/* Fix chapter marks in Vorbis Comments.
	 */
	FixChapterMarks();

	return True;
}

Int BoCA::EncoderSpeex::WriteData(Buffer<UnsignedByte> &data)
{
	/* Copy data to samples buffer.
	 */
	Int	 samples = data.Size() / 2;

	samplesBuffer.Resize(samplesBuffer.Size() + samples);

	memcpy(samplesBuffer + samplesBuffer.Size() - samples, data, data.Size());

	/* Output samples to encoder.
	 */
	return EncodeFrames(False);
}

Int BoCA::EncoderSpeex::EncodeFrames(Bool flush)
{
	const Format	&format = track.GetFormat();

	/* Pad end of stream with empty samples.
	 */
	Int	 nullSamples = 0;

	if (flush)
	{
		nullSamples = frameSize;

		if ((samplesBuffer.Size() / format.channels) % frameSize > 0) nullSamples += frameSize - (samplesBuffer.Size() / format.channels) % frameSize;

		samplesBuffer.Resize(samplesBuffer.Size() + nullSamples * format.channels);

		memset(((signed short *) samplesBuffer) + samplesBuffer.Size() - nullSamples * format.channels, 0, sizeof(short) * nullSamples * format.channels);
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
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), nextWorker == workers.Length(), False, 0);

		/* Pass new frames to worker.
		 */
		workerToUse->Encode(samplesBuffer, framesProcessed * samplesPerFrame, samplesPerFrame * framesToProcess);
		workerToUse->Release();

		framesProcessed += framesToProcess - overlap;

		nextWorker++;

		if (flush) break;
	}

	memmove((signed short *) samplesBuffer, ((signed short *) samplesBuffer) + framesProcessed * samplesPerFrame, sizeof(short) * (samplesBuffer.Size() - framesProcessed * samplesPerFrame));

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
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), nextWorker == workers.Length(), i == workers.Length() - 1, i == workers.Length() - 1 ? nullSamples : 0);

		workerToUse->Release();

		nextWorker++;
	}

	return dataLength;
}

Int BoCA::EncoderSpeex::ProcessPackets(const Buffer<unsigned char> &packets, const Array<Int> &packetSizes, Bool first, Bool flush, Int nullSamples)
{
	Int	 offset = 0;

	if (!first) for (Int i = 0; i < overlap; i++) offset += packetSizes.GetNth(i);

	for (Int i = 0; i < packetSizes.Length(); i++)
	{
		if (i <	overlap && !first) continue;

		totalSamples += frameSize;

		op.packet     = packets + offset;
		op.bytes      = packetSizes.GetNth(i);
		op.b_o_s      = first && i == 0;
		op.e_o_s      = flush && i == packetSizes.Length() - 1;
		op.granulepos =  (flush && i == packetSizes.Length() - 1) ? totalSamples	     - nullSamples :
				((flush && i == packetSizes.Length() - 2) ? totalSamples + frameSize - nullSamples - lookAhead : totalSamples - lookAhead);
		op.packetno   = numPackets++;

		ex_ogg_stream_packetin(&os, &op);	

		offset += packetSizes.GetNth(i);
	}

	return WriteOggPackets(flush);
}

Int BoCA::EncoderSpeex::WriteOggPackets(Bool flush)
{
	Int	 bytes = 0;

	do
	{
		int	 result = 0;

		if (flush) result = ex_ogg_stream_flush(&os, &og);
		else	   result = ex_ogg_stream_pageout(&os, &og);

		if (result == 0) break;

		bytes += driver->WriteData(og.header, og.header_len);
		bytes += driver->WriteData(og.body, og.body_len);
	}
	while (true);

	return bytes;
}

Bool BoCA::EncoderSpeex::FixChapterMarks()
{
	if (track.tracks.Length() == 0 || !config->GetIntValue("Tags", "WriteChapters", True)) return True;

	driver->Seek(0);

	/* Skip first Ogg page and read second into buffer.
	 */
	Buffer<UnsignedByte>	 buffer;
	Int			 position;
	ogg_page		 og;

	for (Int i = 0; i < 2; i++)
	{
		driver->Seek(driver->GetPos() + 26);

		Int		 dataSize    = 0;
		UnsignedByte	 segments    = 0;
		UnsignedByte	 segmentSize = 0;

		driver->ReadData(&segments, 1);

		for (Int i = 0; i < segments; i++) { driver->ReadData(&segmentSize, 1); dataSize += segmentSize; }

		buffer.Resize(27 + segments + dataSize);
		position = driver->GetPos() - segments - 27;

		driver->Seek(position);
		driver->ReadData(buffer, buffer.Size());

		og.header     = buffer;
		og.header_len = 27 + segments;
		og.body	      = buffer + og.header_len;
		og.body_len   = dataSize;
	}

	/* Update chapter marks.
	 */
	if (buffer.Size() > 0)
	{
		Int64	 offset = 0;

		for (Int i = 0; i < track.tracks.Length(); i++)
		{
			const Track	&chapterTrack  = track.tracks.GetNth(i);
			const Format	&chapterFormat = chapterTrack.GetFormat();

			for (Int b = 0; b < buffer.Size() - 23; b++)
			{
				if (buffer[b + 0] != 'C' || buffer[b + 1] != 'H' || buffer[b + 2] != 'A' || buffer[b +  3] != 'P' ||
				    buffer[b + 4] != 'T' || buffer[b + 5] != 'E' || buffer[b + 6] != 'R' || buffer[b + 10] != '=') continue;

				String	 id;

				id[0] = buffer[b + 7];
				id[1] = buffer[b + 8];
				id[2] = buffer[b + 9];

				if (id.ToInt() != i + 1) continue;

				String	 value	= String(offset / chapterFormat.rate / 60 / 60 < 10 ? "0" : "").Append(String::FromInt(offset / chapterFormat.rate / 60 / 60)).Append(":")
						 .Append(offset / chapterFormat.rate / 60 % 60 < 10 ? "0" : "").Append(String::FromInt(offset / chapterFormat.rate / 60 % 60)).Append(":")
						 .Append(offset / chapterFormat.rate % 60      < 10 ? "0" : "").Append(String::FromInt(offset / chapterFormat.rate % 60)).Append(".")
						 .Append(Math::Round(offset % chapterFormat.rate * 1000.0 / chapterFormat.rate) < 100 ?
							(Math::Round(offset % chapterFormat.rate * 1000.0 / chapterFormat.rate) <  10 ?  "00" : "0") : "").Append(String::FromInt(Math::Round(offset % chapterFormat.rate * 1000.0 / chapterFormat.rate)));

				for (Int p = 0; p < 12; p++) buffer[b + 11 + p] = value[p];

				break;
			}

			if	(chapterTrack.length	   >= 0) offset += chapterTrack.length;
			else if (chapterTrack.approxLength >= 0) offset += chapterTrack.approxLength;
		}

		/* Write page back to file.
		 */
		ex_ogg_page_checksum_set(&og);

		driver->Seek(position);
		driver->WriteData(buffer, buffer.Size());
	}

	driver->Seek(driver->GetSize());

	return True;
}

Bool BoCA::EncoderSpeex::ConvertArguments(Config *config)
{
	if (!config->GetIntValue("Settings", "EnableConsole", False)) return False;

	static const String	 encoderID = "speex-enc";

	/* Get command line settings.
	 */
	Int	 bitrate    = 16;
	Int	 quality    = 8;
	Int	 complexity = 3;
	Int	 abrrate    = 16;

	if (config->GetIntValue(encoderID, "Set Bitrate", False))	      bitrate	 = config->GetIntValue(encoderID, "Bitrate", bitrate);
	if (config->GetIntValue(encoderID, "Set Quality", False))	      quality	 = config->GetIntValue(encoderID, "Quality", quality);
	if (config->GetIntValue(encoderID, "Set Encoding complexity", False)) complexity = config->GetIntValue(encoderID, "Encoding complexity", complexity);
	if (config->GetIntValue(encoderID, "Set Use ABR encoding", False))    abrrate	 = config->GetIntValue(encoderID, "Use ABR encoding", abrrate);

	/* Set configuration values.
	 */
	config->SetIntValue(ConfigureSpeex::ConfigID, "Mode", -1);
	config->SetIntValue(ConfigureSpeex::ConfigID, "VAD", False);
	config->SetIntValue(ConfigureSpeex::ConfigID, "DTX", False);

	config->SetIntValue(ConfigureSpeex::ConfigID, "VBR", config->GetIntValue(encoderID, "Use VBR encoding", False));

	config->SetIntValue(ConfigureSpeex::ConfigID, "Bitrate", config->GetIntValue(encoderID, "Set Bitrate", False) ? Math::Max(4, Math::Min(64, bitrate)) : -16);
	config->SetIntValue(ConfigureSpeex::ConfigID, "Quality", Math::Max(0, Math::Min(10, quality)));
	config->SetIntValue(ConfigureSpeex::ConfigID, "VBRQuality", Math::Max(0, Math::Min(10, quality)) * 10);
	config->SetIntValue(ConfigureSpeex::ConfigID, "VBRMaxBitrate", -48);
	config->SetIntValue(ConfigureSpeex::ConfigID, "Complexity", Math::Max(0, Math::Min(10, complexity)));
	config->SetIntValue(ConfigureSpeex::ConfigID, "ABR", config->GetIntValue(encoderID, "Set Use ABR encoding", False) ? Math::Max(4, Math::Min(64, abrrate)) : -16);

	return True;
}

ConfigLayer *BoCA::EncoderSpeex::GetConfigurationLayer()
{
	if (configLayer == NIL) configLayer = new ConfigureSpeex();

	return configLayer;
}
