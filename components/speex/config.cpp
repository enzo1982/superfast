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

#include "config.h"

const String	 BoCA::ConfigureSpeex::ConfigID = "Speex";

BoCA::ConfigureSpeex::ConfigureSpeex()
{
	const Config	*config = Config::Get();

	quality = config->GetIntValue(ConfigID, "Quality", 8);
	bitrate = config->GetIntValue(ConfigID, "Bitrate", -16);

	if (quality > 0) cbrmode = 0;
	else		 cbrmode = 1;

	quality = Math::Abs(quality);
	bitrate = Math::Abs(bitrate);

	vbrmode = config->GetIntValue(ConfigID, "VBR", 0);
	vbrq = config->GetIntValue(ConfigID, "VBRQuality", 80);
	vbrmax = config->GetIntValue(ConfigID, "VBRMaxBitrate", -48);

	if (vbrmax > 0) use_vbrmax = True;
	else		use_vbrmax = False;

	vbrmax = Math::Abs(vbrmax);

	abr = config->GetIntValue(ConfigID, "ABR", -16);

	if (abr > 0) vbrmode = 2;

	abr = Math::Abs(abr);

	complexity = config->GetIntValue(ConfigID, "Complexity", 3);

	use_vad = config->GetIntValue(ConfigID, "VAD", 0);
	use_dtx = config->GetIntValue(ConfigID, "DTX", 0);

	I18n	*i18n = I18n::Get();

	i18n->SetContext("Encoders::Speex");

	group_profile		= new GroupBox(i18n->TranslateString("Profile"), Point(7, 11), Size(406, 43));

	text_profile		= new Text(i18n->AddColon(i18n->TranslateString("Select encoding profile")), Point(10, 16));

	combo_profile		= new ComboBox(Point(18 + text_profile->GetUnscaledTextWidth(), 13), Size(378 - text_profile->GetUnscaledTextWidth(), 0));
	combo_profile->AddEntry(i18n->TranslateString("Auto"));
	combo_profile->AddEntry(i18n->TranslateString("Narrowband (8 kHz)"));
	combo_profile->AddEntry(i18n->TranslateString("Wideband (16 kHz)"));
	combo_profile->AddEntry(i18n->TranslateString("Ultra-Wideband (32 kHz)"));
	combo_profile->SelectNthEntry(config->GetIntValue(ConfigID, "Mode", -1) + 1);

	group_profile->Add(text_profile);
	group_profile->Add(combo_profile);

	group_vbr_mode		= new GroupBox(i18n->TranslateString("VBR mode"), Point(7, 66), Size(128, 91));

	option_cbr		= new OptionBox(i18n->TranslateString("CBR (no VBR)"), Point(10, 14), Size(108, 0), &vbrmode, 0);
	option_cbr->onAction.Connect(&ConfigureSpeex::SetVBRMode, this);

	option_vbr		= new OptionBox(i18n->TranslateString("VBR"), Point(10, 39), Size(108, 0), &vbrmode, 1);
	option_vbr->onAction.Connect(&ConfigureSpeex::SetVBRMode, this);

	option_abr		= new OptionBox(i18n->TranslateString("ABR"), Point(10, 64), Size(108, 0), &vbrmode, 2);
	option_abr->onAction.Connect(&ConfigureSpeex::SetVBRMode, this);

	group_vbr_mode->Add(option_cbr);
	group_vbr_mode->Add(option_vbr);
	group_vbr_mode->Add(option_abr);

	group_cbr_quality	= new GroupBox(i18n->TranslateString("CBR quality"), Point(143, 66), Size(270, 66));

	option_cbr_quality	= new OptionBox(i18n->AddColon(i18n->TranslateString("Quality")), Point(10, 14), Size(55, 0), &cbrmode, 0);
	option_cbr_quality->onAction.Connect(&ConfigureSpeex::SetCBRMode, this);

	option_cbr_bitrate	= new OptionBox(i18n->AddColon(i18n->TranslateString("Bitrate")), Point(10, 39), Size(55, 0), &cbrmode, 1);
	option_cbr_bitrate->onAction.Connect(&ConfigureSpeex::SetCBRMode, this);

	Int	 maxTextSize = Math::Max(option_cbr_quality->GetUnscaledTextWidth(), option_cbr_bitrate->GetUnscaledTextWidth());

	option_cbr_quality->SetWidth(maxTextSize + 21);
	option_cbr_bitrate->SetWidth(maxTextSize + 21);

	slider_cbr_quality	= new Slider(Point(maxTextSize + 39, 14), Size(176 - maxTextSize, 0), OR_HORZ, &quality, 0, 10);
	slider_cbr_quality->onValueChange.Connect(&ConfigureSpeex::SetQuality, this);

	text_cbr_quality_value	= new Text(NIL, Point(222, 16));

	slider_cbr_bitrate	= new Slider(Point(maxTextSize + 39, 39), Size(176 - maxTextSize, 0), OR_HORZ, &bitrate, 4, 64);
	slider_cbr_bitrate->onValueChange.Connect(&ConfigureSpeex::SetBitrate, this);

	text_cbr_bitrate_value	= new Text(NIL, Point(222, 41));

	group_cbr_quality->Add(option_cbr_quality);
	group_cbr_quality->Add(slider_cbr_quality);
	group_cbr_quality->Add(text_cbr_quality_value);
	group_cbr_quality->Add(option_cbr_bitrate);
	group_cbr_quality->Add(slider_cbr_bitrate);
	group_cbr_quality->Add(text_cbr_bitrate_value);

	group_vbr_quality	= new GroupBox(i18n->TranslateString("VBR quality"), Point(143, 66), Size(270, 66));

	text_vbr_quality	= new Text(i18n->AddColon(i18n->TranslateString("Quality")), Point(10, 16));

	check_vbr_bitrate	= new CheckBox(i18n->AddColon(i18n->TranslateString("Max. bitrate")), Point(10, 39), Size(80, 0), &use_vbrmax);
	check_vbr_bitrate->onAction.Connect(&ConfigureSpeex::ToggleVBRBitrate, this);
	check_vbr_bitrate->SetWidth(check_vbr_bitrate->GetUnscaledTextWidth() + 21);

	maxTextSize = Math::Max(text_vbr_quality->GetUnscaledTextWidth() - 21, check_vbr_bitrate->GetUnscaledTextWidth());

	slider_vbr_quality	= new Slider(Point(maxTextSize + 39, 14), Size(176 - maxTextSize, 0), OR_HORZ, &vbrq, 0, 100);
	slider_vbr_quality->onValueChange.Connect(&ConfigureSpeex::SetVBRQuality, this);

	text_vbr_quality_value	= new Text(NIL, Point(222, 16));

	slider_vbr_bitrate	= new Slider(Point(maxTextSize + 39, 39), Size(176 - maxTextSize, 0), OR_HORZ, &vbrmax, 4, 64);
	slider_vbr_bitrate->onValueChange.Connect(&ConfigureSpeex::SetVBRBitrate, this);

	text_vbr_bitrate_value	= new Text(NIL, Point(222, 41));

	group_vbr_quality->Add(text_vbr_quality);
	group_vbr_quality->Add(slider_vbr_quality);
	group_vbr_quality->Add(text_vbr_quality_value);
	group_vbr_quality->Add(check_vbr_bitrate);
	group_vbr_quality->Add(slider_vbr_bitrate);
	group_vbr_quality->Add(text_vbr_bitrate_value);

	group_abr_bitrate	= new GroupBox(i18n->TranslateString("ABR target bitrate"), Point(143, 66), Size(270, 41));

	text_abr_bitrate	= new Text(i18n->AddColon(i18n->TranslateString("Bitrate")), Point(10, 16));

	slider_abr_bitrate	= new Slider(Point(text_abr_bitrate->GetUnscaledTextWidth() + 18, 14), Size(197 - text_abr_bitrate->GetUnscaledTextWidth(), 0), OR_HORZ, &abr, 4, 64);
	slider_abr_bitrate->onValueChange.Connect(&ConfigureSpeex::SetABRBitrate, this);

	text_abr_bitrate_value	= new Text(NIL, Point(222, 16));

	group_abr_bitrate->Add(text_abr_bitrate);
	group_abr_bitrate->Add(slider_abr_bitrate);
	group_abr_bitrate->Add(text_abr_bitrate_value);

	group_options		= new GroupBox(i18n->TranslateString("Options"), Point(7, 169), Size(197, 66));

	check_vad		= new CheckBox(i18n->TranslateString("Voice Activity Detection"), Point(10, 14), Size(177, 0), &use_vad);
	check_vad->onAction.Connect(&ConfigureSpeex::SetVAD, this);

	check_dtx		= new CheckBox(i18n->TranslateString("Discontinued Transmission"), Point(10, 39), Size(177, 0), &use_dtx);

	group_options->Add(check_vad);
	group_options->Add(check_dtx);

	group_complexity	= new GroupBox(i18n->TranslateString("Algorithm complexity"), Point(212, 169), Size(201, 42));

	text_complexity		= new Text(i18n->AddColon(i18n->TranslateString("Complexity")), Point(10, 16));

	slider_complexity	= new Slider(Point(text_complexity->GetUnscaledTextWidth() + 18, 14), Size(154 - text_complexity->GetUnscaledTextWidth(), 0), OR_HORZ, &complexity, 1, 10);
	slider_complexity->onValueChange.Connect(&ConfigureSpeex::SetComplexity, this);

	text_complexity_value	= new Text(NIL, Point(179, 16));

	group_complexity->Add(text_complexity);
	group_complexity->Add(slider_complexity);
	group_complexity->Add(text_complexity_value);

	SetVBRMode();
	SetVBRQuality();

	ToggleVBRBitrate();
	SetVBRBitrate();

	SetCBRMode();
	SetQuality();
	SetBitrate();

	SetABRBitrate();

	SetComplexity();

	Add(group_profile);
	Add(group_vbr_mode);
	Add(group_cbr_quality);
	Add(group_vbr_quality);
	Add(group_abr_bitrate);
	Add(group_options);
	Add(group_complexity);

	SetSize(Size(420, 242));
}

