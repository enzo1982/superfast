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

const String	 BoCA::ConfigureOpus::ConfigID = "Opus";

BoCA::ConfigureOpus::ConfigureOpus()
{
	const Config	*config = Config::Get();

	fileExtension	= config->GetIntValue(ConfigID, "FileExtension", 0);
	bitrate		= config->GetIntValue(ConfigID, "Bitrate", 128) / 2;
	complexity	= config->GetIntValue(ConfigID, "Complexity", 10);
	framesize	= config->GetIntValue(ConfigID, "FrameSize", 20000);
	framesize	= Math::Round(framesize <= 10000 ? Math::Log2(framesize / 2500) : framesize / 20000 + 2);
	packet_loss	= config->GetIntValue(ConfigID, "PacketLoss", 0);

	enableVBR	= config->GetIntValue(ConfigID, "EnableVBR", True);
	enableCVBR	= config->GetIntValue(ConfigID, "EnableConstrainedVBR", False);
	enableDTX	= config->GetIntValue(ConfigID, "EnableDTX", False);

	I18n	*i18n = I18n::Get();

	i18n->SetContext("Encoders::Opus");

	group_basic		= new GroupBox(i18n->TranslateString("Basic settings"), Point(7, 11), Size(228, 66));

	text_mode		= new Text(i18n->AddColon(i18n->TranslateString("Encoding mode")), Point(7, 13));

	combo_mode		= new ComboBox(Point(120, 10), Size(214, 0));
	combo_mode->AddEntry(i18n->TranslateString("Auto"));
	combo_mode->AddEntry(i18n->TranslateString("Voice"));
	combo_mode->AddEntry(i18n->TranslateString("Music"));
	combo_mode->SelectNthEntry(config->GetIntValue(ConfigID, "Mode", 0));
	combo_mode->onSelectEntry.Connect(&ConfigureOpus::SetMode, this);

	text_bandwidth		= new Text(i18n->AddColon(i18n->TranslateString("Bandwidth")), Point(7, 40));

	combo_bandwidth		= new ComboBox(Point(120, 37), Size(214, 0));
	combo_bandwidth->AddEntry(i18n->TranslateString("Auto"));
	combo_bandwidth->AddEntry(i18n->TranslateString("Narrowband"));
	combo_bandwidth->AddEntry(i18n->TranslateString("Mediumband"));
	combo_bandwidth->AddEntry(i18n->TranslateString("Wideband"));
	combo_bandwidth->AddEntry(i18n->TranslateString("Superwideband"));
	combo_bandwidth->AddEntry(i18n->TranslateString("Fullband"));
	combo_bandwidth->SelectNthEntry(config->GetIntValue(ConfigID, "Bandwidth", 0));

	Int	 maxTextSize = Math::Max(text_mode->GetUnscaledTextWidth(), text_bandwidth->GetUnscaledTextWidth());

	combo_mode->SetMetrics(Point(14 + maxTextSize, combo_mode->GetY()), Size(202 - maxTextSize, combo_mode->GetHeight()));
	combo_bandwidth->SetMetrics(Point(14 + maxTextSize, combo_bandwidth->GetY()), Size(202 - maxTextSize, combo_bandwidth->GetHeight()));

	group_basic->Add(text_mode);
	group_basic->Add(combo_mode);
	group_basic->Add(text_bandwidth);
	group_basic->Add(combo_bandwidth);

	group_extension		= new GroupBox(i18n->TranslateString("File extension"), Point(243, 11), Size(108, 66));

	option_extension_opus	= new OptionBox(".opus", Point(10, 13), Size(88, 0), &fileExtension, 0);
	option_extension_oga	= new OptionBox(".oga", Point(10, 38), Size(88, 0), &fileExtension, 1);

	group_extension->Add(option_extension_opus);
	group_extension->Add(option_extension_oga);

	group_vbr		= new GroupBox(i18n->TranslateString("Variable bitrate"), Point(7, 89), Size(344, 63));

	check_vbr		= new CheckBox(i18n->TranslateString("Enable variable bitrate encoding"), Point(10, 13), Size(324, 0), &enableVBR);
	check_vbr->onAction.Connect(&ConfigureOpus::SetVBR, this);

	check_cvbr		= new CheckBox(i18n->TranslateString("Constrain bitrate to target value"), Point(27, 36), Size(307, 0), &enableCVBR);

	group_vbr->Add(check_vbr);
	group_vbr->Add(check_cvbr);

	group_quality		= new GroupBox(i18n->TranslateString("Quality"), Point(7, 164), Size(344, 66));

	text_bitrate		= new Text(i18n->AddColon(i18n->TranslateString("Bitrate")), Point(10, 13));

	slider_bitrate		= new Slider(Point(70, 11), Size(201, 0), OR_HORZ, &bitrate, 3, 255);
	slider_bitrate->onValueChange.Connect(&ConfigureOpus::SetBitrate, this);

	edit_bitrate		= new EditBox(NIL, Point(279, 10), Size(25, 0), 3);
	edit_bitrate->SetFlags(EDB_NUMERIC);
	edit_bitrate->onInput.Connect(&ConfigureOpus::SetBitrateByEditBox, this);

	text_bitrate_kbps	= new Text(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(310, 13));

	text_complexity		= new Text(i18n->AddColon(i18n->TranslateString("Complexity")), Point(10, 40));

	slider_complexity	= new Slider(Point(70, 38), Size(201, 0), OR_HORZ, &complexity, 0, 10);
	slider_complexity->onValueChange.Connect(&ConfigureOpus::SetComplexity, this);

	text_complexity_value	= new Text(NIL, Point(279, 40));

	maxTextSize = Math::Max(text_bitrate->GetUnscaledTextWidth(), text_complexity->GetUnscaledTextWidth());

	slider_bitrate->SetMetrics(Point(17 + maxTextSize, slider_bitrate->GetY()), Size(254 - maxTextSize, slider_bitrate->GetHeight()));
	slider_complexity->SetMetrics(Point(17 + maxTextSize, slider_complexity->GetY()), Size(254 - maxTextSize, slider_complexity->GetHeight()));

	group_quality->Add(text_bitrate);
	group_quality->Add(slider_bitrate);
	group_quality->Add(edit_bitrate);
	group_quality->Add(text_bitrate_kbps);
	group_quality->Add(text_complexity);
	group_quality->Add(slider_complexity);
	group_quality->Add(text_complexity_value);

	group_stream		= new GroupBox(i18n->TranslateString("Stream"), Point(7, 242), Size(344, 40));

	text_framesize		= new Text(i18n->AddColon(i18n->TranslateString("Frame length")), Point(10, 13));

	slider_framesize	= new Slider(Point(17 + text_framesize->GetUnscaledTextWidth(), 11), Size(254 - text_framesize->GetUnscaledTextWidth(), 0), OR_HORZ, &framesize, 0, 8);
	slider_framesize->onValueChange.Connect(&ConfigureOpus::SetFrameSize, this);

	text_framesize_value	= new Text(NIL, Point(279, 13));

	group_stream->Add(text_framesize);
	group_stream->Add(slider_framesize);
	group_stream->Add(text_framesize_value);

	group_options		= new GroupBox(i18n->TranslateString("Options"), Point(7, 294), Size(344, 66));

	check_dtx		= new CheckBox(i18n->TranslateString("Enable discontinous transmission"), Point(10, 13), Size(324, 0), &enableDTX);

	text_packet_loss	= new Text(i18n->AddColon(i18n->TranslateString("Expected packet loss")), Point(10, 40));

	slider_packet_loss	= new Slider(Point(17 + text_packet_loss->GetUnscaledTextWidth(), 38), Size(254 - text_packet_loss->GetUnscaledTextWidth(), 0), OR_HORZ, &packet_loss, 0, 100);
	slider_packet_loss->onValueChange.Connect(&ConfigureOpus::SetPacketLoss, this);

	text_packet_loss_value	= new Text(NIL, Point(279, 40));

	group_options->Add(check_dtx);
	group_options->Add(text_packet_loss);
	group_options->Add(slider_packet_loss);
	group_options->Add(text_packet_loss_value);

	SetMode();

	SetVBR();

	SetBitrate();
	SetComplexity();
	SetFrameSize();

	SetPacketLoss();

	Add(group_basic);
	Add(group_extension);
	Add(group_vbr);
	Add(group_quality);
	Add(group_stream);
	Add(group_options);

	SetSize(Size(358, 367));
}

