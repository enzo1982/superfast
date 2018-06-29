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

#include "config.h"

#include "lame/lame.h"

const String	 BoCA::ConfigureLAME::ConfigID = "LAME";

BoCA::ConfigureLAME::ConfigureLAME()
{
	const Config	*config = Config::Get();

	preset			= config->GetIntValue(ConfigID, "Preset", 2);
	set_bitrate		= config->GetIntValue(ConfigID, "SetBitrate", 1);
	bitrate			= GetSliderValue();
	ratio			= config->GetIntValue(ConfigID, "Ratio", 1100);
	set_quality		= config->GetIntValue(ConfigID, "SetQuality", 0);
	quality			= 9 - config->GetIntValue(ConfigID, "Quality", 3);
	forcejs			= config->GetIntValue(ConfigID, "ForceJS", 0);
	vbrmode			= config->GetIntValue(ConfigID, "VBRMode", 4);
	vbrquality		= 99 - config->GetIntValue(ConfigID, "VBRQuality", 50);
	abrbitrate		= config->GetIntValue(ConfigID, "ABRBitrate", 192);
	set_min_vbr_brate	= config->GetIntValue(ConfigID, "SetMinVBRBitrate", 0);
	min_vbr_brate		= GetMinVBRSliderValue();
	set_max_vbr_brate	= config->GetIntValue(ConfigID, "SetMaxVBRBitrate", 0);
	max_vbr_brate		= GetMaxVBRSliderValue();
	set_original		= config->GetIntValue(ConfigID, "Original", 1);
	set_copyright		= config->GetIntValue(ConfigID, "Copyright", 0);
	set_private		= config->GetIntValue(ConfigID, "Private", 0);
	set_crc			= config->GetIntValue(ConfigID, "CRC", 0);
	set_iso			= config->GetIntValue(ConfigID, "StrictISO", 0);
	disable_filtering	= config->GetIntValue(ConfigID, "DisableFiltering", 0);
	set_lowpass		= config->GetIntValue(ConfigID, "SetLowpass", 0);
	set_lowpass_width	= config->GetIntValue(ConfigID, "SetLowpassWidth", 0);
	set_highpass		= config->GetIntValue(ConfigID, "SetHighpass", 0);
	set_highpass_width	= config->GetIntValue(ConfigID, "SetHighpassWidth", 0);
	enable_ath		= config->GetIntValue(ConfigID, "EnableATH", 1);
	enable_tempmask		= config->GetIntValue(ConfigID, "UseTNS", 1);

	I18n	*i18n = I18n::Get();

	i18n->SetContext("Encoders::LAME");

	register_layer_basic		= new Layer(i18n->TranslateString("Basic"));
	register_layer_misc		= new Layer(i18n->TranslateString("Misc"));
	register_layer_expert		= new Layer(i18n->TranslateString("Expert"));
	register_layer_filtering	= new Layer(i18n->TranslateString("Audio processing"));

	i18n->SetContext("Encoders::LAME::Basic");

	reg_register			= new TabWidget(Point(7, 7), Size(433, 218));

	basic_preset			= new GroupBox(i18n->TranslateString("Presets"), Point(7, 11), Size(415, 39));

	basic_text_preset		= new Text(i18n->AddColon(i18n->TranslateString("Use preset")), Point(9, 13));

	basic_combo_preset		= new ComboBox(Point(17 + basic_text_preset->GetUnscaledTextWidth(), 10), Size(388 - basic_text_preset->GetUnscaledTextWidth(), 0));
	basic_combo_preset->AddEntry(i18n->TranslateString("Custom settings"));
	basic_combo_preset->AddEntry(i18n->TranslateString("Medium, Fast"));
	basic_combo_preset->AddEntry(i18n->TranslateString("Standard, Fast"));
	basic_combo_preset->AddEntry(i18n->TranslateString("Extreme, Fast"));
	basic_combo_preset->AddEntry("ABR");
	basic_combo_preset->SelectNthEntry(preset);
	basic_combo_preset->onSelectEntry.Connect(&ConfigureLAME::SetPreset, this);

	basic_preset->Add(basic_text_preset);
	basic_preset->Add(basic_combo_preset);

	basic_bitrate			= new GroupBox(i18n->TranslateString("Bitrate"), Point(142, 62), Size(280, 63));

	basic_option_set_bitrate	= new OptionBox(i18n->AddColon(i18n->TranslateString("Set bitrate")), Point(10, 11), Size(90, 0), &set_bitrate, 1);
	basic_option_set_bitrate->onAction.Connect(&ConfigureLAME::SetBitrateOption, this);

	basic_option_set_ratio		= new OptionBox(i18n->AddColon(i18n->TranslateString("Set ratio")), Point(10, 36), Size(90, 0), &set_bitrate, 0);
	basic_option_set_ratio->onAction.Connect(&ConfigureLAME::SetBitrateOption, this);

	basic_option_set_bitrate->SetWidth(Math::Max(basic_option_set_bitrate->GetUnscaledTextWidth(), basic_option_set_ratio->GetUnscaledTextWidth()) + 21);
	basic_option_set_ratio->SetWidth(Math::Max(basic_option_set_bitrate->GetUnscaledTextWidth(), basic_option_set_ratio->GetUnscaledTextWidth()) + 21);

	basic_slider_bitrate		= new Slider(Point(18 + basic_option_set_bitrate->GetWidth(), 11), Size(200 - basic_option_set_bitrate->GetWidth(), 0), OR_HORZ, &bitrate, 0, 17);
	basic_slider_bitrate->onValueChange.Connect(&ConfigureLAME::SetBitrate, this);

	basic_text_bitrate		= new Text(NIL, Point(226, 13));
	SetBitrate();

	basic_text_ratio		= new Text(i18n->IsActiveLanguageRightToLeft() ? ": 1" : "1 :", Point(18 + basic_option_set_bitrate->GetWidth(), 38));

	basic_edit_ratio		= new EditBox(String::FromFloat(((double) ratio) / 100), Point(basic_text_ratio->GetX() + 16, 35), Size(19, 0), 2);
	basic_edit_ratio->SetFlags(EDB_NUMERIC);

	basic_bitrate->Add(basic_option_set_bitrate);
	basic_bitrate->Add(basic_option_set_ratio);
	basic_bitrate->Add(basic_slider_bitrate);
	basic_bitrate->Add(basic_text_bitrate);
	basic_bitrate->Add(basic_text_ratio);
	basic_bitrate->Add(basic_edit_ratio);

	basic_quality			= new GroupBox(i18n->TranslateString("Quality"), Point(142, 137), Size(280, 51));

	basic_check_set_quality		= new CheckBox(i18n->AddColon(i18n->TranslateString("Set quality")), Point(10, 11), Size(90, 0), &set_quality);
	basic_check_set_quality->SetWidth(basic_check_set_quality->GetUnscaledTextWidth() + 21);
	basic_check_set_quality->onAction.Connect(&ConfigureLAME::SetQualityOption, this);

	basic_slider_quality		= new Slider(Point(18 + basic_check_set_quality->GetWidth(), 11), Size(238 - basic_check_set_quality->GetWidth(), 0), OR_HORZ, &quality, 0, 9);
	basic_slider_quality->onValueChange.Connect(&ConfigureLAME::SetQuality, this);

	basic_text_quality		= new Text(NIL, Point(264, 13));
	SetQuality();

	basic_text_quality_worse	= new Text(i18n->TranslateString("worse"), Point());
	basic_text_quality_worse->SetPosition(Point(basic_slider_quality->GetX() + 3 - (basic_text_quality_worse->GetUnscaledTextWidth() / 2), 30));

	basic_text_quality_better	= new Text(i18n->TranslateString("better"), Point());
	basic_text_quality_better->SetPosition(Point(251 - (basic_text_quality_better->GetUnscaledTextWidth() / 2), 30));

	basic_quality->Add(basic_check_set_quality);
	basic_quality->Add(basic_slider_quality);
	basic_quality->Add(basic_text_quality);
	basic_quality->Add(basic_text_quality_worse);
	basic_quality->Add(basic_text_quality_better);

	vbr_vbrmode			= new GroupBox(i18n->TranslateString("VBR mode"), Point(7, 62), Size(127, 88));

	vbr_option_vbrmtrh		= new OptionBox("VBR", Point(10, 11), Size(107, 0), &vbrmode, vbr_mtrh);
	vbr_option_vbrmtrh->onAction.Connect(&ConfigureLAME::SetVBRMode, this);

	vbr_option_abr			= new OptionBox("ABR", Point(10, 36), Size(107, 0), &vbrmode, vbr_abr);
	vbr_option_abr->onAction.Connect(&ConfigureLAME::SetVBRMode, this);

	vbr_option_cbr			= new OptionBox(String("CBR (").Append(i18n->TranslateString("no VBR")).Append(")"), Point(10, 61), Size(107, 0), &vbrmode, vbr_off);
	vbr_option_cbr->onAction.Connect(&ConfigureLAME::SetVBRMode, this);

	vbr_vbrmode->Add(vbr_option_cbr);
	vbr_vbrmode->Add(vbr_option_abr);
	vbr_vbrmode->Add(vbr_option_vbrmtrh);

	vbr_quality			= new GroupBox(i18n->TranslateString("VBR quality"), Point(142, 62), Size(280, 51));

	vbr_text_setquality		= new Text(i18n->AddColon(i18n->TranslateString("Quality")), Point(11, 13));

	vbr_slider_quality		= new Slider(Point(18 + vbr_text_setquality->GetUnscaledTextWidth(), 11), Size(229 - vbr_text_setquality->GetUnscaledTextWidth(), 0), OR_HORZ, &vbrquality, 0, 99);
	vbr_slider_quality->onValueChange.Connect(&ConfigureLAME::SetVBRQuality, this);

	vbr_text_quality		= new Text(NIL, Point(255, 13));

	vbr_text_quality_worse		= new Text(i18n->TranslateString("worse"), Point());
	vbr_text_quality_worse->SetPosition(Point(vbr_slider_quality->GetX() + 3 - (vbr_text_quality_worse->GetUnscaledTextWidth() / 2), 30));

	vbr_text_quality_better		= new Text(i18n->TranslateString("better"), Point());
	vbr_text_quality_better->SetPosition(Point(242 - (vbr_text_quality_better->GetUnscaledTextWidth() / 2), 30));

	SetVBRQuality();

	vbr_quality->Add(vbr_text_setquality);
	vbr_quality->Add(vbr_slider_quality);
	vbr_quality->Add(vbr_text_quality);
	vbr_quality->Add(vbr_text_quality_worse);
	vbr_quality->Add(vbr_text_quality_better);

	vbr_abrbitrate			= new GroupBox(i18n->TranslateString("ABR target bitrate"), Point(142, 62), Size(280, 39));

	vbr_slider_abrbitrate		= new Slider(Point(10, 11), Size(194, 0), OR_HORZ, &abrbitrate, 8, 320);
	vbr_slider_abrbitrate->onValueChange.Connect(&ConfigureLAME::SetABRBitrate, this);

	vbr_edit_abrbitrate		= new EditBox(NIL, Point(212, 10), Size(25, 0), 3);
	vbr_edit_abrbitrate->SetFlags(EDB_NUMERIC);
	vbr_edit_abrbitrate->onInput.Connect(&ConfigureLAME::SetABRBitrateByEditBox, this);

	vbr_text_abrbitrate_kbps	= new Text(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(244, 13));

	SetABRBitrate();

	vbr_abrbitrate->Add(vbr_slider_abrbitrate);
	vbr_abrbitrate->Add(vbr_edit_abrbitrate);
	vbr_abrbitrate->Add(vbr_text_abrbitrate_kbps);

	i18n->SetContext("Encoders::LAME::Misc");

	misc_bits			= new GroupBox(i18n->TranslateString("Control bits"), Point(7, 11), Size(138, 90));

	misc_check_copyright		= new CheckBox(i18n->TranslateString("Set Copyright bit"), Point(10, 11), Size(117, 0), &set_copyright);
	misc_check_original		= new CheckBox(i18n->TranslateString("Set Original bit"), Point(10, 36), Size(117, 0), &set_original);
	misc_check_private		= new CheckBox(i18n->TranslateString("Set Private bit"), Point(10, 61), Size(117, 0), &set_private);

	misc_bits->Add(misc_check_original);
	misc_bits->Add(misc_check_copyright);
	misc_bits->Add(misc_check_private);

	misc_crc			= new GroupBox(i18n->TranslateString("CRC"), Point(153, 62), Size(269, 39));

	misc_check_crc			= new CheckBox(i18n->TranslateString("Enable CRC"), Point(10, 11), Size(250, 0), &set_crc);

	misc_crc->Add(misc_check_crc);

	misc_stereomode			= new GroupBox(i18n->TranslateString("Stereo mode"), Point(153, 11), Size(269, 39));

	misc_combo_stereomode		= new ComboBox(Point(10, 10), Size(120, 0));
	misc_combo_stereomode->AddEntry(i18n->TranslateString("auto"));
	misc_combo_stereomode->AddEntry(i18n->TranslateString("Mono"));
	misc_combo_stereomode->AddEntry(i18n->TranslateString("Stereo"));
	misc_combo_stereomode->AddEntry(i18n->TranslateString("Joint Stereo"));
	misc_combo_stereomode->SelectNthEntry(config->GetIntValue(ConfigID, "StereoMode", 0));
	misc_combo_stereomode->onSelectEntry.Connect(&ConfigureLAME::SetStereoMode, this);

	misc_check_forcejs		= new CheckBox(i18n->TranslateString("Force Joint Stereo"), Point(137, 11), Size(121, 0), &forcejs);

	misc_stereomode->Add(misc_combo_stereomode);
	misc_stereomode->Add(misc_check_forcejs);

	misc_bitrate			= new GroupBox(i18n->TranslateString("VBR bitrate range"), Point(7, 113), Size(415, 63));

	misc_check_set_min_brate	= new CheckBox(i18n->AddColon(i18n->TranslateString("Set minimum VBR bitrate")), Point(10, 11), Size(176, 0), &set_min_vbr_brate);
	misc_check_set_min_brate->onAction.Connect(&ConfigureLAME::SetMinVBRBitrateOption, this);

	misc_check_set_max_brate	= new CheckBox(i18n->AddColon(i18n->TranslateString("Set maximum VBR bitrate")), Point(10, 36), Size(176, 0), &set_max_vbr_brate);
	misc_check_set_max_brate->onAction.Connect(&ConfigureLAME::SetMaxVBRBitrateOption, this);

	misc_check_set_min_brate->SetWidth(Math::Max(misc_check_set_min_brate->GetUnscaledTextWidth(), misc_check_set_max_brate->GetUnscaledTextWidth()) + 21);
	misc_check_set_max_brate->SetWidth(Math::Max(misc_check_set_min_brate->GetUnscaledTextWidth(), misc_check_set_max_brate->GetUnscaledTextWidth()) + 21);

	misc_slider_min_brate		= new Slider(Point(18 + misc_check_set_min_brate->GetWidth(), 11), Size(332 - misc_check_set_min_brate->GetWidth(), 0), OR_HORZ, &min_vbr_brate, 0, 17);
	misc_slider_min_brate->onValueChange.Connect(&ConfigureLAME::SetMinVBRBitrate, this);

	misc_text_min_brate_kbps	= new Text(NIL, Point(358, 13));

	misc_slider_max_brate		= new Slider(Point(18 + misc_check_set_min_brate->GetWidth(), 36), Size(332 - misc_check_set_min_brate->GetWidth(), 0), OR_HORZ, &max_vbr_brate, 0, 17);
	misc_slider_max_brate->onValueChange.Connect(&ConfigureLAME::SetMaxVBRBitrate, this);

	misc_text_max_brate_kbps	= new Text(NIL, Point(358, 38));

	SetMinVBRBitrate();
	SetMaxVBRBitrate();
	SetVBRMode();

	misc_bitrate->Add(misc_check_set_min_brate);
	misc_bitrate->Add(misc_check_set_max_brate);
	misc_bitrate->Add(misc_slider_min_brate);
	misc_bitrate->Add(misc_slider_max_brate);
	misc_bitrate->Add(misc_text_min_brate_kbps);
	misc_bitrate->Add(misc_text_max_brate_kbps);

	i18n->SetContext("Encoders::LAME::Expert");

	expert_ath			= new GroupBox(i18n->TranslateString("ATH"), Point(7, 11), Size(415, 39));

	expert_check_ath		= new CheckBox(i18n->AddColon(i18n->TranslateString("Enable ATH")), Point(10, 11), Size(93, 0), &enable_ath);
	expert_check_ath->onAction.Connect(&ConfigureLAME::SetEnableATH, this);
	expert_check_ath->SetWidth(expert_check_ath->GetUnscaledTextWidth() + 19);

	expert_combo_athtype		= new ComboBox(Point(38 + expert_check_ath->GetUnscaledTextWidth(), 10), Size(367 - expert_check_ath->GetUnscaledTextWidth(), 0));
	expert_combo_athtype->AddEntry(i18n->TranslateString("Use default setting"));
	expert_combo_athtype->AddEntry("Gabriel Bouvigne, 9");
	expert_combo_athtype->AddEntry("Frank Klemm");
	expert_combo_athtype->AddEntry("Gabriel Bouvigne, 0");
	expert_combo_athtype->AddEntry("Roel Van Den Berghe");
	expert_combo_athtype->AddEntry("Gabriel Bouvigne VBR");
	expert_combo_athtype->AddEntry("John Dahlstrom");
	expert_combo_athtype->SelectNthEntry(config->GetIntValue(ConfigID, "ATHType", -1) + 1);

	if (!enable_ath) expert_combo_athtype->Deactivate();

	expert_ath->Add(expert_check_ath);
	expert_ath->Add(expert_combo_athtype);

	expert_psycho			= new GroupBox(i18n->TranslateString("Psycho acoustic model"), Point(7, 62), Size(415, 39));

	expert_check_tempmask		= new CheckBox(i18n->TranslateString("Use Temporal Masking Effect"), Point(10, 11), Size(394, 0), &enable_tempmask);

	expert_psycho->Add(expert_check_tempmask);

	expert_format			= new GroupBox(i18n->TranslateString("Stream format"), Point(7, 113), Size(415, 39));

	expert_check_iso		= new CheckBox(i18n->TranslateString("Enforce strict ISO compliance"), Point(10, 11), Size(394, 0), &set_iso);

	expert_format->Add(expert_check_iso);

	i18n->SetContext("Encoders::LAME::Audio processing");

	filtering_highpass		= new GroupBox(i18n->TranslateString("Highpass filter"), Point(7, 11), Size(246, 64));

	filtering_set_highpass		= new CheckBox(i18n->AddColon(i18n->TranslateString("Set Highpass frequency (Hz)")), Point(10, 11), Size(180, 0), &set_highpass);
	filtering_set_highpass->onAction.Connect(&ConfigureLAME::SetHighpass, this);

	filtering_edit_highpass		= new EditBox(String::FromInt(config->GetIntValue(ConfigID, "Highpass", 0)), Point(199, 10), Size(37, 0), 5);
	filtering_edit_highpass->SetFlags(EDB_NUMERIC);

	filtering_set_highpass_width	= new CheckBox(i18n->AddColon(i18n->TranslateString("Set Highpass width (Hz)")), Point(10, 36), Size(180, 0), &set_highpass_width);
	filtering_set_highpass_width->onAction.Connect(&ConfigureLAME::SetHighpassWidth, this);

	filtering_edit_highpass_width	= new EditBox(String::FromInt(config->GetIntValue(ConfigID, "HighpassWidth", 0)), Point(199, 35), Size(37, 0), 5);
	filtering_edit_highpass_width->SetFlags(EDB_NUMERIC);

	filtering_highpass->Add(filtering_set_highpass);
	filtering_highpass->Add(filtering_edit_highpass);
	filtering_highpass->Add(filtering_set_highpass_width);
	filtering_highpass->Add(filtering_edit_highpass_width);

	filtering_lowpass		= new GroupBox(i18n->TranslateString("Lowpass filter"), Point(7, 87), Size(246, 64));

	filtering_set_lowpass		= new CheckBox(i18n->AddColon(i18n->TranslateString("Set Lowpass frequency (Hz)")), Point(10, 11), Size(180, 0), &set_lowpass);
	filtering_set_lowpass->onAction.Connect(&ConfigureLAME::SetLowpass, this);

	filtering_edit_lowpass		= new EditBox(String::FromInt(config->GetIntValue(ConfigID, "Lowpass", 0)), Point(199, 10), Size(37, 0), 5);
	filtering_edit_lowpass->SetFlags(EDB_NUMERIC);

	filtering_set_lowpass_width	= new CheckBox(i18n->AddColon(i18n->TranslateString("Set Lowpass width (Hz)")), Point(10, 36), Size(180, 0), &set_lowpass_width);
	filtering_set_lowpass_width->onAction.Connect(&ConfigureLAME::SetLowpassWidth, this);

	filtering_edit_lowpass_width	= new EditBox(String::FromInt(config->GetIntValue(ConfigID, "LowpassWidth", 0)), Point(199, 35), Size(37, 0), 5);
	filtering_edit_lowpass_width->SetFlags(EDB_NUMERIC);

	filtering_lowpass->Add(filtering_set_lowpass);
	filtering_lowpass->Add(filtering_edit_lowpass);
	filtering_lowpass->Add(filtering_set_lowpass_width);
	filtering_lowpass->Add(filtering_edit_lowpass_width);

	filtering_misc			= new GroupBox(i18n->TranslateString("Misc settings"), Point(261, 11), Size(161, 39));

	filtering_check_disable_all	= new CheckBox(i18n->TranslateString("Disable all filtering"), Point(10, 11), Size(140, 0), &disable_filtering);
	filtering_check_disable_all->onAction.Connect(&ConfigureLAME::SetDisableFiltering, this);

	filtering_misc->Add(filtering_check_disable_all);

	SetPreset();

	Add(reg_register);

	reg_register->Add(register_layer_basic);
	reg_register->Add(register_layer_misc);
	reg_register->Add(register_layer_expert);
	reg_register->Add(register_layer_filtering);

	register_layer_basic->Add(basic_preset);
	register_layer_basic->Add(basic_bitrate);
	register_layer_basic->Add(basic_quality);

	register_layer_basic->Add(vbr_vbrmode);
	register_layer_basic->Add(vbr_quality);
	register_layer_basic->Add(vbr_abrbitrate);

	register_layer_misc->Add(misc_bits);
	register_layer_misc->Add(misc_crc);
	register_layer_misc->Add(misc_stereomode);
	register_layer_misc->Add(misc_bitrate);

	register_layer_expert->Add(expert_ath);
	register_layer_expert->Add(expert_psycho);
	register_layer_expert->Add(expert_format);

	register_layer_filtering->Add(filtering_lowpass);
	register_layer_filtering->Add(filtering_highpass);
	register_layer_filtering->Add(filtering_misc);

	SetSize(Size(447, 232));
}

