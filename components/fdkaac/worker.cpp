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
	Bool	 mp4Container = config->GetIntValue(ConfigureFDKAAC::ConfigID, "MP4Container", True);
	Int	 mpegVersion  = config->GetIntValue(ConfigureFDKAAC::ConfigID, "MPEGVersion", 0);
	Int	 aacType      = config->GetIntValue(ConfigureFDKAAC::ConfigID, "AACType", AOT_SBR);
	Int	 bitrate      = config->GetIntValue(ConfigureFDKAAC::ConfigID, "Bitrate", 64);

	/* Create and configure FDK AAC encoder.
	 */
	ex_aacEncOpen(&handle, 0x07, format.channels);

	Int	 channelMode = MODE_UNKNOWN;

	switch (format.channels)
	{
		case 1: channelMode = MODE_1;	      break;
		case 2: channelMode = MODE_2;	      break;
		case 3: channelMode = MODE_1_2;	      break;
		case 4: channelMode = MODE_1_2_1;     break;
		case 5: channelMode = MODE_1_2_2;     break;
		case 6: channelMode = MODE_1_2_2_1;   break;
		case 8: channelMode = MODE_1_2_2_2_1; break;
	}

	ex_aacEncoder_SetParam(handle, AACENC_SAMPLERATE, format.rate);
	ex_aacEncoder_SetParam(handle, AACENC_CHANNELMODE, channelMode);
	ex_aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1 /* WAVE channel order */);

	ex_aacEncoder_SetParam(handle, AACENC_AOT, mpegVersion + aacType);
	ex_aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate * 1000 * format.channels);
	ex_aacEncoder_SetParam(handle, AACENC_AFTERBURNER, 1);
	ex_aacEncoder_SetParam(handle, AACENC_TRANSMUX, mp4Container ? TT_MP4_RAW : TT_MP4_ADTS);

	if (!mp4Container)
	{
		if (aacType == AOT_ER_AAC_LD ||
		    aacType == AOT_ER_AAC_ELD) ex_aacEncoder_SetParam(handle, AACENC_TRANSMUX, TT_MP4_LOAS);
	}

	AACENC_InfoStruct	 aacInfo;

	ex_aacEncEncode(handle, NULL, NULL, NULL, NULL);
	ex_aacEncInfo(handle, &aacInfo);

	frameSize     = aacInfo.frameLength;
	maxPacketSize = aacInfo.maxOutBufBytes;
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Destroy FDK encoder.
	 */
	ex_aacEncClose(&handle);
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
		Int	 samplesPerFrame = frameSize * format.channels;

		while (samplesBuffer.Size() - framesProcessed * samplesPerFrame >= samplesPerFrame)
		{
			packetBuffer.Resize(packetBuffer.Size() + maxPacketSize);

			/* Prepare buffer information.
			 */
			Void	*inputBuffer	   = (int16_t *) samplesBuffer + framesProcessed * samplesPerFrame;
			Int	 inputBufferID	   = IN_AUDIO_DATA;
			Int	 inputBufferSize   = samplesPerFrame;
			Int	 inputElementSize  = 2;

			Void	*outputBuffer	   = (uint8_t *) packetBuffer + packetBuffer.Size() - maxPacketSize;
			Int	 outputBufferID	   = OUT_BITSTREAM_DATA;
			Int	 outputBufferSize  = maxPacketSize;
			Int	 outputElementSize = 1;

			/* Configure buffer descriptors.
			 */
			AACENC_BufDesc	 input	     = { 0 };
			AACENC_BufDesc	 output	     = { 0 };

			input.numBufs		 = 1;
			input.bufs		 = &inputBuffer;
			input.bufferIdentifiers  = &inputBufferID;
			input.bufSizes		 = &inputBufferSize;
			input.bufElSizes	 = &inputElementSize;

			output.numBufs		 = 1;
			output.bufs		 = &outputBuffer;
			output.bufferIdentifiers = &outputBufferID;
			output.bufSizes		 = &outputBufferSize;
			output.bufElSizes	 = &outputElementSize;

			/* Hand input data to encoder and retrieve output.
			*/
			AACENC_InArgs	 inputInfo   = { 0 };
			AACENC_OutArgs	 outputInfo  = { 0 };

			inputInfo.numInSamples = samplesPerFrame;

			Int	 dataLength = 0;

			if (ex_aacEncEncode(handle, &input, &output, &inputInfo, &outputInfo) == AACENC_OK) dataLength = outputInfo.numOutBytes;

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
