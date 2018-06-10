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
#include "worker.h"

BoCA_BEGIN_COMPONENT(EncoderCoreAudio)

namespace BoCA
{
	class EncoderCoreAudio : public CS::EncoderComponent
	{
		friend CA::OSStatus	 AudioFileReadProc(void *, CA::SInt64, CA::UInt32, void *, CA::UInt32 *);
		friend CA::OSStatus	 AudioFileWriteProc(void *, CA::SInt64, CA::UInt32, const void *, CA::UInt32 *);
		friend CA::SInt64	 AudioFileGetSizeProc(void *);
		friend CA::OSStatus	 AudioFileSetSizeProc(void *, CA::SInt64);

		private:
			ConfigLayer		*configLayer;

			Array<SuperWorker *>	 workers;

			CA::AudioFileID		 audioFile;

			CA::UInt32		 fileType;

			UnsignedInt		 dataOffset;

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
