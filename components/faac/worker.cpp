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
#include "config.h"

BoCA::SuperWorker::SuperWorker(const Config *config, const Format &iFormat)
{
	process	= False;
	flush	= False;
	quit	= False;

	format	= iFormat;

	threadMain.Connect(&SuperWorker::Run, this);

	/* Get configuration.
	 */
	Bool	 mp4Container = config->GetIntValue(ConfigureFAAC::ConfigID, "MP4Container", True);
	Int	 mpegVersion  = config->GetIntValue(ConfigureFAAC::ConfigID, "MPEGVersion", 0);
	Bool	 setQuality   = config->GetIntValue(ConfigureFAAC::ConfigID, "SetQuality", True);
	Int	 aacQuality   = config->GetIntValue(ConfigureFAAC::ConfigID, "AACQuality", 100);
	Int	 bitrate      = config->GetIntValue(ConfigureFAAC::ConfigID, "Bitrate", 96);
	Bool	 allowJS      = config->GetIntValue(ConfigureFAAC::ConfigID, "AllowJS", True);
	Bool	 useTNS	      = config->GetIntValue(ConfigureFAAC::ConfigID, "UseTNS", False);
	Int	 bandwidth    = config->GetIntValue(ConfigureFAAC::ConfigID, "BandWidth", 22050);

	/* Create FAAC encoder.
	 */
	unsigned long	 samplesSize	= 0;
	unsigned long	 bufferSize	= 0;

	handle = ex_faacEncOpen(format.rate, format.channels, &samplesSize, &bufferSize);

	/* Set encoder parameters.
	 */
	faacEncConfigurationPtr	 fConfig = ex_faacEncGetCurrentConfiguration(handle);

	fConfig->mpegVersion	= mp4Container ? MPEG4 : mpegVersion;
	fConfig->aacObjectType	= LOW;
	fConfig->allowMidside	= allowJS;
	fConfig->useTns		= useTNS;
	fConfig->bandWidth	= bandwidth;

	if (mp4Container) fConfig->outputFormat = 0; // Raw AAC frame headers

	if (setQuality)	fConfig->quantqual = aacQuality;
	else		fConfig->bitRate   = bitrate * 1000;

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
