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

#include <smooth.h>

#include "repacker.h"

/* ToDo: Add support for CBR, rate limiting, Xing header TOC, Xing header CRC, frame CRC.
 */
namespace BoCA
{
	static const Int	 bitrates[2][16]   = { { 0,  8000, 16000, 24000, 32000, 40000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 },   // MPEG 2/2.5
						       { 0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 } }; // MPEG 1

	static const Int	 samplerates[4][4] = { { 11025, 12000,  8000, 0 },   // MPEG 2.5
						       {     0,	    0,	   0, 0 },   // reserved
						       { 22050, 24000, 16000, 0 },   // MPEG 2
						       { 44100, 48000, 32000, 0 } }; // MPEG 1

	static Int GetMode(UnsignedByte *frame)
	{
		return (frame[1] >> 3) & 0x03;
	}

	static Int GetBitrateIndex(UnsignedByte *frame)
	{
		return frame[2] >> 4;
	}

	static Void SetBitrateIndex(UnsignedByte *frame, Int brindex)
	{
		frame[2] = (brindex << 4) | (frame[2] & 0x0F);
	}

	static Int GetSampleRateIndex(UnsignedByte *frame)
	{
		return (frame[2] >> 2) & 0x03;
	}

	static Bool GetPadding(UnsignedByte *frame)
	{
		return (frame[2] >> 1) & 0x01;
	}

	static Void SetPadding(UnsignedByte *frame, Bool padding)
	{
		frame[2] = (padding ? 0x02 : 0x00) | (frame[2] & 0xFD);
	}

	static Int GetFrameSize(UnsignedByte *frame)
	{
		Int	 mode	 = GetMode(frame);
		Int	 brindex = GetBitrateIndex(frame);
		Int	 srindex = GetSampleRateIndex(frame);

		Int	 bitrate = bitrates[mode & 1][brindex];
		Int	 srate	 = samplerates[mode][srindex];

		Int	 padding = GetPadding(frame);

		return (mode == 3 ? 144 : 72) * bitrate / srate + padding;
	}

	static Int GetSideInfoLength(UnsignedByte *frame)
	{
		return GetMode(frame) == 3 ? ((frame[3] >> 6) == 0x03 ? 17 : 32) :
					     ((frame[3] >> 6) == 0x03 ?  9 : 17);
	}

	static Int GetMainDataOffset(UnsignedByte *frame)
	{
		return GetMode(frame) == 3 ? (frame[4] << 1) | (frame[5] >> 7) :
					      frame[4];
	}

	static Void SetMainDataOffset(UnsignedByte*frame, Int offset)
	{
		if (GetMode(frame) == 3)
		{
			frame[4] = offset >> 1;
			frame[5] = ((offset & 1) << 7) | (frame[5] & 0x7F);
		}
		else
		{
			frame[4] = offset;
		}
	}

	static Int GetMainDataLength(UnsignedByte *frame)
	{
		Int	 bits = 0;

		if (GetMode(frame) != 3)
		{
			if (GetSideInfoLength(frame) == 9)
			{
				bits += ((frame[ 5] & 0x7F) <<  5) | (frame[ 6] >> 3);
			}
			else
			{
				bits += ((frame[ 5] & 0x3F) <<  6) | (frame[ 6] >> 2);
				bits += ((frame[13] & 0x7F) <<  5) | (frame[14] >> 3);
			}
		}
		else
		{
			if (GetSideInfoLength(frame) == 17)
			{
				bits += ((frame[ 6] & 0x3F) <<  6) | (frame[ 7] >> 2)			;
				bits += ((frame[13] & 0x07) <<  9) | (frame[14] << 1) | (frame[15] >> 7);
			}
			else
			{
				bits += ((frame[ 6] & 0x0F) <<  8) | (frame[ 7]     )			;
				bits += ((frame[13] & 0x01) << 11) | (frame[14] << 3) | (frame[15] >> 5);
				bits += ((frame[21] & 0x3F) <<  6) | (frame[22] >> 2)			;
				bits += ((frame[28] & 0x07) <<  9) | (frame[29] << 1) | (frame[30] >> 7);
			}
		}

		return Math::Ceil(Float(bits) / 8);
	}

	static Bool IsValidFrame(UnsignedByte *frame)
	{
		if (((frame[0] << 3) | (frame[1] >> 5)) != 0x07FF) return False;

		Int	 mode	 = GetMode(frame);
		Int	 brindex = GetBitrateIndex(frame);
		Int	 srindex = GetSampleRateIndex(frame);

		if (mode == 1 || brindex == 0 || brindex == 15 || srindex == 3) return False;

		Int	 frameb	 = GetFrameSize(frame);

		if (frameb < 24) return False;

		return True;
	}
};

BoCA::SuperRepacker::SuperRepacker(IO::Driver *iDriver)
{
	driver	   = iDriver;

	offset	   = driver->GetPos();

	frameCount = 0;
	reservoir  = 0;
}

BoCA::SuperRepacker::~SuperRepacker()
{
}

