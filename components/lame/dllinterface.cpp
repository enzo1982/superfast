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

#include <boca.h>
#include "dllinterface.h"

using namespace BoCA;

LAME_INIT			 ex_lame_init				= NIL;
LAME_SET_PRESET			 ex_lame_set_preset			= NIL;
LAME_SET_IN_SAMPLERATE		 ex_lame_set_in_samplerate		= NIL;
LAME_SET_NUM_CHANNELS		 ex_lame_set_num_channels		= NIL;
LAME_SET_COPYRIGHT		 ex_lame_set_copyright			= NIL;
LAME_SET_ORIGINAL		 ex_lame_set_original			= NIL;
LAME_SET_EXTENSION		 ex_lame_set_extension			= NIL;
LAME_SET_ERROR_PROTECTION	 ex_lame_set_error_protection		= NIL;
LAME_SET_STRICT_ISO		 ex_lame_set_strict_ISO			= NIL;
LAME_SET_BRATE			 ex_lame_set_brate			= NIL;
LAME_SET_COMPRESSION_RATIO	 ex_lame_set_compression_ratio		= NIL;
LAME_SET_QUALITY		 ex_lame_set_quality			= NIL;
LAME_SET_LOWPASSFREQ		 ex_lame_set_lowpassfreq		= NIL;
LAME_SET_HIGHPASSFREQ		 ex_lame_set_highpassfreq		= NIL;
LAME_SET_LOWPASSWIDTH		 ex_lame_set_lowpasswidth		= NIL;
LAME_SET_HIGHPASSWIDTH		 ex_lame_set_highpasswidth		= NIL;
LAME_SET_MODE			 ex_lame_set_mode			= NIL;
LAME_SET_FORCE_MS		 ex_lame_set_force_ms			= NIL;
LAME_CLOSE			 ex_lame_close				= NIL;
LAME_SET_VBR			 ex_lame_set_VBR			= NIL;
LAME_SET_VBR_QUALITY		 ex_lame_set_VBR_quality		= NIL;
LAME_SET_VBR_MEAN_BITRATE_KBPS	 ex_lame_set_VBR_mean_bitrate_kbps	= NIL;
LAME_SET_VBR_MIN_BITRATE_KBPS	 ex_lame_set_VBR_min_bitrate_kbps	= NIL;
LAME_SET_VBR_MAX_BITRATE_KBPS	 ex_lame_set_VBR_max_bitrate_kbps	= NIL;
LAME_SET_NOATH			 ex_lame_set_noATH			= NIL;
LAME_SET_ATHTYPE		 ex_lame_set_ATHtype			= NIL;
LAME_SET_USETEMPORAL		 ex_lame_set_useTemporal		= NIL;
LAME_INIT_PARAMS		 ex_lame_init_params			= NIL;
LAME_GET_FRAMESIZE		 ex_lame_get_framesize			= NIL;
LAME_ENCODE_BUFFER		 ex_lame_encode_buffer			= NIL;
LAME_ENCODE_BUFFER_INTERLEAVED	 ex_lame_encode_buffer_interleaved	= NIL;
LAME_ENCODE_FLUSH		 ex_lame_encode_flush			= NIL;
LAME_ENCODE_FLUSH_NOGAP		 ex_lame_encode_flush_nogap		= NIL;
GET_LAME_SHORT_VERSION		 ex_get_lame_short_version		= NIL;
LAME_GET_LAMETAG_FRAME		 ex_lame_get_lametag_frame		= NIL;
LAME_SET_BWRITEVBRTAG		 ex_lame_set_bWriteVbrTag		= NIL;

DynamicLoader *lamedll	= NIL;

