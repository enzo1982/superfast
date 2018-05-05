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

#include "framecrc.h"

UnsignedInt16	 BoCA::FrameCRC::table[256];
Bool		 BoCA::FrameCRC::initialized = InitTable();

BoCA::FrameCRC::FrameCRC() : crc(0xFFFF)
{
}

BoCA::FrameCRC::~FrameCRC()
{
}

Bool BoCA::FrameCRC::InitTable()
{
	UnsignedInt16	 polynomial = 0x8005;

	for (Int i = 0; i <= 0xFF; i++)
	{
		UnsignedInt16	 value = i << 8;

		for (Int n = 0; n < 8; n++) value = (value << 1) ^ (value & (1 << 15) ? polynomial : 0);

		table[i] = value;
	}

	return True;
}

Bool BoCA::FrameCRC::Feed(const UnsignedByte *data, Int size)
{
	while (size--) crc = (crc << 8) ^ table[(crc >> 8) ^ *data++];

	return True;
}

Bool BoCA::FrameCRC::Update(UnsignedByte *frame)
{
	FrameCRC	 crc;
	Int		 sil = ((frame[1] >> 3) & 0x03) == 0x03 ? ((frame[3] >> 6) == 0x03 ? 17 : 32) :
								  ((frame[3] >> 6) == 0x03 ?  9 : 17);

	crc.Feed(frame + 2, 2);
	crc.Feed(frame + 6, sil);

	frame[4] = crc.crc >> 8;
	frame[5] = crc.crc	& 0xFF;

	return True;
}