BoCA::ConfigureOpus::~ConfigureOpus()
{
	DeleteObject(group_basic);
	DeleteObject(text_mode);
	DeleteObject(combo_mode);
	DeleteObject(text_bandwidth);
	DeleteObject(combo_bandwidth);

	DeleteObject(group_extension);
	DeleteObject(option_extension_opus);
	DeleteObject(option_extension_oga);

	DeleteObject(group_vbr);
	DeleteObject(check_vbr);
	DeleteObject(check_cvbr);

	DeleteObject(group_quality);
	DeleteObject(text_bitrate);
	DeleteObject(slider_bitrate);
	DeleteObject(edit_bitrate);
	DeleteObject(text_bitrate_kbps);
	DeleteObject(text_complexity);
	DeleteObject(slider_complexity);
	DeleteObject(text_complexity_value);

	DeleteObject(group_stream);
	DeleteObject(text_framesize);
	DeleteObject(slider_framesize);
	DeleteObject(text_framesize_value);

	DeleteObject(group_options);
	DeleteObject(check_dtx);
	DeleteObject(text_packet_loss);
	DeleteObject(slider_packet_loss);
	DeleteObject(text_packet_loss_value);
}

Int BoCA::ConfigureOpus::SaveSettings()
{
	Config	*config = Config::Get();

	config->SetIntValue(ConfigID, "Mode", combo_mode->GetSelectedEntryNumber());
	config->SetIntValue(ConfigID, "Bandwidth", combo_bandwidth->GetSelectedEntryNumber());

	config->SetIntValue(ConfigID, "FileExtension", fileExtension);

	config->SetIntValue(ConfigID, "Bitrate", bitrate * 2);
	config->SetIntValue(ConfigID, "Complexity", complexity);
	config->SetIntValue(ConfigID, "FrameSize", Math::Min(framesize <= 2 ? (Int) (2500 * Math::Pow(2, framesize)) : 20000 * (framesize - 2), 120000));
	config->SetIntValue(ConfigID, "PacketLoss", packet_loss);

	config->SetIntValue(ConfigID, "EnableVBR", enableVBR);
	config->SetIntValue(ConfigID, "EnableConstrainedVBR", enableCVBR);
	config->SetIntValue(ConfigID, "EnableDTX", enableDTX);

	return Success();
}

