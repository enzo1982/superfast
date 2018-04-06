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
#include "dllinterface.h"

namespace BoCA
{
	class SuperWorker : public Threads::Thread
	{
		private:
			Threads::Mutex			 workerMutex;

			lame_t				 context;

			Format				 format;

			Int				 frameSize;
			Int				 maxPacketSize;

			Buffer<signed short>		 samplesBuffer;

			Buffer<unsigned char>		 packetBuffer;
			Array<Int>			 packetSizes;

			Int				 overlap;

			Bool				 process;
			Bool				 flush;
			Bool				 quit;

			Int				 Run();
		public:
							 SuperWorker(const Config *, const Format &, Int);
							~SuperWorker();

			Void				 Encode(const Buffer<signed short> &, Int, Int, Bool);

			Void				 GetInfoTag(Buffer<UnsignedByte> &) const;

			Int				 Quit();

			Bool				 Lock()			{ return workerMutex.Lock(); }
			Bool				 Release()		{ return workerMutex.Release(); }

			Bool				 IsReady() const	{ return !process; }

			const Buffer<unsigned char>	&GetPackets() const	{ return packetBuffer; };
			const Array<Int>		&GetPacketSizes() const	{ return packetSizes; };
	};
};
