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
#include "worker.h"
#include "dllinterface.h"

BoCA_BEGIN_COMPONENT(EncoderSpeex)

namespace BoCA
{
	class EncoderSpeex : public CS::EncoderComponent
	{
		private:
			ConfigLayer		*configLayer;

			Array<SuperWorker *>	 workers;

			ogg_stream_state	 os;
			ogg_page		 og;
			ogg_packet		 op;

			Int			 nextWorker;

			spx_int32_t		 frameSize;
			spx_int32_t		 lookAhead;

			Int			 blockSize;
			Int			 overlap;

			Int			 numPackets;
			Int			 totalSamples;

			Buffer<spx_int16_t>	 samplesBuffer;

			Int			 EncodeFrames(Bool);
			Int			 ProcessPackets(const Buffer<unsigned char> &, const Array<Int> &, Bool, Bool, Int);
			Bool			 FixChapterMarks();

			Int			 WriteOggPackets(Bool);
		public:
			static const String	&GetComponentSpecs();

						 EncoderSpeex();
						~EncoderSpeex();

			Bool			 Activate();
			Bool			 Deactivate();

			Int			 WriteData(Buffer<UnsignedByte> &);

			ConfigLayer		*GetConfigurationLayer();
	};
};

BoCA_DEFINE_ENCODER_COMPONENT(EncoderSpeex)

BoCA_END_COMPONENT(EncoderSpeex)
