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

#include "lame/lame.h"

using namespace smooth;
using namespace smooth::System;

extern DynamicLoader	*lamedll;

Bool			 LoadLAMEDLL();
Void			 FreeLAMEDLL();

typedef lame_t				(CDECL *LAME_INIT)			();
typedef int				(CDECL *LAME_SET_PRESET)		(lame_t, int);
typedef int				(CDECL *LAME_SET_IN_SAMPLERATE)		(lame_t, int);
typedef int				(CDECL *LAME_SET_NUM_CHANNELS)		(lame_t, int);
typedef int				(CDECL *LAME_SET_COPYRIGHT)		(lame_t, int);
typedef int				(CDECL *LAME_SET_ORIGINAL)		(lame_t, int);
typedef int				(CDECL *LAME_SET_EXTENSION)		(lame_t, int);
typedef int				(CDECL *LAME_SET_ERROR_PROTECTION)	(lame_t, int);
typedef int				(CDECL *LAME_SET_STRICT_ISO)		(lame_t, int);
typedef int				(CDECL *LAME_SET_BRATE)			(lame_t, int);
typedef int				(CDECL *LAME_SET_COMPRESSION_RATIO)	(lame_t, float);
typedef int				(CDECL *LAME_SET_QUALITY)		(lame_t, int);
typedef int				(CDECL *LAME_SET_LOWPASSFREQ)		(lame_t, int);
typedef int				(CDECL *LAME_SET_HIGHPASSFREQ)		(lame_t, int);
typedef int				(CDECL *LAME_SET_LOWPASSWIDTH)		(lame_t, int);
typedef int				(CDECL *LAME_SET_HIGHPASSWIDTH)		(lame_t, int);
typedef int				(CDECL *LAME_SET_MODE)			(lame_t, MPEG_mode);
typedef int				(CDECL *LAME_SET_FORCE_MS)		(lame_t, int);
typedef int				(CDECL *LAME_CLOSE)			(lame_t);
typedef int				(CDECL *LAME_SET_VBR)			(lame_t, vbr_mode);
typedef int				(CDECL *LAME_SET_VBR_QUALITY)		(lame_t, float);
typedef int				(CDECL *LAME_SET_VBR_MEAN_BITRATE_KBPS)	(lame_t, int);
typedef int				(CDECL *LAME_SET_VBR_MIN_BITRATE_KBPS)	(lame_t, int);
typedef int				(CDECL *LAME_SET_VBR_MAX_BITRATE_KBPS)	(lame_t, int);
typedef int				(CDECL *LAME_SET_NOATH)			(lame_t, int);
typedef int				(CDECL *LAME_SET_ATHTYPE)		(lame_t, int);
typedef int				(CDECL *LAME_SET_USETEMPORAL)		(lame_t, int);
typedef int				(CDECL *LAME_INIT_PARAMS)		(lame_t const);
typedef int				(CDECL *LAME_GET_FRAMESIZE)		(lame_t const);
typedef int				(CDECL *LAME_ENCODE_BUFFER)		(lame_t, const short int [], const short int [], const int, unsigned char *, const int);
typedef int				(CDECL *LAME_ENCODE_BUFFER_INTERLEAVED)	(lame_t, short int [], int, unsigned char *, int);
typedef int				(CDECL *LAME_ENCODE_FLUSH)		(lame_t, unsigned char *, int);
typedef int				(CDECL *LAME_ENCODE_FLUSH_NOGAP)	(lame_t, unsigned char *, int);
typedef char *				(CDECL *GET_LAME_SHORT_VERSION)		();
typedef size_t				(CDECL *LAME_GET_LAMETAG_FRAME)		(lame_t, unsigned char *, size_t);
typedef int				(CDECL *LAME_SET_BWRITEVBRTAG)		(lame_t, int);

extern LAME_INIT			 ex_lame_init;
extern LAME_SET_PRESET			 ex_lame_set_preset;
extern LAME_SET_IN_SAMPLERATE		 ex_lame_set_in_samplerate;
extern LAME_SET_NUM_CHANNELS		 ex_lame_set_num_channels;
extern LAME_SET_COPYRIGHT		 ex_lame_set_copyright;
extern LAME_SET_ORIGINAL		 ex_lame_set_original;
extern LAME_SET_EXTENSION		 ex_lame_set_extension;
extern LAME_SET_ERROR_PROTECTION	 ex_lame_set_error_protection;
extern LAME_SET_STRICT_ISO		 ex_lame_set_strict_ISO;
extern LAME_SET_BRATE			 ex_lame_set_brate;
extern LAME_SET_COMPRESSION_RATIO	 ex_lame_set_compression_ratio;
extern LAME_SET_QUALITY			 ex_lame_set_quality;
extern LAME_SET_LOWPASSFREQ		 ex_lame_set_lowpassfreq;
extern LAME_SET_HIGHPASSFREQ		 ex_lame_set_highpassfreq;
extern LAME_SET_LOWPASSWIDTH		 ex_lame_set_lowpasswidth;
extern LAME_SET_HIGHPASSWIDTH		 ex_lame_set_highpasswidth;
extern LAME_SET_MODE			 ex_lame_set_mode;
extern LAME_SET_FORCE_MS		 ex_lame_set_force_ms;
extern LAME_CLOSE			 ex_lame_close;
extern LAME_SET_VBR			 ex_lame_set_VBR;
extern LAME_SET_VBR_QUALITY		 ex_lame_set_VBR_quality;
extern LAME_SET_VBR_MEAN_BITRATE_KBPS	 ex_lame_set_VBR_mean_bitrate_kbps;
extern LAME_SET_VBR_MIN_BITRATE_KBPS	 ex_lame_set_VBR_min_bitrate_kbps;
extern LAME_SET_VBR_MAX_BITRATE_KBPS	 ex_lame_set_VBR_max_bitrate_kbps;
extern LAME_SET_NOATH			 ex_lame_set_noATH;
extern LAME_SET_ATHTYPE			 ex_lame_set_ATHtype;
extern LAME_SET_USETEMPORAL		 ex_lame_set_useTemporal;
extern LAME_INIT_PARAMS			 ex_lame_init_params;
extern LAME_GET_FRAMESIZE		 ex_lame_get_framesize;
extern LAME_ENCODE_BUFFER		 ex_lame_encode_buffer;
extern LAME_ENCODE_BUFFER_INTERLEAVED	 ex_lame_encode_buffer_interleaved;
extern LAME_ENCODE_FLUSH		 ex_lame_encode_flush;
extern LAME_ENCODE_FLUSH_NOGAP		 ex_lame_encode_flush_nogap;
extern GET_LAME_SHORT_VERSION		 ex_get_lame_short_version;
extern LAME_GET_LAMETAG_FRAME		 ex_lame_get_lametag_frame;
extern LAME_SET_BWRITEVBRTAG		 ex_lame_set_bWriteVbrTag;
