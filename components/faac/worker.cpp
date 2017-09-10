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

BoCA::SuperWorker::SuperWorker(const Config *config, const Format &iFormat)
{
	process	= False;
	flush	= False;
	quit	= False;

	format	= iFormat;

	threadMain.Connect(&SuperWorker::Run, this);

	/* Create FAAC encoder.
	 */
	unsigned long	 samplesSize	= 0;
	unsigned long	 bufferSize	= 0;

	handle = ex_faacEncOpen(format.rate, format.channels, &samplesSize, &bufferSize);

	/* Set encoder parameters.
	 */
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

	frameSize     = samplesSize / format.channels;
	maxPacketSize = bufferSize;
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Destroy FAAC encoder.
	 */
	ex_faacEncClose(handle);
}

Int BoCA::SuperWorker::Run()
{
	while (!quit)
	{
		while (!quit && !process) S::System::System::Sleep(1);

		workerMutex.Lock();

		packetBuffer.Resize(0);
		packetSizes.RemoveAll();

		Int	 framesProcessed = 0;
		Int	 samplesPerFrame = frameSize * format.channels / (format.bits <= 16 ? 2 : 1);

		while (flush || samplesBuffer.Size() - framesProcessed * samplesPerFrame >= samplesPerFrame)
		{
			packetBuffer.Resize(packetBuffer.Size() + maxPacketSize);

			Int	 dataLength = 0;

			if (samplesBuffer.Size() - framesProcessed * samplesPerFrame >= samplesPerFrame) dataLength = ex_faacEncEncode(handle, samplesBuffer + framesProcessed * samplesPerFrame, frameSize * format.channels, packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);
			else										 dataLength = ex_faacEncEncode(handle, NULL, 0, packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);

			if (flush && dataLength == 0) break;

			packetBuffer.Resize(packetBuffer.Size() - maxPacketSize + dataLength);

			packetSizes.Add(dataLength);

			framesProcessed++;
		}

		samplesBuffer.Resize(0);

		workerMutex.Release();

		process	= False;
	}

	return Success();
}

Void BoCA::SuperWorker::Encode(const Buffer<int32_t> &buffer, Int offset, Int size, Bool last)
{
	workerMutex.Lock();

	samplesBuffer.Resize(size);

	memcpy(samplesBuffer, buffer + offset, size * sizeof(int32_t));

	workerMutex.Release();

	flush	= last;
	process = True;
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}