Bool LoadLAMEDLL()
{
#ifdef __WIN32__
	lamedll = BoCA::Utilities::LoadCodecDLL("LAME");
#else
	lamedll = BoCA::Utilities::LoadCodecDLL("mp3lame");
#endif

	if (lamedll == NIL) return False;

	ex_lame_init				= (LAME_INIT) lamedll->GetFunctionAddress("lame_init");
	ex_lame_set_preset			= (LAME_SET_PRESET) lamedll->GetFunctionAddress("lame_set_preset");
	ex_lame_set_in_samplerate		= (LAME_SET_IN_SAMPLERATE) lamedll->GetFunctionAddress("lame_set_in_samplerate");
	ex_lame_set_num_channels		= (LAME_SET_NUM_CHANNELS) lamedll->GetFunctionAddress("lame_set_num_channels");
	ex_lame_set_copyright			= (LAME_SET_COPYRIGHT) lamedll->GetFunctionAddress("lame_set_copyright");
	ex_lame_set_original			= (LAME_SET_ORIGINAL) lamedll->GetFunctionAddress("lame_set_original");
	ex_lame_set_extension			= (LAME_SET_EXTENSION) lamedll->GetFunctionAddress("lame_set_extension");
	ex_lame_set_error_protection		= (LAME_SET_ERROR_PROTECTION) lamedll->GetFunctionAddress("lame_set_error_protection");
	ex_lame_set_strict_ISO			= (LAME_SET_STRICT_ISO) lamedll->GetFunctionAddress("lame_set_strict_ISO");
	ex_lame_set_brate			= (LAME_SET_BRATE) lamedll->GetFunctionAddress("lame_set_brate");
	ex_lame_set_compression_ratio		= (LAME_SET_COMPRESSION_RATIO) lamedll->GetFunctionAddress("lame_set_compression_ratio");
	ex_lame_set_quality			= (LAME_SET_QUALITY) lamedll->GetFunctionAddress("lame_set_quality");
	ex_lame_set_lowpassfreq			= (LAME_SET_LOWPASSFREQ) lamedll->GetFunctionAddress("lame_set_lowpassfreq");
	ex_lame_set_highpassfreq		= (LAME_SET_HIGHPASSFREQ) lamedll->GetFunctionAddress("lame_set_highpassfreq");
	ex_lame_set_lowpasswidth		= (LAME_SET_LOWPASSWIDTH) lamedll->GetFunctionAddress("lame_set_lowpasswidth");
	ex_lame_set_highpasswidth		= (LAME_SET_HIGHPASSWIDTH) lamedll->GetFunctionAddress("lame_set_highpasswidth");
	ex_lame_set_mode			= (LAME_SET_MODE) lamedll->GetFunctionAddress("lame_set_mode");
	ex_lame_set_force_ms			= (LAME_SET_FORCE_MS) lamedll->GetFunctionAddress("lame_set_force_ms");
	ex_lame_close				= (LAME_CLOSE) lamedll->GetFunctionAddress("lame_close");
	ex_lame_set_VBR				= (LAME_SET_VBR) lamedll->GetFunctionAddress("lame_set_VBR");
	ex_lame_set_VBR_quality			= (LAME_SET_VBR_QUALITY) lamedll->GetFunctionAddress("lame_set_VBR_quality");
	ex_lame_set_VBR_mean_bitrate_kbps	= (LAME_SET_VBR_MEAN_BITRATE_KBPS) lamedll->GetFunctionAddress("lame_set_VBR_mean_bitrate_kbps");
	ex_lame_set_VBR_min_bitrate_kbps	= (LAME_SET_VBR_MIN_BITRATE_KBPS) lamedll->GetFunctionAddress("lame_set_VBR_min_bitrate_kbps");
	ex_lame_set_VBR_max_bitrate_kbps	= (LAME_SET_VBR_MAX_BITRATE_KBPS) lamedll->GetFunctionAddress("lame_set_VBR_max_bitrate_kbps");
	ex_lame_set_noATH			= (LAME_SET_NOATH) lamedll->GetFunctionAddress("lame_set_noATH");
	ex_lame_set_ATHtype			= (LAME_SET_ATHTYPE) lamedll->GetFunctionAddress("lame_set_ATHtype");
	ex_lame_set_useTemporal			= (LAME_SET_USETEMPORAL) lamedll->GetFunctionAddress("lame_set_useTemporal");
	ex_lame_init_params			= (LAME_INIT_PARAMS) lamedll->GetFunctionAddress("lame_init_params");
	ex_lame_get_framesize			= (LAME_GET_FRAMESIZE) lamedll->GetFunctionAddress("lame_get_framesize");
	ex_lame_encode_buffer			= (LAME_ENCODE_BUFFER) lamedll->GetFunctionAddress("lame_encode_buffer");
	ex_lame_encode_buffer_interleaved	= (LAME_ENCODE_BUFFER_INTERLEAVED) lamedll->GetFunctionAddress("lame_encode_buffer_interleaved");
	ex_lame_encode_flush			= (LAME_ENCODE_FLUSH) lamedll->GetFunctionAddress("lame_encode_flush");
	ex_lame_encode_flush_nogap		= (LAME_ENCODE_FLUSH_NOGAP) lamedll->GetFunctionAddress("lame_encode_flush_nogap");
	ex_get_lame_short_version		= (GET_LAME_SHORT_VERSION) lamedll->GetFunctionAddress("get_lame_short_version");
	ex_lame_get_lametag_frame		= (LAME_GET_LAMETAG_FRAME) lamedll->GetFunctionAddress("lame_get_lametag_frame");
	ex_lame_set_bWriteVbrTag		= (LAME_SET_BWRITEVBRTAG) lamedll->GetFunctionAddress("lame_set_bWriteVbrTag");

	if (ex_lame_init			== NIL ||
	    ex_lame_set_preset			== NIL ||
	    ex_lame_set_in_samplerate		== NIL ||
	    ex_lame_set_num_channels		== NIL ||
	    ex_lame_set_copyright		== NIL ||
	    ex_lame_set_original		== NIL ||
	    ex_lame_set_extension		== NIL ||
	    ex_lame_set_error_protection	== NIL ||
	    ex_lame_set_strict_ISO		== NIL ||
	    ex_lame_set_brate			== NIL ||
	    ex_lame_set_compression_ratio	== NIL ||
	    ex_lame_set_quality			== NIL ||
	    ex_lame_set_lowpassfreq		== NIL ||
	    ex_lame_set_highpassfreq		== NIL ||
	    ex_lame_set_lowpasswidth		== NIL ||
	    ex_lame_set_highpasswidth		== NIL ||
	    ex_lame_set_mode			== NIL ||
	    ex_lame_set_force_ms		== NIL ||
	    ex_lame_close			== NIL ||
	    ex_lame_set_VBR			== NIL ||
	    ex_lame_set_VBR_quality		== NIL ||
	    ex_lame_set_VBR_mean_bitrate_kbps	== NIL ||
	    ex_lame_set_VBR_min_bitrate_kbps	== NIL ||
	    ex_lame_set_VBR_max_bitrate_kbps	== NIL ||
	    ex_lame_set_noATH			== NIL ||
	    ex_lame_set_ATHtype			== NIL ||
	    ex_lame_set_useTemporal		== NIL ||
	    ex_lame_init_params			== NIL ||
	    ex_lame_get_framesize		== NIL ||
	    ex_lame_encode_buffer		== NIL ||
	    ex_lame_encode_buffer_interleaved	== NIL ||
	    ex_lame_encode_flush		== NIL ||
	    ex_lame_encode_flush_nogap		== NIL ||
	    ex_get_lame_short_version		== NIL ||
	    ex_lame_get_lametag_frame		== NIL ||
	    ex_lame_set_bWriteVbrTag		== NIL) { FreeLAMEDLL(); return False; }

	return True;
}

Void FreeLAMEDLL()
{
	BoCA::Utilities::FreeCodecDLL(lamedll);

	lamedll = NIL;
}