BoCA::ConfigureLAME::~ConfigureLAME()
{
	DeleteObject(reg_register);
	DeleteObject(register_layer_basic);
	DeleteObject(register_layer_misc);
	DeleteObject(register_layer_expert);
	DeleteObject(register_layer_filtering);

	DeleteObject(basic_preset);
	DeleteObject(basic_text_preset);
	DeleteObject(basic_combo_preset);
	DeleteObject(basic_bitrate);
	DeleteObject(basic_option_set_bitrate);
	DeleteObject(basic_option_set_ratio);
	DeleteObject(basic_slider_bitrate);
	DeleteObject(basic_text_bitrate);
	DeleteObject(basic_text_ratio);
	DeleteObject(basic_edit_ratio);
	DeleteObject(basic_quality);
	DeleteObject(basic_check_set_quality);
	DeleteObject(basic_slider_quality);
	DeleteObject(basic_text_quality);
	DeleteObject(basic_text_quality_worse);
	DeleteObject(basic_text_quality_better);

	DeleteObject(vbr_vbrmode);
	DeleteObject(vbr_option_cbr);
	DeleteObject(vbr_option_abr);
	DeleteObject(vbr_option_vbrmtrh);
	DeleteObject(vbr_quality);
	DeleteObject(vbr_text_setquality);
	DeleteObject(vbr_slider_quality);
	DeleteObject(vbr_text_quality);
	DeleteObject(vbr_text_quality_worse);
	DeleteObject(vbr_text_quality_better);
	DeleteObject(vbr_abrbitrate);
	DeleteObject(vbr_slider_abrbitrate);
	DeleteObject(vbr_edit_abrbitrate);
	DeleteObject(vbr_text_abrbitrate_kbps);

	DeleteObject(misc_bits);
	DeleteObject(misc_check_original);
	DeleteObject(misc_check_copyright);
	DeleteObject(misc_check_private);
	DeleteObject(misc_crc);
	DeleteObject(misc_check_crc);
	DeleteObject(misc_stereomode);
	DeleteObject(misc_combo_stereomode);
	DeleteObject(misc_check_forcejs);
	DeleteObject(misc_bitrate);
	DeleteObject(misc_check_set_min_brate);
	DeleteObject(misc_check_set_max_brate);
	DeleteObject(misc_slider_min_brate);
	DeleteObject(misc_slider_max_brate);
	DeleteObject(misc_text_min_brate_kbps);
	DeleteObject(misc_text_max_brate_kbps);

	DeleteObject(expert_ath);
	DeleteObject(expert_check_ath);
	DeleteObject(expert_combo_athtype);
	DeleteObject(expert_psycho);
	DeleteObject(expert_check_tempmask);
	DeleteObject(expert_format);
	DeleteObject(expert_check_iso);

	DeleteObject(filtering_lowpass);
	DeleteObject(filtering_set_lowpass);
	DeleteObject(filtering_edit_lowpass);
	DeleteObject(filtering_set_lowpass_width);
	DeleteObject(filtering_edit_lowpass_width);
	DeleteObject(filtering_highpass);
	DeleteObject(filtering_set_highpass);
	DeleteObject(filtering_edit_highpass);
	DeleteObject(filtering_set_highpass_width);
	DeleteObject(filtering_edit_highpass_width);
	DeleteObject(filtering_misc);
	DeleteObject(filtering_check_disable_all);
}

