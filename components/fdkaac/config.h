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

#ifndef H_FDKAACCONFIG
#define H_FDKAACCONFIG

#include <smooth.h>
#include <boca.h>

using namespace smooth;
using namespace smooth::GUI;

using namespace BoCA;

namespace BoCA
{
	class ConfigureFDKAAC : public ConfigLayer
	{
		private:
			TabWidget		*tabwidget;

			Layer			*layer_format;

			GroupBox		*group_version;
			OptionBox		*option_version_mpeg2;
			OptionBox		*option_version_mpeg4;

			GroupBox		*group_aactype;
			OptionBox		*option_aactype_low;
			OptionBox		*option_aactype_he;
			OptionBox		*option_aactype_hev2;
			OptionBox		*option_aactype_ld;
			OptionBox		*option_aactype_eld;

			GroupBox		*group_id3v2;
			CheckBox		*check_id3v2;
			Text			*text_note;
			Text			*text_id3v2;

			GroupBox		*group_mp4;
			OptionBox		*option_mp4;
			OptionBox		*option_aac;

			GroupBox		*group_extension;
			OptionBox		*option_extension_m4a;
			OptionBox		*option_extension_m4b;
			OptionBox		*option_extension_m4r;
			OptionBox		*option_extension_mp4;

			Layer			*layer_quality;

			GroupBox		*group_bitrate;
			Text			*text_bitrate;
			Slider			*slider_bitrate;
			EditBox			*edit_bitrate;
			Text			*text_bitrate_kbps;

			Int			 mpegVersion;
			Int			 aacType;
			Int			 bitrate;
			Bool			 allowID3;
			Int			 fileFormat;
			Int			 fileExtension;
		slots:
			Void			 SetMPEGVersion();
			Void			 SetObjectType();
			Void			 SetBitrate();
			Void			 SetBitrateByEditBox();
			Void			 SetFileFormat();
		public:
			static const String	 ConfigID;

						 ConfigureFDKAAC();
						~ConfigureFDKAAC();

			Int			 SaveSettings();
	};
};

#endif
