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

#include "worker.h"

namespace BoCA
{
	CA::OSStatus	 AudioConverterComplexInputDataProc(CA::AudioConverterRef, CA::UInt32 *, CA::AudioBufferList *, CA::AudioStreamPacketDescription **, void *);
};

BoCA::SuperWorker::SuperWorker(const Config *config, const Format &iFormat)
{
	static Endianness	 endianness = CPU().GetEndianness();

	process	= False;
	flush	= False;
	quit	= False;

	format	= iFormat;

	threadMain.Connect(&SuperWorker::Run, this);

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
	destinationFormat.mSampleRate	    = GetOutputSampleRate(destinationFormat.mFormatID, format.rate);
	destinationFormat.mChannelsPerFrame = format.channels;

	CA::UInt32	 formatSize = sizeof(destinationFormat);

	CA::AudioFormatGetProperty(CA::kAudioFormatProperty_FormatInfo, 0, NIL, &formatSize, &destinationFormat);

	/* Create audio converter object.
	 */
	CA::AudioConverterNew(&sourceFormat, &destinationFormat, &converter);

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

	/* Get maximum output packet size.
	 */
	CA::UInt32	 valueSize = 4;

	CA::AudioConverterGetProperty(converter, CA::kAudioConverterPropertyMaximumOutputPacketSize, &valueSize, &bufferSize);

	/* Set up buffer for Core Audio.
	 */
	buffers = (CA::AudioBufferList	*) new unsigned char [sizeof(CA::AudioBufferList) + sizeof(CA::AudioBuffer)];

	buffers->mNumberBuffers = 1;

	buffers->mBuffers[0].mData	     = new unsigned char [bufferSize];
	buffers->mBuffers[0].mDataByteSize   = bufferSize;
	buffers->mBuffers[0].mNumberChannels = format.channels;

	bytesConsumed = 0;
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Free buffer.
	 */
	delete [] (unsigned char *) buffers->mBuffers[0].mData;
	delete [] (unsigned char *) buffers;

	/* Close converter.
	 */
	CA::AudioConverterDispose(converter);
}

Int BoCA::SuperWorker::Run()
{
	while (!quit)
	{
		while (!quit && !process) S::System::System::Sleep(1);

		workerMutex.Lock();

		packetBuffer.Resize(0);
		packetSizes.RemoveAll();

		foreach (CA::AudioStreamPacketDescription *packet, packetInfos) delete packet;

		packetInfos.RemoveAll();

		Int	 bytesPerFrame = frameSize * format.channels * (format.bits / 8);

		CA::UInt32				 packets = 1;
		CA::AudioStreamPacketDescription	 packet;

		buffers->mBuffers[0].mDataByteSize = bufferSize;

		while ((flush || samplesBuffer.Size() - bytesConsumed >= bytesPerFrame) && CA::AudioConverterFillComplexBuffer(converter, &AudioConverterComplexInputDataProc, this, &packets, buffers, &packet) == 0)
		{
			if (buffers->mBuffers[0].mDataByteSize == 0) break;

			packetBuffer.Resize(packetBuffer.Size() + buffers->mBuffers[0].mDataByteSize);

			memcpy(packetBuffer + packetBuffer.Size() - buffers->mBuffers[0].mDataByteSize, buffers->mBuffers[0].mData, buffers->mBuffers[0].mDataByteSize);

			packetSizes.Add(buffers->mBuffers[0].mDataByteSize);
			packetInfos.Add(new CA::AudioStreamPacketDescription(packet));

			buffers->mBuffers[0].mDataByteSize = bufferSize;
		}

		workerMutex.Release();

		process	= False;
	}

	return Success();
}

Void BoCA::SuperWorker::Encode(const Buffer<unsigned char> &buffer, Int offset, Int size, Bool last)
{
	workerMutex.Lock();

	samplesBuffer.Resize(samplesBuffer.Size() + size);

	memmove(samplesBuffer, samplesBuffer + bytesConsumed, samplesBuffer.Size() - bytesConsumed - size);
	memcpy(samplesBuffer + samplesBuffer.Size() - bytesConsumed - size, buffer + offset, size);

	samplesBuffer.Resize(samplesBuffer.Size() - bytesConsumed);

	bytesConsumed = 0;

	workerMutex.Release();

	flush	= last;
	process = True;
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}

