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

BoCA::SuperWorker::SuperWorker(const Config *config, const Format &iFormat, Int iOverlap)
{
	process	= False;
	flush	= False;
	quit	= False;

	format	= iFormat;

	overlap	= iOverlap;

	threadMain.Connect(&SuperWorker::Run, this);

	/* Get configuration.
	 */
	Int	 preset		  = config->GetIntValue(ConfigureLAME::ConfigID, "Preset", 2);
	Int	 vbrMode	  = config->GetIntValue(ConfigureLAME::ConfigID, "VBRMode", 4);
	Bool	 setBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "SetBitrate", 1);
	Int	 bitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "Bitrate", 192);
	Int	 ratio		  = config->GetIntValue(ConfigureLAME::ConfigID, "Ratio", 1100);
	Bool	 setQuality	  = config->GetIntValue(ConfigureLAME::ConfigID, "SetQuality", 0);
	Int	 quality	  = config->GetIntValue(ConfigureLAME::ConfigID, "Quality", 3);
	Int	 abrBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "ABRBitrate", 192);
	Int	 vbrQuality	  = config->GetIntValue(ConfigureLAME::ConfigID, "VBRQuality", 50);
	Bool	 setMinVBRBitrate = config->GetIntValue(ConfigureLAME::ConfigID, "SetMinVBRBitrate", 0);
	Bool	 setMaxVBRBitrate = config->GetIntValue(ConfigureLAME::ConfigID, "SetMaxVBRBitrate", 0);
	Int	 minVBRBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "MinVBRBitrate", 128);
	Int	 maxVBRBitrate	  = config->GetIntValue(ConfigureLAME::ConfigID, "MaxVBRBitrate", 256);
	Bool	 copyrightBit	  = config->GetIntValue(ConfigureLAME::ConfigID, "Copyright", 0);
	Bool	 originalBit	  = config->GetIntValue(ConfigureLAME::ConfigID, "Original", 1);
	Bool	 privateBit	  = config->GetIntValue(ConfigureLAME::ConfigID, "Private", 0);
	Bool	 crc		  = config->GetIntValue(ConfigureLAME::ConfigID, "CRC", 0);
	Bool	 strictISO	  = config->GetIntValue(ConfigureLAME::ConfigID, "StrictISO", 0);
	Int	 stereoMode	  = config->GetIntValue(ConfigureLAME::ConfigID, "StereoMode", 0);
	Bool	 forceJS	  = config->GetIntValue(ConfigureLAME::ConfigID, "ForceJS", 0);
	Bool	 enableATH	  = config->GetIntValue(ConfigureLAME::ConfigID, "EnableATH", 1);
	Int	 athType	  = config->GetIntValue(ConfigureLAME::ConfigID, "ATHType", -1);
	Bool	 useTNS		  = config->GetIntValue(ConfigureLAME::ConfigID, "UseTNS", 1);
	Bool	 disableFiltering = config->GetIntValue(ConfigureLAME::ConfigID, "DisableFiltering", 0);
	Bool	 setLowpass	  = config->GetIntValue(ConfigureLAME::ConfigID, "SetLowpass", 0);
	Int	 lowpass	  = config->GetIntValue(ConfigureLAME::ConfigID, "Lowpass", 0);
	Bool	 setHighpass	  = config->GetIntValue(ConfigureLAME::ConfigID, "SetHighpass", 0);
	Int	 highpass	  = config->GetIntValue(ConfigureLAME::ConfigID, "Highpass", 0);
	Bool	 setLowpassWidth  = config->GetIntValue(ConfigureLAME::ConfigID, "SetLowpassWidth", 0);
	Int	 lowpassWidth	  = config->GetIntValue(ConfigureLAME::ConfigID, "LowpassWidth", 0);
	Bool	 setHighpassWidth = config->GetIntValue(ConfigureLAME::ConfigID, "SetHighpassWidth", 0);
	Int	 highpassWidth	  = config->GetIntValue(ConfigureLAME::ConfigID, "HighpassWidth", 0);

	/* Create and configure LAME encoder.
	 */
	context = ex_lame_init();

	ex_lame_set_in_samplerate(context, format.rate);
	ex_lame_set_num_channels(context, format.channels);

	switch (preset)
	{
		case 0:
			ex_lame_set_copyright(context, copyrightBit);
			ex_lame_set_original(context, originalBit);
			ex_lame_set_extension(context, privateBit);
			ex_lame_set_error_protection(context, crc);
			ex_lame_set_strict_ISO(context, strictISO);

			/* Set bitrate.
			 */
			if (vbrMode == vbr_off)
			{
				if (setBitrate) ex_lame_set_brate(context, bitrate);
				else		ex_lame_set_compression_ratio(context, ratio / 100.0);
			}

			/* Set quality.
			 */
			if (setQuality) ex_lame_set_quality(context, quality);

			/* Set audio filtering.
			 */
			if (disableFiltering)
			{
				ex_lame_set_lowpassfreq(context, -1);
				ex_lame_set_highpassfreq(context, -1);
			}
			else
			{
				if (setLowpass)  ex_lame_set_lowpassfreq(context, lowpass);
				if (setHighpass) ex_lame_set_highpassfreq(context, highpass);

				if (setLowpass  && setLowpassWidth)  ex_lame_set_lowpasswidth(context, lowpassWidth);
				if (setHighpass && setHighpassWidth) ex_lame_set_highpasswidth(context, highpassWidth);
			}

			/* Set Stereo mode.
			 */
			if	(stereoMode == 1) ex_lame_set_mode(context, MONO);
			else if (stereoMode == 2) ex_lame_set_mode(context, STEREO);
			else if (stereoMode == 3) ex_lame_set_mode(context, JOINT_STEREO);
			else			  ex_lame_set_mode(context, NOT_SET);

			if (stereoMode == 3)
			{
				if (forceJS) ex_lame_set_force_ms(context, 1);
				else	     ex_lame_set_force_ms(context, 0);
			}

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

			/* Set ATH.
			 */
			if (enableATH)
			{
				if (athType != -1) ex_lame_set_ATHtype(context, athType);
			}
			else
			{
				ex_lame_set_noATH(context, 1);
			}

			/* Set TNS.
			 */
			ex_lame_set_useTemporal(context, useTNS);

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

	ex_lame_init_params(context);

	frameSize     = ex_lame_get_framesize(context);
	maxPacketSize = frameSize * 1.25 + 7200;
}