Int BoCA::ConfigureLAME::SaveSettings()
{
	Config	*config = Config::Get();

	if (abrbitrate < 8)	abrbitrate = 8;
	if (abrbitrate > 320)	abrbitrate = 320;

	if (set_lowpass && filtering_edit_lowpass->GetText().Length() == 0)
	{
		Utilities::ErrorMessage("Please enter a frequency for the Lowpass filter!");

		return Error();
	}

	if (set_lowpass && set_lowpass_width && filtering_edit_lowpass_width->GetText().Length() == 0)
	{
		Utilities::ErrorMessage("Please enter a frequency for the Lowpass filter width!");

		return Error();
	}

	if (set_highpass && filtering_edit_highpass->GetText().Length() == 0)
	{
		Utilities::ErrorMessage("Please enter a frequency for the Highpass filter!");

		return Error();
	}

	if (set_highpass && set_highpass_width && filtering_edit_highpass_width->GetText().Length() == 0)
	{
		Utilities::ErrorMessage("Please enter a frequency for the Highpass filter width!");

		return Error();
	}

	if (set_highpass && set_lowpass && filtering_edit_lowpass->GetText().ToInt() != 0 && filtering_edit_highpass->GetText().ToInt() != 0 && (filtering_edit_lowpass->GetText().ToInt() < filtering_edit_highpass->GetText().ToInt()))
	{
		Utilities::ErrorMessage("Lowpass frequency is lower than Highpass frequency!");

		return Error();
	}

	config->SetIntValue(ConfigID, "Preset", preset);
	config->SetIntValue(ConfigID, "SetBitrate", set_bitrate);
	config->SetIntValue(ConfigID, "Bitrate", GetBitrate());
	config->SetIntValue(ConfigID, "Ratio", (int) (basic_edit_ratio->GetText().ToFloat() * 100));
	config->SetIntValue(ConfigID, "SetQuality", set_quality);
	config->SetIntValue(ConfigID, "Quality", 9 - quality);
	config->SetIntValue(ConfigID, "StereoMode", misc_combo_stereomode->GetSelectedEntryNumber());
	config->SetIntValue(ConfigID, "ForceJS", forcejs);
	config->SetIntValue(ConfigID, "VBRMode", vbrmode);
	config->SetIntValue(ConfigID, "VBRQuality", 99 - vbrquality);
	config->SetIntValue(ConfigID, "ABRBitrate", abrbitrate);
	config->SetIntValue(ConfigID, "SetMinVBRBitrate", set_min_vbr_brate);
	config->SetIntValue(ConfigID, "MinVBRBitrate", GetMinVBRBitrate());
	config->SetIntValue(ConfigID, "SetMaxVBRBitrate", set_max_vbr_brate);
	config->SetIntValue(ConfigID, "MaxVBRBitrate", GetMaxVBRBitrate());
	config->SetIntValue(ConfigID, "CRC", set_crc);
	config->SetIntValue(ConfigID, "Copyright", set_copyright);
	config->SetIntValue(ConfigID, "Original", set_original);
	config->SetIntValue(ConfigID, "Private", set_private);
	config->SetIntValue(ConfigID, "StrictISO", set_iso);
	config->SetIntValue(ConfigID, "DisableFiltering", disable_filtering);
	config->SetIntValue(ConfigID, "SetLowpass", set_lowpass);
	config->SetIntValue(ConfigID, "Lowpass", filtering_edit_lowpass->GetText().ToInt());
	config->SetIntValue(ConfigID, "SetLowpassWidth", set_lowpass_width);
	config->SetIntValue(ConfigID, "LowpassWidth", filtering_edit_lowpass_width->GetText().ToInt());
	config->SetIntValue(ConfigID, "SetHighpass", set_highpass);
	config->SetIntValue(ConfigID, "Highpass", filtering_edit_highpass->GetText().ToInt());
	config->SetIntValue(ConfigID, "SetHighpassWidth", set_highpass_width);
	config->SetIntValue(ConfigID, "HighpassWidth", filtering_edit_highpass_width->GetText().ToInt());
	config->SetIntValue(ConfigID, "EnableATH", enable_ath);
	config->SetIntValue(ConfigID, "ATHType", expert_combo_athtype->GetSelectedEntryNumber() - 1);
	config->SetIntValue(ConfigID, "UseTNS", enable_tempmask);

	return Success();
}