BoCA::ConfigureSpeex::~ConfigureSpeex()
{
	DeleteObject(group_profile);
	DeleteObject(text_profile);
	DeleteObject(combo_profile);

	DeleteObject(group_vbr_mode);
	DeleteObject(option_cbr);
	DeleteObject(option_vbr);
	DeleteObject(option_abr);

	DeleteObject(group_cbr_quality);
	DeleteObject(option_cbr_quality);
	DeleteObject(slider_cbr_quality);
	DeleteObject(text_cbr_quality_value);
	DeleteObject(option_cbr_bitrate);
	DeleteObject(slider_cbr_bitrate);
	DeleteObject(text_cbr_bitrate_value);

	DeleteObject(group_vbr_quality);
	DeleteObject(text_vbr_quality);
	DeleteObject(slider_vbr_quality);
	DeleteObject(text_vbr_quality_value);
	DeleteObject(check_vbr_bitrate);
	DeleteObject(slider_vbr_bitrate);
	DeleteObject(text_vbr_bitrate_value);

	DeleteObject(group_abr_bitrate);
	DeleteObject(text_abr_bitrate);
	DeleteObject(slider_abr_bitrate);
	DeleteObject(text_abr_bitrate_value);

	DeleteObject(group_options);
	DeleteObject(check_vad);
	DeleteObject(check_dtx);

	DeleteObject(group_complexity);
	DeleteObject(text_complexity);
	DeleteObject(slider_complexity);
	DeleteObject(text_complexity_value);
}

