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

#include <boca.h>
#include "dllinterface.h"

using namespace smooth::Threads;

namespace BoCA
{
	class SuperWorker : public Thread
	{
		friend CA::OSStatus	 AudioConverterComplexInputDataProc(CA::AudioConverterRef, CA::UInt32 *, CA::AudioBufferList *, CA::AudioStreamPacketDescription **, void *);

		private:
			Mutex						 workerMutex;

			CA::AudioConverterRef				 converter;
			CA::AudioBufferList				*buffers;

			Format						 format;

			Int						 frameSize;

			Buffer<unsigned char>				 samplesBuffer;
			CA::UInt32					 bufferSize;
			Int						 bytesConsumed;

			Buffer<unsigned char>				 suppliedData;

			Buffer<unsigned char>				 packetBuffer;
			Array<Int>					 packetSizes;
			Array<CA::AudioStreamPacketDescription *>	 packetInfos;

			Bool						 process;
			Bool						 flush;
			Bool						 quit;

			Int						 Run();
		public:
									 SuperWorker(const Config *, const Format &);
									~SuperWorker();

			Void						 Encode(const Buffer<unsigned char> &, Int, Int, Bool);

			Int						 Quit();

			Bool						 Lock()			{ return workerMutex.Lock(); }
			Bool						 Release()		{ return workerMutex.Release(); }

			Bool						 IsReady()		{ return !process; }

			const Buffer<unsigned char>			&GetPackets() const	{ return packetBuffer; };
			const Array<Int>				&GetPacketSizes() const	{ return packetSizes; };
			const Array<CA::AudioStreamPacketDescription *>	&GetPacketInfos() const	{ return packetInfos; };

			CA::AudioConverterPrimeInfo			 GetPrimeInfo() const;
			unsigned char					*GetMagicCookie(CA::UInt32 *) const;

			static Int					 GetOutputSampleRate(CA::UInt32, Int);
	};
};
