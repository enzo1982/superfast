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

const String	 BoCA::ConfigureCoreAudio::ConfigID = "CoreAudio";

BoCA::ConfigureCoreAudio::ConfigureCoreAudio()
{
	const Config	*config = Config::Get();

	bitrate		= config->GetIntValue(ConfigID, "Bitrate", 64);
	allowID3	= config->GetIntValue(ConfigID, "AllowID3v2", False);
	fileFormat	= config->GetIntValue(ConfigID, "MP4Container", True);
	fileExtension	= config->GetIntValue(ConfigID, "MP4FileExtension", 0);

	I18n	*i18n = I18n::Get();

	tabwidget		= new TabWidget(Point(7, 7), Size(500, 208));

	i18n->SetContext("Encoders::AAC::Format");

	layer_format		= new Layer(i18n->TranslateString("Format"));

	group_id3v2		= new GroupBox(i18n->TranslateString("Tags"), Point(7, 88), Size(279, 90));

	check_id3v2		= new CheckBox(i18n->TranslateString("Allow ID3v2 tags in AAC files"), Point(10, 13), Size(200, 0), &allowID3);
	check_id3v2->SetWidth(check_id3v2->GetUnscaledTextWidth() + 20);

	text_note		= new Text(i18n->AddColon(i18n->TranslateString("Note")), Point(10, 38));
	text_id3v2		= new Text(i18n->TranslateString("Some players may have problems playing AAC\nfiles with ID3 tags attached. Please use this option only\nif you are sure that your player can handle these tags."), Point(text_note->GetUnscaledTextWidth() + 12, 38));

	group_id3v2->SetSize(Size(Math::Max(240, text_note->GetUnscaledTextWidth() + text_id3v2->GetUnscaledTextWidth() + 22), Math::Max(text_note->GetUnscaledTextHeight(), text_id3v2->GetUnscaledTextHeight()) + 48));

	group_id3v2->Add(check_id3v2);
	group_id3v2->Add(text_note);
	group_id3v2->Add(text_id3v2);

	group_mp4		= new GroupBox(i18n->TranslateString("File format"), Point(7, 11), Size(group_id3v2->GetWidth() / 2 - 4, 65));

	option_mp4		= new OptionBox("MP4", Point(10, 13), Size(group_mp4->GetWidth() - 20, 0), &fileFormat, 1);
	option_mp4->onAction.Connect(&ConfigureCoreAudio::SetFileFormat, this);

	option_aac		= new OptionBox("AAC", Point(10, 38), Size(group_mp4->GetWidth() - 20, 0), &fileFormat, 0);
	option_aac->onAction.Connect(&ConfigureCoreAudio::SetFileFormat, this);

	group_mp4->Add(option_mp4);
	group_mp4->Add(option_aac);

	group_extension		= new GroupBox(i18n->TranslateString("File extension"), Point(group_mp4->GetWidth() + 15 + (group_id3v2->GetWidth() % 2), 11), Size(group_id3v2->GetWidth() / 2 - 4, 65));

	option_extension_m4a	= new OptionBox(".m4a", Point(10, 13),					Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 0);
	option_extension_m4b	= new OptionBox(".m4b", Point(10, 38),					Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 1);
	option_extension_m4r	= new OptionBox(".m4r", Point(group_extension->GetWidth() / 2 + 4, 13), Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 2);
	option_extension_mp4	= new OptionBox(".mp4", Point(group_extension->GetWidth() / 2 + 4, 38), Size(group_extension->GetWidth() / 2 - 14, 0), &fileExtension, 3);

	group_extension->Add(option_extension_m4a);
	group_extension->Add(option_extension_m4b);
	group_extension->Add(option_extension_m4r);
	group_extension->Add(option_extension_mp4);

	i18n->SetContext("Encoders::AAC::Codec");

	layer_quality		= new Layer(i18n->TranslateString("Codec"));

	group_codec		= new GroupBox(i18n->TranslateString("Audio codec"), Point(7, 11), Size(group_id3v2->GetWidth(), 43));

	text_codec		= new Text(i18n->AddColon(i18n->TranslateString("Audio codec")), Point(10, 15));

	combo_codec		= new ComboBox(Point(text_codec->GetUnscaledTextSize().cx + 17, 12), Size(group_codec->GetWidth() - text_codec->GetUnscaledTextSize().cx - 27, 0));

	/* Query supported formats from Core Audio
	 * and add them to the combo box list.
	 */
	CA::UInt32	 size = 0;

	CA::AudioFormatGetPropertyInfo(CA::kAudioFormatProperty_EncodeFormatIDs, 0, NIL, &size);

	CA::UInt32	*formats = new CA::UInt32 [size / sizeof(CA::UInt32)];

	CA::AudioFormatGetProperty(CA::kAudioFormatProperty_EncodeFormatIDs, 0, NIL, &size, formats);

	for (UnsignedInt i = 0; i < size / sizeof(CA::UInt32); i++)
	{
		if	(formats[i] == CA::kAudioFormatMPEG4AAC)	 combo_codec->AddEntry("MPEG4 AAC Low Complexity");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_HE)	 combo_codec->AddEntry("MPEG4 AAC High Efficiency");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_HE_V2)	 combo_codec->AddEntry("MPEG4 AAC High Efficiency v2");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_LD)	 combo_codec->AddEntry("MPEG4 AAC Low Delay");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_ELD)	 combo_codec->AddEntry("MPEG4 AAC Enhanced Low Delay");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_ELD_SBR) combo_codec->AddEntry("MPEG4 AAC Enhanced Low Delay SBR");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_ELD_V2)	 combo_codec->AddEntry("MPEG4 AAC Enhanced Low Delay v2");
		else if	(formats[i] == CA::kAudioFormatMPEG4AAC_Spatial) combo_codec->AddEntry("MPEG4 AAC Spatial");
		else if (formats[i] == CA::kAudioFormatAppleLossless)	 combo_codec->AddEntry("Apple Lossless Audio Codec");
		else							 continue;

		codecs.Add(formats[i]);

		if ((UnsignedInt) config->GetIntValue(ConfigID, "Codec", CA::kAudioFormatMPEG4AAC) == formats[i]) combo_codec->SelectNthEntry(combo_codec->Length() - 1);
	}

	delete [] formats;

	combo_codec->onSelectEntry.Connect(&ConfigureCoreAudio::SetCodec, this);

	group_codec->Add(text_codec);
	group_codec->Add(combo_codec);

	i18n->SetContext("Encoders::AAC::Quality");

	group_bitrate		= new GroupBox(i18n->TranslateString("Bitrate"), Point(7, 66), Size(group_id3v2->GetWidth(), 43));

	text_bitrate		= new Text(i18n->AddColon(i18n->TranslateString("Bitrate per channel")), Point(10, 15));

	slider_bitrate		= new Slider(Point(text_bitrate->GetUnscaledTextSize().cx + 17, 13), Size(group_bitrate->GetWidth() - 91 - text_bitrate->GetUnscaledTextSize().cx, 0), OR_HORZ, &bitrate, 1, 256);
	slider_bitrate->onValueChange.Connect(&ConfigureCoreAudio::SetBitrate, this);

	edit_bitrate		= new EditBox(String::FromInt(bitrate), Point(group_bitrate->GetWidth() - 66, 12), Size(25, 0), 3);
	edit_bitrate->SetFlags(EDB_NUMERIC);
	edit_bitrate->onInput.Connect(&ConfigureCoreAudio::SetBitrateByEditBox, this);

	text_bitrate_kbps	= new Text(i18n->TranslateString("%1 kbps", "Technical").Replace("%1", NIL).Replace(" ", NIL), Point(group_bitrate->GetWidth() - 34, 15));

	group_bitrate->Add(text_bitrate);
	group_bitrate->Add(slider_bitrate);
	group_bitrate->Add(edit_bitrate);
	group_bitrate->Add(text_bitrate_kbps);

	SetCodec();

	tabwidget->SetSize(group_id3v2->GetSize() + Size(18, 118));

	Add(tabwidget);

	tabwidget->Add(layer_quality);
	tabwidget->Add(layer_format);

	layer_format->Add(group_mp4);
	layer_format->Add(group_extension);
	layer_format->Add(group_id3v2);

	layer_quality->Add(group_codec);
	layer_quality->Add(group_bitrate);

	SetSize(group_id3v2->GetSize() + Size(32, 132));
}