Int BoCA::ConfigureSpeex::SaveSettings()
{
	Config	*config = Config::Get();

	config->SetIntValue(ConfigID, "Mode", combo_profile->GetSelectedEntryNumber() - 1);
	config->SetIntValue(ConfigID, "VBR", vbrmode == 1);
	config->SetIntValue(ConfigID, "VBRQuality", vbrq);
	config->SetIntValue(ConfigID, "VBRMaxBitrate", use_vbrmax ? vbrmax : -vbrmax);
	config->SetIntValue(ConfigID, "ABR", vbrmode == 2 ? abr : -abr);
	config->SetIntValue(ConfigID, "Quality", cbrmode == 0 ? quality : -quality);
	config->SetIntValue(ConfigID, "Bitrate", cbrmode == 1 ? bitrate : -bitrate);
	config->SetIntValue(ConfigID, "Complexity", complexity);
	config->SetIntValue(ConfigID, "VAD", use_vad);
	config->SetIntValue(ConfigID, "DTX", use_dtx);

	return Success();
}

Void BoCA::ConfigureSpeex::SetVBRMode()
{
	group_cbr_quality->Hide();
	group_vbr_quality->Hide();
	group_abr_bitrate->Hide();

	switch (vbrmode)
	{
		case 0:
			group_cbr_quality->Show();

			check_vad->Activate();

			SetVAD();

			break;
		case 1:
			group_vbr_quality->Show();

			check_vad->Deactivate();
			check_dtx->Activate();

			break;
		case 2:
			group_abr_bitrate->Show();

			check_vad->Deactivate();
			check_dtx->Activate();

			break;
	}
}