Bool BoCA::SuperRepacker::UpdateInfoTag(Buffer<UnsignedByte> &frame, Int64 totalSamples) const
{
	UnsignedByte	*tag = frame + GetSideInfoLength(frame) + 4;

	/* Remove TOC flag.
	 */
	tag[0x07] = tag[0x07] ^ 0x04;

	/* Set frame count.
	 */
	Int	 frames = frameCount - 1; // Minus this info frame

	tag[0x08] =  frames >> 24;
	tag[0x09] = (frames >> 16) & 0xFF;
	tag[0x0A] = (frames >>  8) & 0xFF;
	tag[0x0B] =  frames	   & 0xFF;

	/* Set byte count.
	 */
	Int	 bytes = driver->GetSize() - offset;

	tag[0x0C] =  bytes  >> 24;
	tag[0x0D] = (bytes  >> 16) & 0xFF;
	tag[0x0E] = (bytes  >>  8) & 0xFF;
	tag[0x0F] =  bytes 	   & 0xFF;

	/* Set pad samples.
	 */
	Int	 delaySamples = (tag[0x8D] << 4) | ((tag[0x8E] & 0xF0) >> 4);
	Int	 padSamples   = frames * (GetMode(frame) == 3 ? 1152 : 576) - totalSamples - delaySamples;

	tag[0x8E] |= padSamples >> 8;
	tag[0x8F]  = padSamples & 0xFF;

	/* Set byte count.
	 */
	tag[0x94] =  bytes  >> 24;
	tag[0x95] = (bytes  >> 16) & 0xFF;
	tag[0x96] = (bytes  >>  8) & 0xFF;
	tag[0x97] =  bytes 	   & 0xFF;

	return True;
}

Bool BoCA::SuperRepacker::UnpackFrames(const Buffer<UnsignedByte> &data, Buffer<UnsignedByte> &packets, Array<Int> &packetSizes) const
{
	Buffer<UnsignedByte>	 main;

	for (Int n = 0; n < data.Size(); n++)
	{
		if (!IsValidFrame(data + n)) continue;

		UnsignedByte	*frame = data + n;

		Int	 frameb	= GetFrameSize(frame);

		Int	 info	= GetSideInfoLength(frame) + 4;
		Int	 bytes	= GetMainDataLength(frame);
		Int	 pre	= GetMainDataOffset(frame);

		/* Buffer main data.
		 */
		main.Resize(main.Size() + frameb - info);

		memcpy(main + main.Size() - (frameb - info), frame + info, frameb - info);

		/* Write packet.
		 */
		Int	 offset	= packets.Size();
		Int	 size	= Math::Max(info + bytes, frameb);

		packets.Resize(packets.Size() + size);

		memcpy(packets + offset, frame, info);
		memcpy(packets + offset + info, main + main.Size() - (frameb - info) - pre, bytes);

		SetBitrateIndex(packets + offset, 0);
		SetPadding(packets + offset, False);
		SetMainDataOffset(packets + offset, 0);

		packetSizes.Add(size);

		n += frameb - 1;
	}

	return True;
}

Bool BoCA::SuperRepacker::WriteFrame(UnsignedByte *iframe, Int size)
{
	if (frameCount++ == 0) { driver->WriteData(iframe, size); return True; }

	UnsignedByte	 frame[1441] = { 0 };

	memcpy(frame, iframe, size);

	Int	 info  = GetSideInfoLength(frame) + 4;
	Int	 bytes = GetMainDataLength(frame);

	/* Fill superfluous reservoir with zero bytes.
	 */
	Int	 maxR  = GetMode(frame) == 3 ? 511 : 255;

	FillReservoir(maxR);

	/* Process frame data.
	 */
	while (True)
	{
		/* Write main data to reservoir.
		 */
		driver->WriteData(frame + info, Math::Min(reservoir, bytes));

		SetMainDataOffset(frame, GetMainDataOffset(frame) + reservoir);

		/* Write next frame header and remaining bytes.
		 */
		if (bytes >= reservoir && frameBuffer.Size() >= info)
		{
			/* Update frame.
			 */
			memmove(frame + info, frame + info + reservoir, bytes - reservoir);

			bytes	  -= reservoir;
			reservoir  = GetFrameSize(frameBuffer) - info;

			/* Write buffered frame header.
			 */
			driver->WriteData(frameBuffer, info);

			memmove(frameBuffer, frameBuffer + info, frameBuffer.Size() - info);

			frameBuffer.Resize(frameBuffer.Size() - info);

			continue;
		}

		/* Update main data offset and compute total reservoir size.
		 */
		Int	 total = reservoir;

		for (Int i = 0; i < frameBuffer.Size(); i += info)
		{
			SetMainDataOffset(frame, GetMainDataOffset(frame) + GetFrameSize(frameBuffer + i) - info);

			total += GetFrameSize(frameBuffer + i) - info;
		}

		/* Adjust bitrate to stay under maximum reservoir size.
		 */
		SetBitrateIndex(frame, 14);

		while (GetFrameSize(frame) - info + total - bytes > maxR)
		{
			if (GetBitrateIndex(frame) == 1) break;

			SetBitrateIndex(frame, GetBitrateIndex(frame) - 1);
		}

		if (!GetPadding(frame) && GetFrameSize(frame) - info + total - bytes < maxR) SetPadding(frame, True);

		/* Increase reservoir if not enough bytes left.
		 */
		Int	 required = bytes - reservoir - (GetFrameSize(frame) - info);

		if (required > 0)
		{
			Int	 prevRes = reservoir;

			IncreaseReservoir(required);
			SetMainDataOffset(frame, GetMainDataOffset(frame) + reservoir - prevRes);

			driver->WriteData(frame + info + prevRes, reservoir - prevRes);
		}

		/* Process frame depending on reservoir size.
		 */
		if (bytes >= reservoir)
		{
			/* Write frame.
			 */
			driver->WriteData(frame, info);
			driver->WriteData(frame + info + reservoir, bytes - reservoir);

			reservoir += GetFrameSize(frame) - info - bytes;

			if (reservoir < 0) { reservoir = 0; return False; }
		}
		else
		{
			/* Write header to back buffer.
			 */
			frameBuffer.Resize(frameBuffer.Size() + info);

			memcpy(frameBuffer + frameBuffer.Size() - info, frame, info);

			reservoir -= bytes;
		}

		break;
	}

	return True;
}

