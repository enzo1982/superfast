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
#include "dllinterface.h"

BoCA::ConfigureFAAC::ConfigureFAAC()
{
	Config	*config = Config::Get();

	mpegVersion	= config->GetIntValue("FAAC", "MPEGVersion", 0);
	bitrate		= config->GetIntValue("FAAC", "Bitrate", 96);
	allowjs		= config->GetIntValue("FAAC", "AllowJS", True);
	usetns		= config->GetIntValue("FAAC", "UseTNS", False);
	setQuality	= config->GetIntValue("FAAC", "SetQuality", True);
	aacQuality	= config->GetIntValue("FAAC", "AACQuality", 100);
	allowID3	= config->GetIntValue("FAAC", "AllowID3v2", False);
	fileFormat	= config->GetIntValue("FAAC", "MP4Container", True);
	fileExtension	= config->GetIntValue("FAAC", "MP4FileExtension", 0);

	I18n	*i18n = I18n::Get();

	tabwidget		= new TabWidget(Point(7, 7), Size(525, 208));

	i18n->SetContext("Encoders::AAC::Format");

	layer_format		= new Layer(i18n->TranslateString("Format"));

	group_mp4		= new GroupBox(i18n->TranslateString("File format"), Point(7, 11), Size(120, 65));

	option_mp4		= new OptionBox("MP4", Point(10, 13), Size(99, 0), &fileFormat, 1);
	option_mp4->onAction.Connect(&ConfigureFAAC::SetFileFormat, this);

	if (mp4v2dll == NIL)
	{
		option_mp4->Deactivate();

		fileFormat = 0;
	}

	option_aac		= new OptionBox("AAC", Point(10, 38), Size(99, 0), &fileFormat, 0);
	option_aac->onAction.Connect(&ConfigureFAAC::SetFileFormat, this);

	group_mp4->Add(option_mp4);
	group_mp4->Add(option_aac);

	group_id3v2		= new GroupBox(i18n->TranslateString("Tags"), Point(7, 88), Size(279, 90));

	check_id3v2		= new CheckBox(i18n->TranslateString("Allow ID3v2 tags in AAC files"), Point(10, 13), Size(200, 0), &allowID3);
	check_id3v2->SetWidth(check_id3v2->GetUnscaledTextWidth() + 20);

	text_note		= new Text(i18n->AddColon(i18n->TranslateString("Note")), Point(10, 38));
	text_id3v2		= new Text(i18n->TranslateString("Some players may have problems playing AAC\nfiles with ID3 tags attached. Please use this option only\nif you are sure that your player can handle these tags."), Point(text_note->GetUnscaledTextWidth() + 12, 38));

	group_id3v2->SetSize(Size(Math::Max(368, text_note->GetUnscaledTextWidth() + text_id3v2->GetUnscaledTextWidth() + 22), Math::Max(text_note->GetUnscaledTextHeight(), text_id3v2->GetUnscaledTextHeight()) + 48));

	group_id3v2->Add(check_id3v2);
	group_id3v2->Add(text_note);
	group_id3v2->Add(text_id3v2);

	group_version		= new GroupBox(i18n->TranslateString("MPEG version"), Point(135, 11), Size(group_id3v2->GetWidth() / 2 - 68, 65));

	option_version_mpeg2	= new OptionBox("MPEG 2", Point(10, 13), Size(group_version->GetWidth() - 21, 0), &mpegVersion, 1);
	option_version_mpeg4	= new OptionBox("MPEG 4", Point(10, 38), Size(group_version->GetWidth() - 21, 0), &mpegVersion, 0);

	group_version->Add(option_version_mpeg2);
	group_version->Add(option_version_mpeg4);

	group_extension		= new GroupBox(i18n->TranslateString("File extension"), Point(group_version->GetWidth() + 143 + (group_id3v2->GetWidth() % 2), 11), Size(group_id3v2->GetWidth() / 2 - 68, 65));

	option_extension_m4a	= new OptionBox(".m4a", Point(10, 13),					Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 0);
	option_extension_m4b	= new OptionBox(".m4b", Point(10, 38),					Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 1);
	option_extension_m4r	= new OptionBox(".m4r", Point(group_extension->GetWidth() / 2 + 4, 13), Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 2);
	option_extension_mp4	= new OptionBox(".mp4", Point(group_extension->GetWidth() / 2 + 4, 38), Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 3);

	group_extension->Add(option_extension_m4a);
	group_extension->Add(option_extension_m4b);
	group_extension->Add(option_extension_m4r);
	group_extension->Add(option_extension_mp4);

	i18n->SetContext("Encoders::AAC::Quality");

	layer_quality		= new Layer(i18n->TranslateString("Quality"));

	group_bitrate		= new GroupBox(i18n->TranslateString("Bitrate / Quality"), Point(7, 11), Size(320, 65));

	option_bitrate		= new OptionBox(i18n->AddColon(i18n->TranslateString("Bitrate per channel")), Point(10, 13), Size(150, 0), &setQuality, 0);
	option_bitrate->onAction.Connect(&ConfigureFAAC::ToggleBitrateQuality, this);
	option_bitrate->SetWidth(option_bitrate->GetUnscaledTextWidth() + 19);

	slider_bitrate		= new Slider(Point(option_bitrate->GetWidth() + 19, 13), Size(227 - option_bitrate->GetWidth(), 0), OR_HORZ, &bitrate, 8, 256);
	slider_bitrate->onValueChange.Connect(&ConfigureFAAC::SetBitrate, this);

	edit_bitrate		= new EditBox(NIL, Point(254, 12), Size(25, 0), 3);
	edit_bitrate->SetFlags(EDB_NUMERIC);
	edit_bitrate->onInput.Connect(&ConfigureFAAC::SetBitrateByEditBox, this);

	text_bitrate_kbps	= new Text(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(286, 15));

	option_quality		= new OptionBox(i18n->AddColon(i18n->TranslateString("Set quality")), Point(10, 38), Size(150, 0), &setQuality, 1);
	option_quality->onAction.Connect(&ConfigureFAAC::ToggleBitrateQuality, this);
	option_quality->SetWidth(option_bitrate->GetUnscaledTextWidth() + 19);

	slider_quality		= new Slider(Point(option_quality->GetWidth() + 19, 38), Size(227 - option_quality->GetWidth(), 0), OR_HORZ, &aacQuality, 10, 500);
	slider_quality->onValueChange.Connect(&ConfigureFAAC::SetQuality, this);

	edit_quality		= new EditBox(NIL, Point(254, 37), Size(25, 0), 3);
	edit_quality->SetFlags(EDB_NUMERIC);
	edit_quality->onInput.Connect(&ConfigureFAAC::SetQualityByEditBox, this);

	text_quality_percent	= new Text(i18n->TranslateString("%1%", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(286, 40));

	group_bitrate->Add(option_bitrate);
	group_bitrate->Add(slider_bitrate);
	group_bitrate->Add(edit_bitrate);
	group_bitrate->Add(text_bitrate_kbps);
	group_bitrate->Add(option_quality);
	group_bitrate->Add(slider_quality);
	group_bitrate->Add(edit_quality);
	group_bitrate->Add(text_quality_percent);

	group_js		= new GroupBox(i18n->TranslateString("Stereo mode"), Point(335, 11), Size(189, 42));

	check_js		= new CheckBox(i18n->TranslateString("Allow Joint Stereo"), Point(10, 13), Size(168, 0), &allowjs);

	group_js->Add(check_js);

	group_tns		= new GroupBox(i18n->TranslateString("Temporal Noise Shaping"), Point(335, 65), Size(189, 42));

	check_tns		= new CheckBox(i18n->TranslateString("Use Temporal Noise Shaping"), Point(10, 13), Size(168, 0), &usetns);

	group_tns->Add(check_tns);

	group_bandwidth		= new GroupBox(i18n->TranslateString("Maximum bandwidth"), Point(7, 88), Size(320, 43));

	text_bandwidth		= new Text(i18n->AddColon(i18n->TranslateString("Maximum AAC frequency bandwidth to use (Hz)")), Point(11, 15));

	edit_bandwidth		= new EditBox(String::FromInt(config->GetIntValue("FAAC", "BandWidth", 22050)), Point(text_bandwidth->GetUnscaledTextWidth() + 19, 12), Size(291 - text_bandwidth->GetUnscaledTextWidth(), 0), 5);
	edit_bandwidth->SetFlags(EDB_NUMERIC);

	group_bandwidth->Add(text_bandwidth);
	group_bandwidth->Add(edit_bandwidth);

	SetBitrate();
	SetQuality();
	SetFileFormat();

	ToggleBitrateQuality();

	tabwidget->SetSize(Size(Math::Max(535, group_id3v2->GetWidth() + 18), Math::Max(193, group_id3v2->GetHeight() + 118)));

	Add(tabwidget);

	tabwidget->Add(layer_quality);
	tabwidget->Add(layer_format);

	layer_format->Add(group_version);
	layer_format->Add(group_mp4);
	layer_format->Add(group_extension);
	layer_format->Add(group_id3v2);

	layer_quality->Add(group_bitrate);
	layer_quality->Add(group_js);
	layer_quality->Add(group_tns);
	layer_quality->Add(group_bandwidth);

	SetSize(tabwidget->GetSize() + Size(14, 14));
}

BoCA::ConfigureFAAC::~ConfigureFAAC()
{
	DeleteObject(tabwidget);
	DeleteObject(layer_format);
	DeleteObject(layer_quality);

	DeleteObject(group_version);
	DeleteObject(option_version_mpeg2);
	DeleteObject(option_version_mpeg4);
	DeleteObject(group_mp4);
	DeleteObject(option_mp4);
	DeleteObject(option_aac);
	DeleteObject(group_extension);
	DeleteObject(option_extension_m4a);
	DeleteObject(option_extension_m4b);
	DeleteObject(option_extension_m4r);
	DeleteObject(option_extension_mp4);
	DeleteObject(group_id3v2);
	DeleteObject(check_id3v2);
	DeleteObject(text_note);
	DeleteObject(text_id3v2);

	DeleteObject(group_bitrate);
	DeleteObject(option_bitrate);
	DeleteObject(slider_bitrate);
	DeleteObject(edit_bitrate);
	DeleteObject(text_bitrate_kbps);
	DeleteObject(option_quality);
	DeleteObject(slider_quality);
	DeleteObject(edit_quality);
	DeleteObject(text_quality_percent);
	DeleteObject(group_js);
	DeleteObject(check_js);
	DeleteObject(group_tns);
	DeleteObject(check_tns);
	DeleteObject(group_bandwidth);
	DeleteObject(text_bandwidth);
	DeleteObject(edit_bandwidth);
}

Int BoCA::ConfigureFAAC::SaveSettings()
{
	Config	*config = Config::Get();

	if (bitrate    <   8) bitrate	 =   8;
	if (bitrate    > 256) bitrate	 = 256;

	if (aacQuality <  10) aacQuality =  10;
	if (aacQuality > 500) aacQuality = 500;

	config->SetIntValue("FAAC", "MPEGVersion", mpegVersion);
	config->SetIntValue("FAAC", "Bitrate", bitrate);
	config->SetIntValue("FAAC", "AllowJS", allowjs);
	config->SetIntValue("FAAC", "UseTNS", usetns);
	config->SetIntValue("FAAC", "BandWidth", edit_bandwidth->GetText().ToInt());
	config->SetIntValue("FAAC", "SetQuality", setQuality);
	config->SetIntValue("FAAC", "AACQuality", aacQuality);
	config->SetIntValue("FAAC", "AllowID3v2", allowID3);
	config->SetIntValue("FAAC", "MP4Container", fileFormat);
	config->SetIntValue("FAAC", "MP4FileExtension", fileExtension);

	return Success();
}

Void BoCA::ConfigureFAAC::SetBitrate()
{
	edit_bitrate->SetText(String::FromInt(bitrate));
}

Void BoCA::ConfigureFAAC::SetBitrateByEditBox()
{
	slider_bitrate->SetValue(edit_bitrate->GetText().ToInt());
}

Void BoCA::ConfigureFAAC::SetQuality()
{
	edit_quality->SetText(String::FromInt(aacQuality));
}

Void BoCA::ConfigureFAAC::SetQualityByEditBox()
{
	slider_quality->SetValue(edit_quality->GetText().ToInt());
}

Void BoCA::ConfigureFAAC::SetFileFormat()
{
	if (fileFormat == 1)	// MP4 container
	{
		group_version->Deactivate();
		group_id3v2->Deactivate();

		group_extension->Activate();

		if (mpegVersion == 1) // MPEG2
		{
			mpegVersion = 0;

			OptionBox::internalCheckValues.Emit();
		}
	}
	else			// raw AAC file format
	{
		group_version->Activate();
		group_id3v2->Activate();

		group_extension->Deactivate();
	}
}

Void BoCA::ConfigureFAAC::ToggleBitrateQuality()
{
	if (setQuality)
	{
		slider_bitrate->Deactivate();
		edit_bitrate->Deactivate();

		slider_quality->Activate();
		edit_quality->Activate();
	}
	else
	{
		slider_quality->Deactivate();
		edit_quality->Deactivate();

		slider_bitrate->Activate();
		edit_bitrate->Activate();
	}
}