Void BoCA::ConfigureLAME::SetPreset()
{
	preset = basic_combo_preset->GetSelectedEntryNumber();

	if (preset == 0)
	{
		basic_bitrate->Activate();
		basic_option_set_bitrate->Activate();
		basic_option_set_ratio->Activate();
		basic_quality->Activate();
		basic_check_set_quality->Activate();

		vbr_vbrmode->Activate();
		vbr_option_cbr->Activate();
		vbr_option_abr->Activate();
		vbr_option_vbrmtrh->Activate();
		vbr_quality->Activate();
		vbr_text_setquality->Activate();
		vbr_slider_quality->Activate();
		vbr_text_quality->Activate();
		vbr_text_quality_worse->Activate();
		vbr_text_quality_better->Activate();
		vbr_abrbitrate->Activate();
		vbr_slider_abrbitrate->Activate();
		vbr_edit_abrbitrate->Activate();
		vbr_text_abrbitrate_kbps->Activate();

		misc_bits->Activate();
		misc_check_original->Activate();
		misc_check_copyright->Activate();
		misc_check_private->Activate();
		misc_crc->Activate();
		misc_check_crc->Activate();
		misc_stereomode->Activate();
		misc_combo_stereomode->Activate();

		expert_ath->Activate();
		expert_check_ath->Activate();
		expert_psycho->Activate();
		expert_check_tempmask->Activate();
		expert_format->Activate();
		expert_check_iso->Activate();

		filtering_misc->Activate();
		filtering_check_disable_all->Activate();

		SetVBRMode();

		SetQualityOption();
		SetStereoMode();
		SetEnableATH();
		SetDisableFiltering();
	}
	else
	{
		basic_bitrate->Deactivate();
		basic_option_set_bitrate->Deactivate();
		basic_option_set_ratio->Deactivate();
		basic_slider_bitrate->Deactivate();
		basic_text_bitrate->Deactivate();
		basic_edit_ratio->Deactivate();
		basic_quality->Deactivate();
		basic_check_set_quality->Deactivate();
		basic_slider_quality->Deactivate();
		basic_text_quality->Deactivate();
		basic_text_quality_worse->Deactivate();
		basic_text_quality_better->Deactivate();

		vbr_vbrmode->Deactivate();
		vbr_option_cbr->Deactivate();
		vbr_option_abr->Deactivate();
		vbr_option_vbrmtrh->Deactivate();
		vbr_quality->Deactivate();
		vbr_text_setquality->Deactivate();
		vbr_slider_quality->Deactivate();
		vbr_text_quality->Deactivate();
		vbr_text_quality_worse->Deactivate();
		vbr_text_quality_better->Deactivate();
		vbr_abrbitrate->Deactivate();
		vbr_slider_abrbitrate->Deactivate();
		vbr_edit_abrbitrate->Deactivate();
		vbr_text_abrbitrate_kbps->Deactivate();

		misc_bits->Deactivate();
		misc_check_original->Deactivate();
		misc_check_copyright->Deactivate();
		misc_check_private->Deactivate();
		misc_crc->Deactivate();
		misc_check_crc->Deactivate();
		misc_stereomode->Deactivate();
		misc_combo_stereomode->Deactivate();
		misc_check_forcejs->Deactivate();
		misc_bitrate->Deactivate();
		misc_check_set_min_brate->Deactivate();
		misc_check_set_max_brate->Deactivate();
		misc_slider_min_brate->Deactivate();
		misc_slider_max_brate->Deactivate();
		misc_text_min_brate_kbps->Deactivate();
		misc_text_max_brate_kbps->Deactivate();

		expert_ath->Deactivate();
		expert_check_ath->Deactivate();
		expert_combo_athtype->Deactivate();
		expert_psycho->Deactivate();
		expert_check_tempmask->Deactivate();
		expert_format->Deactivate();
		expert_check_iso->Deactivate();

		filtering_lowpass->Deactivate();
		filtering_set_lowpass->Deactivate();
		filtering_edit_lowpass->Deactivate();
		filtering_set_lowpass_width->Deactivate();
		filtering_edit_lowpass_width->Deactivate();
		filtering_highpass->Deactivate();
		filtering_set_highpass->Deactivate();
		filtering_edit_highpass->Deactivate();
		filtering_set_highpass_width->Deactivate();
		filtering_edit_highpass_width->Deactivate();
		filtering_misc->Deactivate();
		filtering_check_disable_all->Deactivate();

		if (preset == 4)
		{
			basic_bitrate->Hide();
			basic_option_set_bitrate->Hide();
			basic_option_set_ratio->Hide();
			basic_slider_bitrate->Hide();
			basic_text_bitrate->Hide();
			basic_text_ratio->Hide();
			basic_edit_ratio->Hide();

			vbr_quality->Hide();
			vbr_text_setquality->Hide();
			vbr_slider_quality->Hide();
			vbr_text_quality->Hide();
			vbr_text_quality_worse->Hide();
			vbr_text_quality_better->Hide();

			vbr_abrbitrate->Show();
			vbr_slider_abrbitrate->Show();
			vbr_edit_abrbitrate->Show();
			vbr_text_abrbitrate_kbps->Show();

			vbr_abrbitrate->Activate();
			vbr_slider_abrbitrate->Activate();
			vbr_edit_abrbitrate->Activate();
			vbr_text_abrbitrate_kbps->Activate();
		}
	}
}

