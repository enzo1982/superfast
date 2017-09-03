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

#include "coreaudio.h"
#include "config.h"

using namespace smooth::IO;

const String &BoCA::EncoderCoreAudio::GetComponentSpecs()
{
	static String	 componentSpecs;

#ifndef __APPLE__
	if (corefoundationdll != NIL && coreaudiodll != NIL)
#endif
	{
		componentSpecs = "						\
										\
		  <?xml version=\"1.0\" encoding=\"UTF-8\"?>			\
		  <component>							\
		    <name>Core Audio AAC/ALAC Encoder (SuperFast)</name>	\
		    <version>1.0</version>					\
		    <id>supercoreaudio-enc</id>					\
		    <type>encoder</type>					\
		    <replace>superfaac-enc</replace>				\
		    <replace>voaacenc-enc</replace>				\
		    <replace>avconv-alac-enc</replace>				\
		    <format>							\
		      <name>MPEG-4 AAC Files</name>				\
		      <extension>m4a</extension>				\
		      <extension>m4b</extension>				\
		      <extension>m4r</extension>				\
		      <extension>mp4</extension>				\
		      <tag id=\"mp4-tag\" mode=\"other\">MP4 Metadata</tag>	\
		    </format>							\
		    <format>							\
		      <name>Raw AAC Files</name>				\
		      <extension>aac</extension>				\
		      <tag id=\"id3v2-tag\" mode=\"prepend\">ID3v2</tag>	\
		    </format>							\
		    <format>							\
		      <name>Apple Lossless Files</name>				\
		      <extension>m4a</extension>				\
		      <extension>m4b</extension>				\
		      <extension>mp4</extension>				\
		      <tag id=\"mp4-tag\" mode=\"other\">MP4 Metadata</tag>	\
		    </format>							\
		  </component>							\
										\
		";
	}

	return componentSpecs;
}

Void smooth::AttachDLL(Void *instance)
{
#ifndef __APPLE__
	LoadCoreFoundationDLL();
	LoadCoreAudioDLL();
#endif
}

Void smooth::DetachDLL()
{
#ifndef __APPLE__
	FreeCoreFoundationDLL();
	FreeCoreAudioDLL();
#endif
}

BoCA::EncoderCoreAudio::EncoderCoreAudio()
{
	configLayer    = NIL;

	audioFile      = NIL;

	blockSize      = 128;
	overlap	       = 8;

	nextWorker     = 0;

	packetsWritten = 0;
	packetsMissing = 0;

	totalSamples   = 0;
}

BoCA::EncoderCoreAudio::~EncoderCoreAudio()
{
	if (configLayer != NIL) Object::DeleteObject(configLayer);
}

Bool BoCA::EncoderCoreAudio::IsLossless() const
{
	const Config	*config = GetConfiguration();

	if (config->GetIntValue("CoreAudio", "Codec", 'aac ') == 'alac') return True;

	return False;
}

