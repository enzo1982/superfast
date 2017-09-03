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

BoCA_BEGIN_COMPONENT(EncoderCoreAudio)

namespace BoCA
{
	class EncoderCoreAudio : public CS::EncoderComponent
	{
		private:
			ConfigLayer		*configLayer;

			Array<SuperWorker *>	 workers;

			CA::AudioFileID		 audioFile;

			Buffer<unsigned char>	 samplesBuffer;

			Int			 nextWorker;

			Int			 frameSize;

			Int			 blockSize;
			Int			 overlap;

			Int			 packetsWritten;
			Int			 packetsMissing;

			Int			 totalSamples;

			Int			 EncodeFrames(Bool);
			Int			 ProcessPackets(const Buffer<unsigned char> &, const Array<Int> &, const Array<CA::AudioStreamPacketDescription *> &, Bool, Bool);
		public:
			static const String	&GetComponentSpecs();

						 EncoderCoreAudio();
						~EncoderCoreAudio();

			Bool			 IsLossless() const;

			Bool			 Activate();
			Bool			 Deactivate();

			Int			 WriteData(Buffer<UnsignedByte> &);

			Bool			 SetOutputFormat(Int);
			String			 GetOutputFileExtension() const;

			ConfigLayer		*GetConfigurationLayer();
	};
};

BoCA_DEFINE_ENCODER_COMPONENT(EncoderCoreAudio)

BoCA_END_COMPONENT(EncoderCoreAudio)