Void BoCA::ConfigureSpeex::SetVBRQuality()
{
	String	 txt = String::FromFloat(double(vbrq) / 10);

	if (vbrq % 10 == 0) txt.Append(".0");

	text_vbr_quality_value->SetText(txt);
}

Void BoCA::ConfigureSpeex::ToggleVBRBitrate()
{
	if (use_vbrmax)
	{
		slider_vbr_bitrate->Activate();
		text_vbr_bitrate_value->Activate();
	}
	else
	{
		slider_vbr_bitrate->Deactivate();
		text_vbr_bitrate_value->Deactivate();
	}
}

Void BoCA::ConfigureSpeex::SetVBRBitrate()
{
	I18n	*i18n = I18n::Get();

	text_vbr_bitrate_value->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(vbrmax)));
}

Void BoCA::ConfigureSpeex::SetCBRMode()
{
	switch (cbrmode)
	{
		case 0:
			slider_cbr_quality->Activate();
			text_cbr_quality_value->Activate();

			slider_cbr_bitrate->Deactivate();
			text_cbr_bitrate_value->Deactivate();

			break;
		case 1:
			slider_cbr_bitrate->Activate();
			text_cbr_bitrate_value->Activate();

			slider_cbr_quality->Deactivate();
			text_cbr_quality_value->Deactivate();

			break;
	}
}

Void BoCA::ConfigureSpeex::SetQuality()
{
	text_cbr_quality_value->SetText(String::FromInt(quality));
}

Void BoCA::ConfigureSpeex::SetBitrate()
{
	I18n	*i18n = I18n::Get();

	text_cbr_bitrate_value->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(bitrate)));
}

Void BoCA::ConfigureSpeex::SetABRBitrate()
{
	I18n	*i18n = I18n::Get();

	text_abr_bitrate_value->SetText(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", String::FromInt(abr)));
}

Void BoCA::ConfigureSpeex::SetComplexity()
{
	text_complexity_value->SetText(String::FromInt(complexity));
}

Void BoCA::ConfigureSpeex::SetVAD()
{
	if (use_vad) check_dtx->Activate();
	else	     check_dtx->Deactivate();
}
