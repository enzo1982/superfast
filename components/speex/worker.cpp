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
	quit	= False;

	format	= iFormat;

	threadMain.Connect(&SuperWorker::Run, this);

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

	/* Create Speex encoder.
	 */
	encoder = ex_speex_encoder_init(ex_speex_lib_get_mode(modeID));

	/* Set encoder options.
	 */
	spx_int32_t	 vbr = config->GetIntValue(ConfigureSpeex::ConfigID, "VBR", 0);
	spx_int32_t	 abr = config->GetIntValue(ConfigureSpeex::ConfigID, "ABR", -16) * 1000;
	spx_int32_t	 complexity = config->GetIntValue(ConfigureSpeex::ConfigID, "Complexity", 3);

	ex_speex_encoder_ctl(encoder, SPEEX_SET_VBR, &vbr);
	ex_speex_encoder_ctl(encoder, SPEEX_SET_COMPLEXITY, &complexity);

	if (abr > 0) ex_speex_encoder_ctl(encoder, SPEEX_SET_ABR, &abr);

	if (vbr)
	{
		float		 vbrq = config->GetIntValue(ConfigureSpeex::ConfigID, "VBRQuality", 80) / 10.0;
		spx_int32_t	 vbrmax = config->GetIntValue(ConfigureSpeex::ConfigID, "VBRMaxBitrate", -48) * 1000;

		ex_speex_encoder_ctl(encoder, SPEEX_SET_VBR_QUALITY, &vbrq);

		if (vbrmax > 0) ex_speex_encoder_ctl(encoder, SPEEX_SET_VBR_MAX_BITRATE, &vbrmax);
	}
	else
	{
		spx_int32_t	 quality = config->GetIntValue(ConfigureSpeex::ConfigID, "Quality", 8);
		spx_int32_t	 bitrate = config->GetIntValue(ConfigureSpeex::ConfigID, "Bitrate", -16) * 1000;
		spx_int32_t	 vad = config->GetIntValue(ConfigureSpeex::ConfigID, "VAD", 0);

		if (quality > 0) ex_speex_encoder_ctl(encoder, SPEEX_SET_QUALITY, &quality);
		if (bitrate > 0) ex_speex_encoder_ctl(encoder, SPEEX_SET_BITRATE, &bitrate);

		ex_speex_encoder_ctl(encoder, SPEEX_SET_VAD, &vad);
	}

	if (vbr || abr > 0 || config->GetIntValue(ConfigureSpeex::ConfigID, "VAD", 0))
	{
		spx_int32_t	 dtx = config->GetIntValue(ConfigureSpeex::ConfigID, "DTX", 0);

		ex_speex_encoder_ctl(encoder, SPEEX_SET_DTX, &dtx);
	}

	/* Get frame size and look-ahead.
	 */
	spx_int32_t	 rate = format.rate;

	ex_speex_encoder_ctl(encoder, SPEEX_SET_SAMPLING_RATE, &rate);
	ex_speex_encoder_ctl(encoder, SPEEX_GET_FRAME_SIZE, &frameSize);

	ex_speex_bits_init(&bits);
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Destroy Speex encoder.
	 */
	ex_speex_bits_destroy(&bits);

	ex_speex_encoder_destroy(encoder);
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

		while (samplesLeft >= samplesPerFrame)
		{
			if (format.channels == 2) ex_speex_encode_stereo_int(samplesBuffer + framesProcessed * samplesPerFrame, frameSize, &bits);

			ex_speex_encode_int(encoder, samplesBuffer + framesProcessed * samplesPerFrame, &bits);
			ex_speex_bits_insert_terminator(&bits);

			Int	 dataLength = ex_speex_bits_nbytes(&bits);

			packetBuffer.Resize(packetBuffer.Size() + dataLength);

			dataLength = ex_speex_bits_write(&bits, (char *) (unsigned char *) packetBuffer + packetBuffer.Size() - dataLength, dataLength);

			ex_speex_bits_reset(&bits);

			if (dataLength > 0) packetSizes.Add(dataLength);

			framesProcessed++;
			samplesLeft -= samplesPerFrame;
		}

		workerMutex.Release();

		process	= False;
	}

	return Success();
}

Void BoCA::SuperWorker::Encode(const Buffer<spx_int16_t> &buffer, Int offset, Int size)
{
	workerMutex.Lock();

	samplesBuffer.Resize(size);

	memcpy(samplesBuffer, buffer + offset, size * sizeof(spx_int16_t));

	workerMutex.Release();

	process = True;
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}
