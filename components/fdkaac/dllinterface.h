 /* BoCA - BonkEnc Component Architecture
  * Copyright (C) 2007-2015 Robert Kausch <robert.kausch@freac.org>
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

#include "fdk-aac/aacenc_lib.h"
#include "mp4v2/mp4v2.h"

using namespace smooth;
using namespace smooth::System;

extern DynamicLoader	*fdkaacdll;
extern DynamicLoader	*mp4v2dll;

Bool			 LoadFDKAACDLL();
Void			 FreeFDKAACDLL();

Bool			 LoadMP4v2DLL();
Void			 FreeMP4v2DLL();

typedef AACENC_ERROR			(*AACENCOPEN)			(HANDLE_AACENCODER *, const UINT, const UINT);
typedef AACENC_ERROR			(*AACENCCLOSE)			(HANDLE_AACENCODER *);
typedef AACENC_ERROR			(*AACENCENCODE)			(const HANDLE_AACENCODER, const AACENC_BufDesc *, const AACENC_BufDesc *, const AACENC_InArgs *, AACENC_OutArgs *);
typedef AACENC_ERROR			(*AACENCINFO)			(const HANDLE_AACENCODER, AACENC_InfoStruct *);
typedef AACENC_ERROR			(*AACENCODER_SETPARAM)		(const HANDLE_AACENCODER, const AACENC_PARAM, const UINT);

extern AACENCOPEN			 ex_aacEncOpen;
extern AACENCCLOSE			 ex_aacEncClose;
extern AACENCENCODE			 ex_aacEncEncode;
extern AACENCINFO			 ex_aacEncInfo;
extern AACENCODER_SETPARAM		 ex_aacEncoder_SetParam;

typedef MP4FileHandle			(*MP4CREATEEX)			(const char *, uint32_t, int, int, char *, uint32_t, char **, uint32_t);
typedef void				(*MP4CLOSE)			(MP4FileHandle, uint32_t);
typedef bool				(*MP4OPTIMIZE)			(const char *, const char *);

typedef bool				(*MP4SETTRACKESCONFIGURATION)	(MP4FileHandle, MP4TrackId, const uint8_t *, uint32_t);
typedef void				(*MP4SETAUDIOPROFILELEVEL)	(MP4FileHandle, uint8_t);
typedef MP4TrackId			(*MP4ADDAUDIOTRACK)		(MP4FileHandle, uint32_t, MP4Duration, uint8_t);
typedef bool				(*MP4WRITESAMPLE)		(MP4FileHandle, MP4TrackId, const uint8_t *, uint32_t, MP4Duration, MP4Duration, bool);

typedef MP4ItmfItem *			(*MP4ITMFITEMALLOC)		(const char *, uint32_t);
typedef void				(*MP4ITMFITEMFREE)		(MP4ItmfItem *);
typedef bool				(*MP4ITMFADDITEM)		(MP4FileHandle, const MP4ItmfItem *);

extern MP4CREATEEX			 ex_MP4CreateEx;
extern MP4CLOSE				 ex_MP4Close;
extern MP4OPTIMIZE			 ex_MP4Optimize;

extern MP4SETTRACKESCONFIGURATION	 ex_MP4SetTrackESConfiguration;
extern MP4SETAUDIOPROFILELEVEL		 ex_MP4SetAudioProfileLevel;
extern MP4ADDAUDIOTRACK			 ex_MP4AddAudioTrack;
extern MP4WRITESAMPLE			 ex_MP4WriteSample;

extern MP4ITMFITEMALLOC			 ex_MP4ItmfItemAlloc;
extern MP4ITMFITEMFREE			 ex_MP4ItmfItemFree;
extern MP4ITMFADDITEM			 ex_MP4ItmfAddItem;
