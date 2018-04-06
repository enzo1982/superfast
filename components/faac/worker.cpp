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

	fConfig->inputFormat	= FAAC_INPUT_16BIT;
	fConfig->outputFormat	= mp4Container ? RAW_STREAM : ADTS_STREAM;
	fConfig->mpegVersion	= mp4Container ? MPEG4 : mpegVersion;
	fConfig->aacObjectType	= LOW;
	fConfig->quantqual	= setQuality ? aacQuality : 0;
	fConfig->bitRate	= setQuality ? 0 : bitrate * 1000;
	fConfig->bandWidth	= bandwidth;
	fConfig->jointmode	= allowJS ? JOINT_IS : JOINT_NONE;
	fConfig->useTns		= useTNS;

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

		Int	 samplesLeft	 = samplesBuffer.Size();
		Int	 samplesPerFrame = frameSize * format.channels;

		Int	 framesProcessed = 0;

		while (flush || samplesLeft >= samplesPerFrame)
		{
			packetBuffer.Resize(packetBuffer.Size() + maxPacketSize);

			Int	 dataLength = 0;

			if (samplesLeft > 0) dataLength = ex_faacEncEncode(handle, (int32_t *) (int16_t *) (samplesBuffer + framesProcessed * samplesPerFrame), Math::Min(samplesLeft, samplesPerFrame), packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);
			else		     dataLength = ex_faacEncEncode(handle, NULL,									0,					 packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);

			packetBuffer.Resize(packetBuffer.Size() - maxPacketSize + dataLength);

			if (samplesLeft < 0 && dataLength == 0) break;

			if (dataLength > 0) packetSizes.Add(dataLength);

			framesProcessed++;
			samplesLeft -= samplesPerFrame;
		}

		samplesBuffer.Resize(0);

		workerMutex.Release();

		process	= False;
	}

	return Success();
}

Void BoCA::SuperWorker::Encode(const Buffer<int16_t> &buffer, Int offset, Int size, Bool last)
{
	workerMutex.Lock();

	samplesBuffer.Resize(size);

	memcpy(samplesBuffer, buffer + offset, size * sizeof(int16_t));

	workerMutex.Release();

	flush	= last;
	process = True;
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}
