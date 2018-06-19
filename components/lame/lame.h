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
#include "repacker.h"
#include "worker.h"

BoCA_BEGIN_COMPONENT(EncoderLAME)

namespace BoCA
{
	class EncoderLAME : public CS::EncoderComponent
	{
		private:
			ConfigLayer		*configLayer;

			Array<SuperWorker *>	 workers;

			Int			 nextWorker;

			Int			 dataOffset;
			Int			 frameSize;

			Int			 blockSize;
			Int			 overlap;

			Int64			 totalSamples;

			SuperRepacker		*repacker;

			Buffer<signed short>	 samplesBuffer;

			Int			 EncodeFrames(Bool);
			Int			 ProcessResults(SuperWorker *, Bool);
			Int			 ProcessPackets(const Buffer<unsigned char> &, const Array<Int> &, Bool, Int &, Bool &);
		public:
			static const String	&GetComponentSpecs();

						 EncoderLAME();
						~EncoderLAME();

			Bool			 Activate();
			Bool			 Deactivate();

			Int			 WriteData(Buffer<UnsignedByte> &);

			ConfigLayer		*GetConfigurationLayer();
	};
};

BoCA_DEFINE_ENCODER_COMPONENT(EncoderLAME)

BoCA_END_COMPONENT(EncoderLAME)