Void BoCA::ConfigureLAME::SetBitrateOption()
{
	if (set_bitrate)
	{
		basic_slider_bitrate->Activate();
		basic_text_bitrate->Activate();
		basic_edit_ratio->Deactivate();
	}
	else
	{
		basic_edit_ratio->Activate();
		basic_slider_bitrate->Deactivate();
		basic_text_bitrate->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetBitrate()
{
	I18n	*i18n = I18n::Get();

	basic_text_bitrate->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(GetBitrate())));
}

Void BoCA::ConfigureLAME::SetQualityOption()
{
	if (set_quality)
	{
		basic_slider_quality->Activate();
		basic_text_quality->Activate();
		basic_text_quality_worse->Activate();
		basic_text_quality_better->Activate();
	}
	else
	{
		basic_slider_quality->Deactivate();
		basic_text_quality->Deactivate();
		basic_text_quality_worse->Deactivate();
		basic_text_quality_better->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetQuality()
{
	basic_text_quality->SetText(String::FromInt(9 - quality));
}

Void BoCA::ConfigureLAME::SetStereoMode()
{
	if (misc_combo_stereomode->GetSelectedEntryNumber() == 3) misc_check_forcejs->Activate();
	else							  misc_check_forcejs->Deactivate();
}

Void BoCA::ConfigureLAME::SetVBRQuality()
{
	String	 txt = String::FromFloat(9.9 - vbrquality / 10.0);

	if ((vbrquality + 1) % 10 == 0) txt.Append(".0");

	vbr_text_quality->SetText(txt);
}

Void BoCA::ConfigureLAME::SetVBRMode()
{
	switch (vbrmode)
	{
		default:
			vbr_quality->Hide();
			vbr_text_setquality->Hide();
			vbr_slider_quality->Hide();
			vbr_text_quality->Hide();
			vbr_text_quality_worse->Hide();
			vbr_text_quality_better->Hide();

			vbr_abrbitrate->Hide();
			vbr_slider_abrbitrate->Hide();
			vbr_edit_abrbitrate->Hide();
			vbr_text_abrbitrate_kbps->Hide();

			misc_bitrate->Deactivate();
			misc_check_set_min_brate->Deactivate();
			misc_slider_min_brate->Deactivate();
			misc_text_min_brate_kbps->Deactivate();
			misc_check_set_max_brate->Deactivate();
			misc_slider_max_brate->Deactivate();
			misc_text_max_brate_kbps->Deactivate();

			basic_bitrate->Show();
			basic_option_set_bitrate->Show();
			basic_option_set_ratio->Show();
			basic_slider_bitrate->Show();
			basic_text_bitrate->Show();
			basic_text_ratio->Show();
			basic_edit_ratio->Show();

			SetBitrateOption();

			break;
		case vbr_abr:
			basic_bitrate->Hide();
			basic_option_set_bitrate->Hide();
			basic_option_set_ratio->Hide();
			basic_slider_bitrate->Hide();
			basic_text_bitrate->Hide();
			basic_text_ratio->Hide();
			basic_edit_ratio->Hide();

			vbr_quality->Hide();
			vbr_text_setquality->Hide();
			vbr_slider_quality->Hide();
			vbr_text_quality->Hide();
			vbr_text_quality_worse->Hide();
			vbr_text_quality_better->Hide();

			vbr_abrbitrate->Show();
			vbr_slider_abrbitrate->Show();
			vbr_edit_abrbitrate->Show();
			vbr_text_abrbitrate_kbps->Show();

			misc_bitrate->Activate();
			misc_check_set_min_brate->Activate();

			misc_check_set_max_brate->Activate();

			SetMinVBRBitrateOption();
			SetMaxVBRBitrateOption();

			break;
		case vbr_mtrh:
			basic_bitrate->Hide();
			basic_option_set_bitrate->Hide();
			basic_option_set_ratio->Hide();
			basic_slider_bitrate->Hide();
			basic_text_bitrate->Hide();
			basic_text_ratio->Hide();
			basic_edit_ratio->Hide();

			vbr_abrbitrate->Hide();
			vbr_slider_abrbitrate->Hide();
			vbr_edit_abrbitrate->Hide();
			vbr_text_abrbitrate_kbps->Hide();

			vbr_quality->Show();
			vbr_text_setquality->Show();
			vbr_slider_quality->Show();
			vbr_text_quality->Show();
			vbr_text_quality_worse->Show();
			vbr_text_quality_better->Show();

			misc_bitrate->Activate();
			misc_check_set_min_brate->Activate();

			misc_check_set_max_brate->Activate();

			SetMinVBRBitrateOption();
			SetMaxVBRBitrateOption();

			break;
	}
}

Void BoCA::ConfigureLAME::SetABRBitrate()
{
	if (!vbr_edit_abrbitrate->IsFocussed()) vbr_edit_abrbitrate->SetText(String::FromInt(abrbitrate));
}

Void BoCA::ConfigureLAME::SetABRBitrateByEditBox()
{
	vbr_slider_abrbitrate->SetValue(vbr_edit_abrbitrate->GetText().ToInt());
}

Void BoCA::ConfigureLAME::SetMinVBRBitrateOption()
{
	if (set_min_vbr_brate)
	{
		misc_slider_min_brate->Activate();
		misc_text_min_brate_kbps->Activate();
	}
	else
	{
		misc_slider_min_brate->Deactivate();
		misc_text_min_brate_kbps->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetMaxVBRBitrateOption()
{
	if (set_max_vbr_brate)
	{
		misc_slider_max_brate->Activate();
		misc_text_max_brate_kbps->Activate();
	}
	else
	{
		misc_slider_max_brate->Deactivate();
		misc_text_max_brate_kbps->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetMinVBRBitrate()
{
	I18n	*i18n = I18n::Get();

	misc_text_min_brate_kbps->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(GetMinVBRBitrate())));

	if (min_vbr_brate > max_vbr_brate)
	{
		misc_slider_max_brate->SetValue(min_vbr_brate);
	}
}

Void BoCA::ConfigureLAME::SetMaxVBRBitrate()
{
	I18n	*i18n = I18n::Get();

	misc_text_max_brate_kbps->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(GetMaxVBRBitrate())));

	if (max_vbr_brate < min_vbr_brate)
	{
		misc_slider_min_brate->SetValue(max_vbr_brate);
	}
}

Void BoCA::ConfigureLAME::SetHighpass()
{
	if (set_highpass)
	{
		filtering_edit_highpass->Activate();
		filtering_set_highpass_width->Activate();

		SetHighpassWidth();
	}
	else
	{
		filtering_edit_highpass->Deactivate();
		filtering_set_highpass_width->Deactivate();
		filtering_edit_highpass_width->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetHighpassWidth()
{
	if (set_highpass_width)	filtering_edit_highpass_width->Activate();
	else			filtering_edit_highpass_width->Deactivate();
}

Void BoCA::ConfigureLAME::SetLowpass()
{
	if (set_lowpass)
	{
		filtering_edit_lowpass->Activate();
		filtering_set_lowpass_width->Activate();

		SetLowpassWidth();
	}
	else
	{
		filtering_edit_lowpass->Deactivate();
		filtering_set_lowpass_width->Deactivate();
		filtering_edit_lowpass_width->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetLowpassWidth()
{
	if (set_lowpass_width)	filtering_edit_lowpass_width->Activate();
	else			filtering_edit_lowpass_width->Deactivate();
}

Void BoCA::ConfigureLAME::SetEnableATH()
{
	if (enable_ath)
	{
		expert_combo_athtype->Activate();
	}
	else
	{
		expert_combo_athtype->Deactivate();
	}
}

Void BoCA::ConfigureLAME::SetDisableFiltering()
{
	if (disable_filtering)
	{
		filtering_lowpass->Deactivate();
		filtering_highpass->Deactivate();
		filtering_set_lowpass->Deactivate();
		filtering_edit_lowpass->Deactivate();
		filtering_set_lowpass_width->Deactivate();
		filtering_edit_lowpass_width->Deactivate();
		filtering_set_highpass->Deactivate();
		filtering_edit_highpass->Deactivate();
		filtering_set_highpass_width->Deactivate();
		filtering_edit_highpass_width->Deactivate();
	}
	else
	{
		filtering_lowpass->Activate();
		filtering_highpass->Activate();
		filtering_set_lowpass->Activate();
		filtering_set_highpass->Activate();

		SetLowpass();
		SetHighpass();
	}
}

Int BoCA::ConfigureLAME::GetBitrate()
{
	return SliderValueToBitrate(bitrate);
}

Int BoCA::ConfigureLAME::GetSliderValue()
{
	return BitrateToSliderValue(Config::Get()->GetIntValue("LAME", "Bitrate", 192));
}

Int BoCA::ConfigureLAME::GetMinVBRBitrate()
{
	return SliderValueToBitrate(min_vbr_brate);
}

Int BoCA::ConfigureLAME::GetMinVBRSliderValue()
{
	return BitrateToSliderValue(Config::Get()->GetIntValue("LAME", "MinVBRBitrate", 128));
}

Int BoCA::ConfigureLAME::GetMaxVBRBitrate()
{
	return SliderValueToBitrate(max_vbr_brate);
}

Int BoCA::ConfigureLAME::GetMaxVBRSliderValue()
{
	return BitrateToSliderValue(Config::Get()->GetIntValue("LAME", "MaxVBRBitrate", 256));
}

Int BoCA::ConfigureLAME::SliderValueToBitrate(Int value)
{
	switch (value)
	{
		case  0: return   8;
		case  1: return  16;
		case  2: return  24;
		case  3: return  32;
		case  4: return  40;
		case  5: return  48;
		case  6: return  56;
		case  7: return  64;
		case  8: return  80;
		case  9: return  96;
		case 10: return 112;
		case 11: return 128;
		case 12: return 144;
		case 13: return 160;
		case 14: return 192;
		case 15: return 224;
		case 16: return 256;
		case 17: return 320;

		default: return 128;
	}
}

Int BoCA::ConfigureLAME::BitrateToSliderValue(Int value)
{
	switch (value)
	{
		case   8: return  0;
		case  16: return  1;
		case  24: return  2;
		case  32: return  3;
		case  40: return  4;
		case  48: return  5;
		case  56: return  6;
		case  64: return  7;
		case  80: return  8;
		case  96: return  9;
		case 112: return 10;
		case 128: return 11;
		case 144: return 12;
		case 160: return 13;
		case 192: return 14;
		case 224: return 15;
		case 256: return 16;
		case 320: return 17;

		default:  return 11;
	}
}