BoCA::ConfigureCoreAudio::~ConfigureCoreAudio()
{
	DeleteObject(tabwidget);
	DeleteObject(layer_format);
	DeleteObject(layer_quality);

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

	DeleteObject(group_codec);
	DeleteObject(text_codec);
	DeleteObject(combo_codec);
	DeleteObject(group_bitrate);
	DeleteObject(text_bitrate);
	DeleteObject(slider_bitrate);
	DeleteObject(edit_bitrate);
	DeleteObject(text_bitrate_kbps);
}

Int BoCA::ConfigureCoreAudio::SaveSettings()
{
	Config	*config = Config::Get();

	config->SetIntValue(ConfigID, "Codec", codecs.GetNth(combo_codec->GetSelectedEntryNumber()));

	if (bitrates.Length() == 2)
	{
		if (bitrate < bitrates.GetNth(0)) bitrate = bitrates.GetNth(0);
		if (bitrate > bitrates.GetNth(1)) bitrate = bitrates.GetNth(1);
	}

	if	(bitrates.Length() == 2) config->SetIntValue(ConfigID, "Bitrate", bitrate);
	else if (bitrates.Length() >  2) config->SetIntValue(ConfigID, "Bitrate", bitrates.GetNth((bitrates.Length() / 2 + bitrate) * 2 + 1));

	config->SetIntValue(ConfigID, "MP4Container", fileFormat);
	config->SetIntValue(ConfigID, "MP4FileExtension", fileExtension);
	config->SetIntValue(ConfigID, "AllowID3v2", allowID3);

	return Success();
}

