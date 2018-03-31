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

#ifndef H_LAMECONFIG
#define H_LAMECONFIG

#include <smooth.h>
#include <boca.h>

using namespace smooth;
using namespace smooth::GUI;

using namespace BoCA;

namespace BoCA
{
	class ConfigureLAME : public ConfigLayer
	{
		private:
			TabWidget		*reg_register;

			Layer			*register_layer_basic;
			Layer			*register_layer_misc;
			Layer			*register_layer_expert;
			Layer			*register_layer_filtering;

			GroupBox		*basic_preset;
			Text			*basic_text_preset;
			ComboBox		*basic_combo_preset;

			GroupBox		*basic_bitrate;
			OptionBox		*basic_option_set_bitrate;
			OptionBox		*basic_option_set_ratio;
			Slider			*basic_slider_bitrate;
			Text			*basic_text_bitrate;
			Text			*basic_text_ratio;
			EditBox			*basic_edit_ratio;

			GroupBox		*basic_quality;
			CheckBox		*basic_check_set_quality;
			Slider			*basic_slider_quality;
			Text			*basic_text_quality;
			Text			*basic_text_quality_better;
			Text			*basic_text_quality_worse;

			GroupBox		*vbr_vbrmode;
			OptionBox		*vbr_option_cbr;
			OptionBox		*vbr_option_abr;
			OptionBox		*vbr_option_vbrmtrh;

			GroupBox		*vbr_quality;
			Text			*vbr_text_setquality;
			Slider			*vbr_slider_quality;
			Text			*vbr_text_quality;
			Text			*vbr_text_quality_better;
			Text			*vbr_text_quality_worse;

			GroupBox		*vbr_abrbitrate;
			Slider			*vbr_slider_abrbitrate;
			EditBox			*vbr_edit_abrbitrate;
			Text			*vbr_text_abrbitrate_kbps;

			GroupBox		*misc_bits;
			CheckBox		*misc_check_original;
			CheckBox		*misc_check_copyright;
			CheckBox		*misc_check_private;

			GroupBox		*misc_crc;
			CheckBox		*misc_check_crc;

			GroupBox		*misc_stereomode;
			ComboBox		*misc_combo_stereomode;
			CheckBox		*misc_check_forcejs;

			GroupBox		*misc_bitrate;
			CheckBox		*misc_check_set_min_brate;
			CheckBox		*misc_check_set_max_brate;
			Slider			*misc_slider_min_brate;
			Slider			*misc_slider_max_brate;
			Text			*misc_text_min_brate_kbps;
			Text			*misc_text_max_brate_kbps;

			GroupBox		*expert_ath;
			CheckBox		*expert_check_ath;
			ComboBox		*expert_combo_athtype;

			GroupBox		*expert_psycho;
			CheckBox		*expert_check_tempmask;

			GroupBox		*expert_format;
			CheckBox		*expert_check_iso;

			GroupBox		*filtering_lowpass;
			CheckBox		*filtering_set_lowpass;
			EditBox			*filtering_edit_lowpass;
			CheckBox		*filtering_set_lowpass_width;
			EditBox			*filtering_edit_lowpass_width;

			GroupBox		*filtering_highpass;
			CheckBox		*filtering_set_highpass;
			EditBox			*filtering_edit_highpass;
			CheckBox		*filtering_set_highpass_width;
			EditBox			*filtering_edit_highpass_width;

			GroupBox		*filtering_misc;
			CheckBox		*filtering_check_disable_all;

			Int			 preset;
			Int			 set_bitrate;
			Int			 bitrate;
			Int			 ratio;
			Bool			 set_quality;
			Int			 quality;
			Bool			 forcejs;
			Int			 vbrmode;
			Int			 vbrquality;
			Int			 abrbitrate;
			Bool			 set_min_vbr_brate;
			Bool			 set_max_vbr_brate;
			Int			 min_vbr_brate;
			Int			 max_vbr_brate;
			Bool			 set_original;
			Bool			 set_private;
			Bool			 set_copyright;
			Bool			 set_crc;
			Bool			 set_iso;
			Bool			 enable_ath;
			Bool			 enable_tempmask;
			Bool			 disable_filtering;
			Bool			 set_lowpass;
			Bool			 set_lowpass_width;
			Bool			 set_highpass;
			Bool			 set_highpass_width;
		slots:
			Void			 SetPreset();
			Void			 SetBitrateOption();
			Void			 SetBitrate();
			Void			 SetQualityOption();
			Void			 SetQuality();
			Void			 SetStereoMode();
			Void			 SetVBRMode();
			Void			 SetVBRQuality();
			Void			 SetABRBitrate();
			Void			 SetABRBitrateByEditBox();
			Void			 SetMinVBRBitrateOption();
			Void			 SetMaxVBRBitrateOption();
			Void			 SetMinVBRBitrate();
			Void			 SetMaxVBRBitrate();
			Void			 SetEnableATH();
			Void			 SetDisableFiltering();
			Void			 SetLowpass();
			Void			 SetLowpassWidth();
			Void			 SetHighpass();
			Void			 SetHighpassWidth();
			Int			 GetBitrate();
			Int			 GetSliderValue();
			Int			 GetMinVBRBitrate();
			Int			 GetMinVBRSliderValue();
			Int			 GetMaxVBRBitrate();
			Int			 GetMaxVBRSliderValue();
			Int			 SliderValueToBitrate(Int);
			Int			 BitrateToSliderValue(Int);
		public:
			static const String	 ConfigID;

						 ConfigureLAME();
						~ConfigureLAME();

			Int			 SaveSettings();
	};
};

#endif