CA::AudioConverterPrimeInfo BoCA::SuperWorker::GetPrimeInfo() const
{
	CA::AudioConverterPrimeInfo	 primeInfo;
	CA::UInt32			 size = sizeof(primeInfo);

	if (CA::AudioConverterGetProperty(converter, CA::kAudioConverterPrimeInfo, &size, &primeInfo) != 0)
	{
		primeInfo.leadingFrames	 = 0xFFFFFFFF;
		primeInfo.trailingFrames = 0xFFFFFFFF;
	}

	return primeInfo;
}

unsigned char *BoCA::SuperWorker::GetMagicCookie(CA::UInt32 *cookieSize) const
{
	if (CA::AudioConverterGetPropertyInfo(converter, CA::kAudioConverterCompressionMagicCookie, cookieSize, NIL) == 0)
	{
		unsigned char	*cookie = new unsigned char [*cookieSize];

		CA::AudioConverterGetProperty(converter, CA::kAudioConverterCompressionMagicCookie, cookieSize, cookie);

		return cookie;
	}

	return NIL;
}

Int BoCA::SuperWorker::GetOutputSampleRate(CA::UInt32 format, Int inputRate)
{
	/* Get supported sample rate ranges for selected codec.
	 */
	CA::UInt32	 size = 0;

	CA::AudioFormatGetPropertyInfo(CA::kAudioFormatProperty_AvailableEncodeSampleRates, sizeof(format), &format, &size);

	if (size == 0) return inputRate;

	CA::AudioValueRange	*sampleRates = new CA::AudioValueRange [size / sizeof(CA::AudioValueRange)];

	CA::AudioFormatGetProperty(CA::kAudioFormatProperty_AvailableEncodeSampleRates, sizeof(format), &format, &size, sampleRates);

	/* Find best fit output sample rate.
	 */
	Int	 outputRate = 0;

	for (UnsignedInt i = 0; i < size / sizeof(CA::AudioValueRange); i++)
	{
		/* Check if encoder supports arbitrary sample rate.
		 */
		if (sampleRates[i].mMinimum == 0 &&
		    sampleRates[i].mMaximum == 0) { outputRate = inputRate; break; }

		/* Check if input rate falls into current sample rate range.
		 */
		if (inputRate >= sampleRates[i].mMinimum &&
		    inputRate <= sampleRates[i].mMaximum) { outputRate = inputRate; break; }

		/* Check if current sample rate range fits better than previous best.
		 */
		if (Math::Abs(inputRate - sampleRates[i].mMinimum) < Math::Abs(inputRate - outputRate)) outputRate = sampleRates[i].mMinimum;
		if (Math::Abs(inputRate - sampleRates[i].mMaximum) < Math::Abs(inputRate - outputRate)) outputRate = sampleRates[i].mMaximum;
	}

	delete [] sampleRates;

	return outputRate;
}

CA::OSStatus BoCA::AudioConverterComplexInputDataProc(CA::AudioConverterRef inAudioConverter, CA::UInt32 *ioNumberDataPackets, CA::AudioBufferList *ioData, CA::AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
	SuperWorker	*worker = (SuperWorker *) inUserData;
	const Format	&format = worker->format;

	worker->suppliedData.Resize(Math::Min(worker->samplesBuffer.Size() - worker->bytesConsumed, *ioNumberDataPackets * format.channels * (format.bits / 8)));

	memcpy(worker->suppliedData, worker->samplesBuffer + worker->bytesConsumed, worker->suppliedData.Size());

	*ioNumberDataPackets = worker->suppliedData.Size() / format.channels / (format.bits / 8);

	ioData->mBuffers[0].mData           = worker->suppliedData;
	ioData->mBuffers[0].mDataByteSize   = worker->suppliedData.Size();
	ioData->mBuffers[0].mNumberChannels = format.channels;

	worker->bytesConsumed += ioData->mBuffers[0].mDataByteSize;

	return 0;
}