BoCA::SuperWorker::~SuperWorker()
{
	/* Destroy LAME encoder.
	 */
	ex_lame_close(context);
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

			if (samplesLeft > 0)
			{
				if (format.channels == 2) dataLength = ex_lame_encode_buffer_interleaved(context, samplesBuffer + framesProcessed * samplesPerFrame, Math::Min(samplesLeft / 2, frameSize), packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);
				else			  dataLength = ex_lame_encode_buffer(		 context, samplesBuffer,
														  samplesBuffer + framesProcessed * samplesPerFrame, Math::Min(samplesLeft,	frameSize), packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);
			}
			else
			{
				dataLength = ex_lame_encode_flush(context, packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);
			}

			packetBuffer.Resize(packetBuffer.Size() - maxPacketSize + dataLength);

			if (samplesLeft < 0 && dataLength == 0) break;

			if (dataLength > 0) packetSizes.Add(dataLength);

			framesProcessed++;
			samplesLeft -= samplesPerFrame;
		}

		/* Flush at the end of each block in parallel mode.
		 */
		if (overlap > 0)
		{
			packetBuffer.Resize(packetBuffer.Size() + maxPacketSize);

			Int	 dataLength = ex_lame_encode_flush_nogap(context, packetBuffer + packetBuffer.Size() - maxPacketSize, maxPacketSize);

			packetBuffer.Resize(packetBuffer.Size() - maxPacketSize + dataLength);

			if (dataLength > 0) packetSizes.Add(dataLength);
		}

		samplesBuffer.Resize(0);

		workerMutex.Release();

		process	= False;
	}

	return Success();
}

Void BoCA::SuperWorker::Encode(const Buffer<signed short> &buffer, Int offset, Int size, Bool last)
{
	workerMutex.Lock();

	samplesBuffer.Resize(size);

	memcpy(samplesBuffer, buffer + offset, size * sizeof(signed short));

	workerMutex.Release();

	flush	= last;
	process = True;
}

Void BoCA::SuperWorker::GetInfoTag(Buffer<UnsignedByte> &buffer) const
{
	buffer.Resize(2880); // Maximum frame size

	Int	 size = ex_lame_get_lametag_frame(context, buffer, buffer.Size());

	buffer.Resize(size);
}

Int BoCA::SuperWorker::Quit()
{
	quit = True;

	return Success();
}
