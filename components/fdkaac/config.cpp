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

const String	 BoCA::ConfigureFDKAAC::ConfigID = "FDKAAC";

BoCA::ConfigureFDKAAC::ConfigureFDKAAC()
{
	const Config	*config = Config::Get();

	mpegVersion	= config->GetIntValue(ConfigID, "MPEGVersion", 0);
	aacType		= config->GetIntValue(ConfigID, "AACType", AOT_SBR);
	bitrate		= config->GetIntValue(ConfigID, "Bitrate", 64);
	allowID3	= config->GetIntValue(ConfigID, "AllowID3v2", False);
	fileFormat	= config->GetIntValue(ConfigID, "MP4Container", True);
	fileExtension	= config->GetIntValue(ConfigID, "MP4FileExtension", 0);

	I18n	*i18n = I18n::Get();

	tabwidget		= new TabWidget(Point(7, 7), Size(500, 258));

	i18n->SetContext("Encoders::AAC::Format");

	layer_format		= new Layer(i18n->TranslateString("Format"));

	group_mp4		= new GroupBox(i18n->TranslateString("File format"), Point(7, 11), Size(120, 65));

	option_mp4		= new OptionBox("MP4", Point(10, 13), Size(99, 0), &fileFormat, 1);
	option_mp4->onAction.Connect(&ConfigureFDKAAC::SetFileFormat, this);

	if (mp4v2dll == NIL)
	{
		option_mp4->Deactivate();

		fileFormat = 0;
	}

	option_aac		= new OptionBox("AAC", Point(10, 38), Size(99, 0), &fileFormat, 0);
	option_aac->onAction.Connect(&ConfigureFDKAAC::SetFileFormat, this);

	group_mp4->Add(option_mp4);
	group_mp4->Add(option_aac);

	group_aactype		= new GroupBox(i18n->TranslateString("AAC object type"), Point(7, 88), Size(120, 140));

	option_aactype_low	= new OptionBox("LC", Point(10, 13), Size(99, 0), &aacType, AOT_AAC_LC);
	option_aactype_low->onAction.Connect(&ConfigureFDKAAC::SetObjectType, this);

	option_aactype_he	= new OptionBox("HE", Point(10, 38), Size(99, 0), &aacType, AOT_SBR);
	option_aactype_he->onAction.Connect(&ConfigureFDKAAC::SetObjectType, this);

	option_aactype_hev2	= new OptionBox("HEv2", Point(10, 63), Size(99, 0), &aacType, AOT_PS);
	option_aactype_hev2->onAction.Connect(&ConfigureFDKAAC::SetObjectType, this);

	option_aactype_ld	= new OptionBox("LD", Point(10, 88), Size(99, 0), &aacType, AOT_ER_AAC_LD);
	option_aactype_ld->onAction.Connect(&ConfigureFDKAAC::SetObjectType, this);

	option_aactype_eld	= new OptionBox("ELD", Point(10, 113), Size(99, 0), &aacType, AOT_ER_AAC_ELD);
	option_aactype_eld->onAction.Connect(&ConfigureFDKAAC::SetObjectType, this);

	group_aactype->Add(option_aactype_low);
	group_aactype->Add(option_aactype_he);
	group_aactype->Add(option_aactype_hev2);
	group_aactype->Add(option_aactype_ld);
	group_aactype->Add(option_aactype_eld);

	group_id3v2		= new GroupBox(i18n->TranslateString("Tags"), Point(135, 88), Size(279, 90));

	check_id3v2		= new CheckBox(i18n->TranslateString("Allow ID3v2 tags in AAC files"), Point(10, 13), Size(200, 0), &allowID3);
	check_id3v2->SetWidth(check_id3v2->GetUnscaledTextWidth() + 20);

	text_note		= new Text(i18n->AddColon(i18n->TranslateString("Note")), Point(10, 38));
	text_id3v2		= new Text(i18n->TranslateString("Some players may have problems playing AAC\nfiles with ID3 tags attached. Please use this option only\nif you are sure that your player can handle these tags."), Point(text_note->GetUnscaledTextWidth() + 12, 38));

	group_id3v2->SetSize(Size(Math::Max(240, text_note->GetUnscaledTextWidth() + text_id3v2->GetUnscaledTextWidth() + 22), Math::Max(text_note->GetUnscaledTextHeight(), text_id3v2->GetUnscaledTextHeight()) + 48));

	group_id3v2->Add(check_id3v2);
	group_id3v2->Add(text_note);
	group_id3v2->Add(text_id3v2);

	group_version		= new GroupBox(i18n->TranslateString("MPEG version"), Point(135, 11), Size(group_id3v2->GetWidth() / 2 - 4, 65));

	option_version_mpeg2	= new OptionBox("MPEG 2", Point(10, 13), Size(group_version->GetWidth() - 21, 0), &mpegVersion, 127);
	option_version_mpeg2->onAction.Connect(&ConfigureFDKAAC::SetMPEGVersion, this);

	option_version_mpeg4	= new OptionBox("MPEG 4", Point(10, 38), Size(group_version->GetWidth() - 21, 0), &mpegVersion, 0);
	option_version_mpeg4->onAction.Connect(&ConfigureFDKAAC::SetMPEGVersion, this);

	group_version->Add(option_version_mpeg2);
	group_version->Add(option_version_mpeg4);

	group_extension		= new GroupBox(i18n->TranslateString("File extension"), Point(group_version->GetWidth() + 143 + (group_id3v2->GetWidth() % 2), 11), Size(group_id3v2->GetWidth() / 2 - 4, 65));

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

	group_bitrate		= new GroupBox(i18n->TranslateString("Bitrate"), Point(7, 11), Size(group_id3v2->GetWidth() + 128, 43));

	text_bitrate		= new Text(i18n->AddColon(i18n->TranslateString("Bitrate per channel")), Point(10, 15));

	slider_bitrate		= new Slider(Point(text_bitrate->GetUnscaledTextSize().cx + 17, 13), Size(group_bitrate->GetWidth() - 91 - text_bitrate->GetUnscaledTextSize().cx, 0), OR_HORZ, &bitrate, 8, 256);
	slider_bitrate->onValueChange.Connect(&ConfigureFDKAAC::SetBitrate, this);

	edit_bitrate		= new EditBox(NIL, Point(group_bitrate->GetWidth() - 66, 12), Size(25, 0), 3);
	edit_bitrate->SetFlags(EDB_NUMERIC);
	edit_bitrate->onInput.Connect(&ConfigureFDKAAC::SetBitrateByEditBox, this);

	text_bitrate_kbps	= new Text(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(group_bitrate->GetWidth() - 34, 15));

	group_bitrate->Add(text_bitrate);
	group_bitrate->Add(slider_bitrate);
	group_bitrate->Add(edit_bitrate);
	group_bitrate->Add(text_bitrate_kbps);

	SetFileFormat();
	SetMPEGVersion();
	SetObjectType();
	SetBitrate();

	tabwidget->SetSize(Size(group_id3v2->GetWidth() + 146, Math::Max(258, group_id3v2->GetHeight() + 118)));

	Add(tabwidget);

	tabwidget->Add(layer_quality);
	tabwidget->Add(layer_format);

	layer_format->Add(group_version);
	layer_format->Add(group_aactype);
	layer_format->Add(group_mp4);
	layer_format->Add(group_extension);
	layer_format->Add(group_id3v2);

	layer_quality->Add(group_bitrate);

	SetSize(Size(group_id3v2->GetWidth() + 160, Math::Max(272, group_id3v2->GetHeight() + 132)));
}

