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

#include <boca.h>

namespace BoCA
{
	class SuperRepacker
	{
		private:
			IO::Driver		*driver;

			Int			 offset;

			Int			 frameCount;
			Int			 reservoir;

			Int			 cbrIndex;

			Int			 minIndex;
			Int			 maxIndex;

			Buffer<UnsignedByte>	 frameBuffer;

			Array<UnsignedInt32>	 frameOffsets;

			Bool			 FillReservoir(Int = 0);
			Bool			 IncreaseReservoir(Int);
		public:
						 SuperRepacker(IO::Driver *);
						~SuperRepacker();

			Bool			 EnableRateControl(Int, Int);

			Bool			 UnpackFrames(const Buffer<UnsignedByte> &, Buffer<UnsignedByte> &, Array<Int> &);
			Bool			 WriteFrame(UnsignedByte *, Int);

			Bool			 Flush();

			Bool			 UpdateInfoTag(Buffer<UnsignedByte> &, Int64) const;
	};
};