Void BoCA::ConfigureOpus::SetMode()
{
	switch (combo_mode->GetSelectedEntryNumber())
	{
		case 0:
		case 1:
			check_dtx->Activate();

			text_packet_loss->Activate();
			slider_packet_loss->Activate();
			text_packet_loss_value->Activate();

			break;
		case 2:
			check_dtx->Deactivate();

			text_packet_loss->Deactivate();
			slider_packet_loss->Deactivate();
			text_packet_loss_value->Deactivate();

			break;
	}

	SetVBR();
}

Void BoCA::ConfigureOpus::SetVBR()
{
	if (enableVBR && combo_mode->GetSelectedEntryNumber() != 1)
	{
		check_cvbr->Activate();
	}
	else
	{
		enableCVBR = False;

		check_cvbr->Deactivate();
	}
}

Void BoCA::ConfigureOpus::SetBitrate()
{
	edit_bitrate->SetText(String::FromInt(bitrate * 2));
}

Void BoCA::ConfigureOpus::SetBitrateByEditBox()
{
	slider_bitrate->SetValue(edit_bitrate->GetText().ToInt() / 2);
}

Void BoCA::ConfigureOpus::SetComplexity()
{
	text_complexity_value->SetText(String::FromInt(complexity));
}

Void BoCA::ConfigureOpus::SetFrameSize()
{
	I18n	*i18n = I18n::Get();

	text_framesize_value->SetText(i18n->TranslateString("%1 ms", "Technical").Replace("%1", String::FromFloat(Math::Min(framesize <= 2 ? 2.5 * Math::Pow(2, framesize) : 20.0 * (framesize - 2), 120.0))));
}

Void BoCA::ConfigureOpus::SetPacketLoss()
{
	I18n	*i18n = I18n::Get();

	text_packet_loss_value->SetText(i18n->TranslateString("%1%", "Technical").Replace("%1", String::FromInt(packet_loss)));
}
