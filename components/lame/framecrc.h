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

#ifndef H_FRAMECRC
#define H_FRAMECRC

#include <smooth.h>

using namespace smooth;

namespace BoCA
{
	class FrameCRC
	{
		private:
			static UnsignedInt16	 table[256];
			static Bool		 initialized;

			UnsignedInt16		 crc;

			static Void		 InitTable();

			Bool			 Feed(const UnsignedByte *, Int);

						 FrameCRC();
						~FrameCRC();
		public:
			static Bool		 Update(UnsignedByte *);
	};
};

#endif