Bool BoCA::EncoderCoreAudio::Activate()
{
	static Endianness	 endianness = CPU().GetEndianness();

	const Format	&format = track.GetFormat();

	if (format.channels > 8)
	{
		errorString = "This encoder does not support more than 8 channels!";
		errorState  = True;

		return False;
	}

	const Config	*config = GetConfiguration();

	/* Fill out source format description.
	 */
	CA::AudioStreamBasicDescription	 sourceFormat = { 0 };

	sourceFormat.mFormatID		    = CA::kAudioFormatLinearPCM;
	sourceFormat.mFormatFlags	    = CA::kLinearPCMFormatFlagIsPacked | (format.bits > 8	  ? CA::kLinearPCMFormatFlagIsSignedInteger : 0) |
										 (endianness == EndianBig ? CA::kLinearPCMFormatFlagIsBigEndian	    : 0);
	sourceFormat.mSampleRate	    = format.rate;
	sourceFormat.mChannelsPerFrame	    = format.channels;
	sourceFormat.mBitsPerChannel	    = format.bits;
	sourceFormat.mFramesPerPacket	    = 1;
	sourceFormat.mBytesPerFrame	    = sourceFormat.mChannelsPerFrame * sourceFormat.mBitsPerChannel / 8;
	sourceFormat.mBytesPerPacket	    = sourceFormat.mFramesPerPacket * sourceFormat.mBytesPerFrame;

	/* Fill out destination format description.
	 */
	CA::AudioStreamBasicDescription	 destinationFormat = { 0 };

	destinationFormat.mFormatID	    = config->GetIntValue("CoreAudio", "Codec", 'aac ');
	destinationFormat.mSampleRate	    = SuperWorker::GetOutputSampleRate(destinationFormat.mFormatID, format.rate);
	destinationFormat.mChannelsPerFrame = format.channels;

	CA::UInt32	 formatSize = sizeof(destinationFormat);

	CA::AudioFormatGetProperty(CA::kAudioFormatProperty_FormatInfo, 0, NIL, &formatSize, &destinationFormat);

	/* Create audio converter object.
	 */
	CA::AudioConverterRef	 converter = NIL;

	if (CA::AudioConverterNew(&sourceFormat, &destinationFormat, &converter) != 0)
	{
		errorString = "Could not create converter component!";
		errorState  = True;

		return False;
	}

	frameSize = destinationFormat.mFramesPerPacket;

	/* Set bitrate if format does support bitrates.
	 */
	CA::UInt32	 size = 0;

	if (CA::AudioConverterGetPropertyInfo(converter, CA::kAudioConverterApplicableEncodeBitRates, &size, NIL) == 0)
	{
		/* Get applicable bitrate values.
		 */
		CA::UInt32		 bitrate       = config->GetIntValue("CoreAudio", "Bitrate", 64) * 1000 * format.channels;
		CA::AudioValueRange	*bitrateValues = new CA::AudioValueRange [size / sizeof(CA::AudioValueRange)];

		CA::AudioConverterGetProperty(converter, CA::kAudioConverterApplicableEncodeBitRates, &size, bitrateValues);

		/* Find best supported bitrate.
		 */
		CA::Float64	 nearest = 0xFFFFFFFF;

		for (UnsignedInt i = 0; i < size / sizeof(CA::AudioValueRange); i++)
		{
			if (bitrate >= bitrateValues[i].mMinimum && bitrate <= bitrateValues[i].mMaximum)  nearest = bitrate;

			if (Math::Abs(bitrate - bitrateValues[i].mMinimum) < Math::Abs(bitrate - nearest)) nearest = bitrateValues[i].mMinimum;
			if (Math::Abs(bitrate - bitrateValues[i].mMaximum) < Math::Abs(bitrate - nearest)) nearest = bitrateValues[i].mMaximum;
		}

		bitrate = nearest;

		delete [] bitrateValues;

		/* Set bitrate on converter.
		 */
		CA::AudioConverterSetProperty(converter, CA::kAudioConverterEncodeBitRate, sizeof(CA::UInt32), &bitrate);
	}

	/* Create audio file object for output file.
	 */
	String		 fileName	= Utilities::GetNonUnicodeTempFileName(track.outfile).Append(".out");
	CA::CFStringRef	 fileNameString	= CA::CFStringCreateWithCString(NULL, fileName.ConvertTo("UTF-8"), CA::kCFStringEncodingUTF8);

#ifdef __WIN32__
	CA::CFURLRef	 fileNameURL	= CA::CFURLCreateWithFileSystemPath(NULL, fileNameString, CA::kCFURLWindowsPathStyle, False);
#else
	CA::CFURLRef	 fileNameURL	= CA::CFURLCreateWithFileSystemPath(NULL, fileNameString, CA::kCFURLPOSIXPathStyle, False);
#endif

	CA::UInt32	 fileType	= config->GetIntValue("CoreAudio", "MP4Container", True) ? CA::kAudioFileM4AType : CA::kAudioFileAAC_ADTSType;

	CA::AudioFileCreateWithURL(fileNameURL, fileType, &destinationFormat, CA::kAudioFileFlags_EraseFile, &audioFile);

	CA::CFRelease(fileNameURL);
	CA::CFRelease(fileNameString);

	/* Get magic cookie and supply it to audio file.
	 */
	CA::UInt32	 cookieSize = 0;

	if (CA::AudioConverterGetPropertyInfo(converter, CA::kAudioConverterCompressionMagicCookie, &cookieSize, NIL) == 0)
	{
		unsigned char	*cookie = new unsigned char [cookieSize];

		CA::AudioConverterGetProperty(converter, CA::kAudioConverterCompressionMagicCookie, &cookieSize, cookie);
		CA::AudioFileSetProperty(audioFile, CA::kAudioFilePropertyMagicCookieData, cookieSize, cookie);

		delete [] cookie;
	}

	CA::AudioConverterDispose(converter);

	packetsWritten = 0;
	packetsMissing = 0;

	totalSamples   = 0;

	/* Write ID3v2 tag if requested.
	 */
	if (!config->GetIntValue("CoreAudio", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue("CoreAudio", "AllowID3v2", False))
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

Bool BoCA::EncoderCoreAudio::Deactivate()
{
	const Config	*config = GetConfiguration();

	/* Convert final frames.
	 */
	EncodeFrames(True);

	/* Write priming and remainder info.
	 */
	CA::AudioConverterPrimeInfo	 initialInfo = workers.GetNth(0					 )->GetPrimeInfo();
	CA::AudioConverterPrimeInfo	 finalInfo   = workers.GetNth((nextWorker - 1) % workers.Length())->GetPrimeInfo();

	if (initialInfo.leadingFrames != 0xFFFFFFFF && finalInfo.trailingFrames != 0xFFFFFFFF)
	{
		Int	 divider = 1;
		Int	 extra	 = 0;

		if (config->GetIntValue("CoreAudio", "Codec", 'aac ') == 'aach' ||
		    config->GetIntValue("CoreAudio", "Codec", 'aac ') == 'aacp') { divider = 2; extra = 480; }

		CA::AudioFilePacketTableInfo	 pti;

		pti.mPrimingFrames     = initialInfo.leadingFrames + extra;
		pti.mRemainderFrames   = finalInfo.trailingFrames;
		pti.mNumberValidFrames = totalSamples / divider;

		CA::AudioFileSetProperty(audioFile, CA::kAudioFilePropertyPacketTableInfo, sizeof(pti), &pti);
	}

	/* Get and set magic cookie again as some
	 * encoders may change it during encoding.
	 */
	CA::UInt32	 cookieSize = 4;
	unsigned char	*cookie	    = workers.GetFirst()->GetMagicCookie(&cookieSize);

	if (cookie != NIL)
	{
		CA::AudioFileSetProperty(audioFile, CA::kAudioFilePropertyMagicCookieData, cookieSize, cookie);

		delete [] cookie;
	}

	/* Tear down worker threads.
	 */
	foreach (SuperWorker *worker, workers) worker->Quit();
	foreach (SuperWorker *worker, workers) worker->Wait();
	foreach (SuperWorker *worker, workers) delete worker;

	workers.RemoveAll();

	/* Close audio file.
	 */
	CA::AudioFileClose(audioFile);

	/* Write metadata to file
	 */
	if (config->GetIntValue("CoreAudio", "MP4Container", True) && config->GetIntValue("Tags", "EnableMP4Metadata", True))
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

	/* Write ID3v1 tag if requested.
	 */
	if (!config->GetIntValue("CoreAudio", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v1", False))
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
	if (!config->GetIntValue("CoreAudio", "MP4Container", True) && config->GetIntValue("Tags", "EnableID3v2", True) && config->GetIntValue("CoreAudio", "AllowID3v2", False))
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

Int BoCA::EncoderCoreAudio::WriteData(Buffer<UnsignedByte> &data)
{
	const Format	&format	= track.GetFormat();

	/* Change to AAC channel order.
	 */
	if	(format.channels == 3) Utilities::ChangeChannelOrder(data, format, Channel::Default_3_0, Channel::AAC_3_0);
	else if (format.channels == 5) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_0, Channel::AAC_5_0);
	else if (format.channels == 6) Utilities::ChangeChannelOrder(data, format, Channel::Default_5_1, Channel::AAC_5_1);
	else if (format.channels == 7) Utilities::ChangeChannelOrder(data, format, Channel::Default_6_1, Channel::AAC_6_1);
	else if (format.channels == 8) Utilities::ChangeChannelOrder(data, format, Channel::Default_7_1, Channel::AAC_7_1);

	/* Configure buffer.
	 */
	samplesBuffer.Resize(samplesBuffer.Size() + data.Size());

	memcpy(samplesBuffer + samplesBuffer.Size() - data.Size(), data, data.Size());

	/* Convert frames.
	 */
	totalSamples += data.Size() / format.channels / (format.bits / 8);

	return EncodeFrames(False);
}

Int BoCA::EncoderCoreAudio::EncodeFrames(Bool flush)
{
	const Format	&format = track.GetFormat();

	/* Pass samples to workers.
	 */
	Int	 framesToProcess = blockSize;
	Int	 framesProcessed = 0;
	Int	 dataLength	 = 0;

	Int	 bytesPerFrame = frameSize * format.channels * (format.bits / 8);

	if (flush) framesToProcess = Math::Floor(samplesBuffer.Size() / bytesPerFrame);

	while (samplesBuffer.Size() - framesProcessed * bytesPerFrame >= bytesPerFrame * framesToProcess)
	{
		SuperWorker	*workerToUse = workers.GetNth(nextWorker % workers.Length());

		while (!workerToUse->IsReady()) S::System::System::Sleep(1);

		workerToUse->Lock();

		/* See if the worker has some packets for us.
		 */
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), workerToUse->GetPacketInfos(), nextWorker == workers.Length(), nextWorker >= 2 * workers.Length());

		/* Pass new frames to worker.
		 */
		workerToUse->Encode(samplesBuffer, framesProcessed * bytesPerFrame, flush ? samplesBuffer.Size() : bytesPerFrame * framesToProcess, flush);
		workerToUse->Release();

		framesProcessed += framesToProcess - overlap;

		nextWorker++;

		if (flush) break;
	}

	memmove(samplesBuffer, samplesBuffer + framesProcessed * bytesPerFrame, samplesBuffer.Size() - framesProcessed * bytesPerFrame);

	samplesBuffer.Resize(samplesBuffer.Size() - framesProcessed * bytesPerFrame);

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
		if (workerToUse->GetPacketSizes().Length() != 0) dataLength += ProcessPackets(workerToUse->GetPackets(), workerToUse->GetPacketSizes(), workerToUse->GetPacketInfos(), nextWorker == workers.Length(), nextWorker >= 2 * workers.Length());

		workerToUse->Release();

		nextWorker++;
	}

	return dataLength;
}

Int BoCA::EncoderCoreAudio::ProcessPackets(const Buffer<unsigned char> &packetData, const Array<Int> &packetSizes, const Array<CA::AudioStreamPacketDescription *> &packetInfos, Bool first, Bool discardMissing)
{
	Int	 offset	    = 0;
	Int	 dataLength = 0;

	if (discardMissing) packetsMissing = 0;

	if (!first) for (Int i = 0; i < overlap - packetsMissing; i++) offset += packetSizes.GetNth(i);

	for (Int i = 0; i < packetSizes.Length(); i++)
	{
		if (i <	overlap - packetsMissing && !first) continue;
		if (packetSizes.GetNth(i) == 0)		    continue;

		CA::UInt32				 packets = 1;
		CA::AudioStreamPacketDescription	*packet	 = packetInfos.GetNth(i);

		CA::AudioFileWritePackets(audioFile, False, packetSizes.GetNth(i), packet, packetsWritten, &packets, (unsigned char *) packetData + offset);

		offset	   += packetSizes.GetNth(i);
		dataLength += packetSizes.GetNth(i);

		packetsWritten += packets;
	}

	packetsMissing = blockSize - packetSizes.Length();

	return dataLength;
}

Bool BoCA::EncoderCoreAudio::SetOutputFormat(Int n)
{
	Config	*config = Config::Get();

	if (n != 1) config->SetIntValue("CoreAudio", "MP4Container", True);
	else	    config->SetIntValue("CoreAudio", "MP4Container", False);

	if	(n != 2 && config->GetIntValue("CoreAudio", "Codec", 'aac ') == 'alac') config->SetIntValue("CoreAudio", "Codec", 'aac ');
	else if (n == 2)								config->SetIntValue("CoreAudio", "Codec", 'alac');

	return True;
}

String BoCA::EncoderCoreAudio::GetOutputFileExtension() const
{
	const Config	*config = GetConfiguration();

	if (config->GetIntValue("CoreAudio", "MP4Container", True))
	{
		switch (config->GetIntValue("CoreAudio", "MP4FileExtension", 0))
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

ConfigLayer *BoCA::EncoderCoreAudio::GetConfigurationLayer()
{
	if (configLayer == NIL) configLayer = new ConfigureCoreAudio();

	return configLayer;
}
