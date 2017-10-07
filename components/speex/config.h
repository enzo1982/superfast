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

#ifndef H_SPEEXCONFIG
#define H_SPEEXCONFIG

#include <smooth.h>
#include <boca.h>

using namespace smooth;
using namespace smooth::GUI;

using namespace BoCA;

namespace BoCA
{
	class ConfigureSpeex : public ConfigLayer
	{
		private:
			GroupBox		*group_profile;
			Text			*text_profile;
			ComboBox		*combo_profile;

			GroupBox		*group_vbr_mode;
			OptionBox		*option_cbr;
			OptionBox		*option_vbr;
			OptionBox		*option_abr;

			GroupBox		*group_cbr_quality;
			OptionBox		*option_cbr_quality;
			Slider			*slider_cbr_quality;
			Text			*text_cbr_quality_value;
			OptionBox		*option_cbr_bitrate;
			Slider			*slider_cbr_bitrate;
			Text			*text_cbr_bitrate_value;

			GroupBox		*group_vbr_quality;
			Text			*text_vbr_quality;
			Slider			*slider_vbr_quality;
			Text			*text_vbr_quality_value;
			CheckBox		*check_vbr_bitrate;
			Slider			*slider_vbr_bitrate;
			Text			*text_vbr_bitrate_value;

			GroupBox		*group_abr_bitrate;
			Text			*text_abr_bitrate;
			Slider			*slider_abr_bitrate;
			Text			*text_abr_bitrate_value;

			GroupBox		*group_options;
			CheckBox		*check_vad;
			CheckBox		*check_dtx;

			GroupBox		*group_complexity;
			Text			*text_complexity;
			Slider			*slider_complexity;
			Text			*text_complexity_value;

			Int			 cbrmode;
			Int			 quality;
			Int			 bitrate;

			Int			 complexity;

			Int			 vbrmode;
			Int			 vbrq;

			Int			 vbrmax;
			Bool			 use_vbrmax;

			Int			 abr;

			Bool			 use_vad;
			Bool			 use_dtx;
		slots:
			Void			 SetVBRMode();
			Void			 SetVBRQuality();

			Void			 ToggleVBRBitrate();
			Void			 SetVBRBitrate();

			Void			 SetCBRMode();
			Void			 SetQuality();
			Void			 SetBitrate();

			Void			 SetABRBitrate();

			Void			 SetComplexity();

			Void			 SetVAD();
		public:
			static const String	 ConfigID;

						 ConfigureSpeex();
						~ConfigureSpeex();

			Int			 SaveSettings();
	};
};

#endif