BoCA::ConfigureFDKAAC::~ConfigureFDKAAC()
{
	DeleteObject(tabwidget);
	DeleteObject(layer_format);
	DeleteObject(layer_quality);

	DeleteObject(group_version);
	DeleteObject(option_version_mpeg2);
	DeleteObject(option_version_mpeg4);
	DeleteObject(group_aactype);
	DeleteObject(option_aactype_low);
	DeleteObject(option_aactype_he);
	DeleteObject(option_aactype_hev2);
	DeleteObject(option_aactype_ld);
	DeleteObject(option_aactype_eld);
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
	DeleteObject(text_bitrate);
	DeleteObject(slider_bitrate);
	DeleteObject(edit_bitrate);
	DeleteObject(text_bitrate_kbps);
}

Int BoCA::ConfigureFDKAAC::SaveSettings()
{
	Config	*config = Config::Get();

	if (bitrate <	8) bitrate =   8;
	if (bitrate > 256) bitrate = 256;

	config->SetIntValue(ConfigID, "MPEGVersion", mpegVersion);
	config->SetIntValue(ConfigID, "AACType", aacType);
	config->SetIntValue(ConfigID, "Bitrate", bitrate);
	config->SetIntValue(ConfigID, "AllowID3v2", allowID3);
	config->SetIntValue(ConfigID, "MP4Container", fileFormat);
	config->SetIntValue(ConfigID, "MP4FileExtension", fileExtension);

	return Success();
}

Void BoCA::ConfigureFDKAAC::SetMPEGVersion()
{
	if (mpegVersion == 0) // MPEG4;
	{
		option_aactype_ld->Activate();
		option_aactype_eld->Activate();
	}
	else if (mpegVersion == 127) // MPEG2;
	{
		if (aacType == AOT_ER_AAC_LD || aacType == AOT_ER_AAC_ELD)
		{
			aacType = AOT_SBR;

			OptionBox::internalCheckValues.Emit();
		}

		option_aactype_ld->Deactivate();
		option_aactype_eld->Deactivate();
	}
}

Void BoCA::ConfigureFDKAAC::SetObjectType()
{
	if (aacType == AOT_ER_AAC_LD ||
	    aacType == AOT_ER_AAC_ELD) option_version_mpeg2->Deactivate();
	else			       option_version_mpeg2->Activate();

	switch (aacType)
	{
		case AOT_AAC_LC:
			slider_bitrate->SetRange(8, 256);

			break;
		case AOT_SBR:
			slider_bitrate->SetRange(8, 64);

			break;
		case AOT_PS:
			slider_bitrate->SetRange(8, 32);

			break;
		case AOT_ER_AAC_LD:
			slider_bitrate->SetRange(8, 256);

			break;
		case AOT_ER_AAC_ELD:
			slider_bitrate->SetRange(8, 256);

			break;
	}

	SetBitrate();
}

Void BoCA::ConfigureFDKAAC::SetBitrate()
{
	edit_bitrate->SetText(String::FromInt(bitrate));
}

Void BoCA::ConfigureFDKAAC::SetBitrateByEditBox()
{
	slider_bitrate->SetValue(edit_bitrate->GetText().ToInt());
}

Void BoCA::ConfigureFDKAAC::SetFileFormat()
{
	if (fileFormat == 1)	// MP4 container
	{
		group_version->Deactivate();
		group_id3v2->Deactivate();

		group_extension->Activate();

		option_aactype_ld->Activate();
		option_aactype_eld->Activate();

		if (mpegVersion == 127) // MPEG2
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