Void BoCA::ConfigureCoreAudio::SetCodec()
{
	CA::UInt32	 format	= codecs.GetNth(combo_codec->GetSelectedEntryNumber());
	CA::UInt32	 size	= 0;

	CA::AudioFormatGetPropertyInfo(CA::kAudioFormatProperty_AvailableEncodeBitRates, sizeof(format), &format, &size);

	CA::AudioValueRange	*bitrateValues = new CA::AudioValueRange [size / sizeof(CA::AudioValueRange)];

	CA::AudioFormatGetProperty(CA::kAudioFormatProperty_AvailableEncodeBitRates, sizeof(format), &format, &size, bitrateValues);

	if (size > 0 && bitrates.Length() > 2) bitrate = bitrates.GetNth((bitrates.Length() / 2 + bitrate) * 2 + 1);

	bitrates.RemoveAll();

	for (UnsignedInt i = 0; i < size / sizeof(CA::AudioValueRange); i++)
	{
		if (bitrateValues[i].mMinimum / 1000 > 192 && bitrateValues[i].mMaximum / 1000 > 192) continue;
		if (					      bitrateValues[i].mMaximum / 1000 > 192) bitrateValues[i].mMaximum = 192 * 1000;

		bitrates.Add(bitrateValues[i].mMinimum / 1000);
		bitrates.Add(bitrateValues[i].mMaximum / 1000);
	}

	delete [] bitrateValues;

	if (bitrates.Length() > 0) group_bitrate->Activate();
	else			   group_bitrate->Deactivate();

	for (Int i = 0; i < bitrates.Length() / 2; i++)
	{
		if (bitrate == bitrates.GetNth(i * 2 + 1)) bitrate = -(bitrates.Length() / 2) + i;
	}

	if	(bitrates.Length() == 2) { edit_bitrate->Activate();   slider_bitrate->SetRange(bitrates.GetNth(0), bitrates.GetNth(1)); }
	else if (bitrates.Length() >  2) { edit_bitrate->Deactivate(); slider_bitrate->SetRange(-bitrates.Length() / 2, -1);		 }

	if (codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatAppleLossless)
	{
		group_mp4->Deactivate();

		fileFormat = 1;

		option_extension_m4r->Deactivate();

		if (fileExtension == 2) fileExtension = 0;
	}
	else if	(codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC	  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_HE	  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_HE_V2	  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_LD	  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_ELD	  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_ELD_SBR ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_ELD_V2  ||
		 codecs.GetNth(combo_codec->GetSelectedEntryNumber()) == CA::kAudioFormatMPEG4AAC_Spatial)
	{
		group_mp4->Activate();

		option_extension_m4r->Activate();
	}

	SetBitrate();
	SetFileFormat();
}

Void BoCA::ConfigureCoreAudio::SetBitrate()
{
	if (bitrates.Length() == 0) return;

	if (bitrates.Length() == 2) edit_bitrate->SetText(String::FromInt(bitrate));
	else			    edit_bitrate->SetText(String::FromInt(bitrates.GetNth((bitrates.Length() / 2 + bitrate) * 2 + 1)));
}

Void BoCA::ConfigureCoreAudio::SetBitrateByEditBox()
{
	slider_bitrate->SetValue(edit_bitrate->GetText().ToInt());
}

Void BoCA::ConfigureCoreAudio::SetFileFormat()
{
	if (fileFormat == 1)	// MP4 container
	{
		group_id3v2->Deactivate();

		group_extension->Activate();
	}
	else			// raw AAC file format
	{
		group_id3v2->Activate();

		group_extension->Deactivate();
	}
}
