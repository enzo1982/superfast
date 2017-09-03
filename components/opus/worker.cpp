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

#include <stdint.h>

#include "worker.h"
#include "config.h"

BoCA::SuperWorker::SuperWorker(const Config *config, const Format &iFormat)
{
	process	= False;
	quit	= False;

	format	= iFormat;

	threadMain.Connect(&SuperWorker::Run, this);

	/* Get best sample rate.
	 */
	Int	 sampleRate = 48000;

	if	(format.rate <=  8000) sampleRate =  8000;
	else if (format.rate <= 12000) sampleRate = 12000;
	else if (format.rate <= 16000) sampleRate = 16000;
	else if (format.rate <= 24000) sampleRate = 24000;

	/* Create Opus encoder.
	 */
	int	 error	 = 0;
	int	 streams = 0;
	int	 coupled = 0;
	uint8_t	 stream_map[255];

	encoder = ex_opus_multistream_surround_encoder_create(sampleRate, format.channels, format.channels <= 2 ? 0 : 1, &streams, &coupled, stream_map, OPUS_APPLICATION_AUDIO, &error);

	/* Set encoder parameters.
	 */
	if (config->GetIntValue("Opus", "Mode", 0)	!= 0) ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE + config->GetIntValue("Opus", "Mode", 0) - 1));
	if (config->GetIntValue("Opus", "Bandwidth", 0) != 0) ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND + config->GetIntValue("Opus", "Bandwidth", 0) - 1));

	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_BITRATE( config->GetIntValue("Opus", "Bitrate", 128) * 1000));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_VBR(config->GetIntValue("Opus", "EnableVBR", True)));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_VBR_CONSTRAINT(config->GetIntValue("Opus", "EnableConstrainedVBR", False)));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(config->GetIntValue("Opus", "Complexity", 10)));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(config->GetIntValue("Opus", "PacketLoss", 0)));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_DTX(config->GetIntValue("Opus", "EnableDTX", False)));
	ex_opus_multistream_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(0));

	frameSize     = Math::Round(Float(sampleRate) / (1000000.0 / config->GetIntValue("Opus", "FrameSize", 20000)));
	maxPacketSize = 4000 * Math::Ceil(format.channels / 2.0);
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Destroy Opus encoder.
	 */
	ex_opus_multistream_encoder_destroy(encoder);
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

		while (samplesBuffer.Size() - framesProcessed * frameSize * format.channels >= frameSize * format.channels)
		{
			packetBuffer.Resize(packetBuffer.Size() + maxPacketSize);

			Int	 dataLength = ex_opus_multistream_encode(encoder, samplesBuffer + framesProcessed * frameSize * format.channels, frameSize, packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);

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

Void BoCA::SuperWorker::Encode(const Buffer<signed short> &buffer, Int offset, Int size)
{
	workerMutex.Lock();

	samplesBuffer.Resize(size);

	memcpy(samplesBuffer, buffer + offset, size * sizeof(signed short));

	workerMutex.Release();

	process = True;
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}