Bool BoCA::SuperRepacker::Flush()
{
	return FillReservoir();
}

Bool BoCA::SuperRepacker::FillReservoir(Int threshold)
{
	/* Get total reservoir size including buffered frames.
	 */
	Int	 total = reservoir;

	if (frameBuffer.Size() > 0)
	{
		Int	 info = GetSideInfoLength(frameBuffer) + 4;

		for (Int i = 0; i < frameBuffer.Size(); i += info) total += GetFrameSize(frameBuffer + i) - info;
	}

	/* Fill superfluous reservoir with zero bytes.
	 */
	UnsignedByte	zero[1441] = { 0 };

	while (total > threshold)
	{
		driver->WriteData(zero, Math::Min(total - threshold, reservoir));

		if (total - threshold <= reservoir)
		{
			reservoir -= total - threshold;
			total	  -= total - threshold;
		}
		else
		{
			Int	 info = GetSideInfoLength(frameBuffer) + 4;

			total	  -= reservoir;
			reservoir  = GetFrameSize(frameBuffer) - info;

			/* Write buffered frame header.
			 */
			driver->WriteData(frameBuffer, info);

			memmove(frameBuffer, frameBuffer + info, frameBuffer.Size() - info);

			frameBuffer.Resize(frameBuffer.Size() - info);
		}
	}

	return True;
}

Bool BoCA::SuperRepacker::IncreaseReservoir(Int bytes)
{
	const Int	 maxFrameSize = 1441;

	/* Find last written frame.
	 */
	UnsignedByte	data[maxFrameSize];

	driver->Seek(driver->GetPos() - maxFrameSize);
	driver->ReadData(data, maxFrameSize);

	for (Int n = maxFrameSize - 13; n >= 0; n--)
	{
		if (!IsValidFrame(data + n)) continue;

		UnsignedByte	*frame = data + n;

		Int	 frameb	 = GetFrameSize(frame);
		Int	 nframeb = GetFrameSize(frame);

		if (frameb != maxFrameSize - n) continue;

		Int	 offset	 = driver->GetPos() - frameb;

		Int	 info	 = GetSideInfoLength(frame) + 4;
		Int	 pre	 = GetMainDataOffset(frame);

		Int	 maxR	 = GetMode(frame) == 3 ? 511 : 255;

		if (reservoir + bytes >= maxR) return False;

		Int	 prevRes = reservoir;

		/* Check if frame can be enlarged.
		 */
		Int	 brindex = GetBitrateIndex(frame);

		while (brindex < 14)
		{
			SetBitrateIndex(frame, ++brindex);

			nframeb = GetFrameSize(frame);

			if (nframeb - frameb >= bytes)
			{
				/* Write updated frame and update reservoir size.
				 */
				driver->Seek(offset);
				driver->WriteData(frame, frameb - prevRes);

				reservoir += nframeb - frameb;

				/* Fill superfluous reservoir with zero bytes.
				 */
				FillReservoir(maxR);

				driver->WriteData(frame + frameb - prevRes, prevRes);

				return True;
			}
		}

		/* Recursively try enlarging previous frames.
		 */
		driver->Seek(offset);

		reservoir = pre;

		Bool	 result	 = IncreaseReservoir(bytes - (nframeb - frameb));

		SetMainDataOffset(frame, reservoir);

		driver->WriteData(frame + info, reservoir - pre);
		driver->WriteData(frame, info);
		driver->WriteData(frame + info + reservoir - pre, frameb - info - (reservoir - pre) - prevRes);

		reservoir += prevRes - pre + nframeb - frameb;

		/* Fill superfluous reservoir with zero bytes.
		 */
		FillReservoir(maxR);

		driver->WriteData(frame + frameb - prevRes, prevRes);

		return result;
	}

	return False;
}
