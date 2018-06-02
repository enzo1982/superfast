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
#include "framecrc.h"

namespace BoCA
{
	static const Int	 maxFrameSize	   = 1441;

	static const Int	 bitrates[4][16]   = { { 0,  8000, 16000, 24000, 32000, 40000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 },   // MPEG 2.5
						       { 0,	0,     0,     0,     0,	    0,	   0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0, 0 },   // reserved
						       { 0,  8000, 16000, 24000, 32000, 40000, 48000, 56000,  64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 },   // MPEG 2
						       { 0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 } }; // MPEG 1

	static const Int	 samplerates[4][4] = { { 11025, 12000,  8000, 0 },   // MPEG 2.5
						       {     0,	    0,	   0, 0 },   // reserved
						       { 22050, 24000, 16000, 0 },   // MPEG 2
						       { 44100, 48000, 32000, 0 } }; // MPEG 1

	static Int GetMode(const UnsignedByte *frame)
	{
		return (frame[1] >> 3) & 0x03;
	}

	static Int GetBitrateIndex(const UnsignedByte *frame)
	{
		return frame[2] >> 4;
	}

	static Void SetBitrateIndex(UnsignedByte *frame, Int brindex)
	{
		frame[2] = (brindex << 4) | (frame[2] & 0x0F);
	}

	static Int GetSampleRateIndex(const UnsignedByte *frame)
	{
		return (frame[2] >> 2) & 0x03;
	}

	static Bool GetPadding(const UnsignedByte *frame)
	{
		return (frame[2] >> 1) & 0x01;
	}

	static Void SetPadding(UnsignedByte *frame, Bool padding)
	{
		frame[2] = (padding ? 0x02 : 0x00) | (frame[2] & 0xFD);
	}

	static Int GetFrameSize(const UnsignedByte *frame)
	{
		Int	 mode	 = GetMode(frame);
		Int	 brindex = GetBitrateIndex(frame);
		Int	 srindex = GetSampleRateIndex(frame);

		Int	 bitrate = bitrates[mode][brindex];
		Int	 srate	 = samplerates[mode][srindex];

		Int	 padding = GetPadding(frame);

		return (mode == 3 ? 144 : 72) * bitrate / srate + padding;
	}

	static Int GetHeaderLength(const UnsignedByte *frame)
	{
		if (frame[1] & 0x01) return 4;

		return 6;
	}

	static Int GetSideInfoLength(const UnsignedByte *frame)
	{
		return GetMode(frame) == 3 ? ((frame[3] >> 6) == 0x03 ? 17 : 32) :
					     ((frame[3] >> 6) == 0x03 ?  9 : 17);
	}

	static Int GetMainDataOffset(const UnsignedByte *frame)
	{
		Int	 hl = GetHeaderLength(frame);

		return GetMode(frame) == 3 ? (frame[hl] << 1) | (frame[hl + 1] >> 7) :
					      frame[hl];
	}

	static Void SetMainDataOffset(UnsignedByte*frame, Int offset)
	{
		Int	 hl = GetHeaderLength(frame);

		if (GetMode(frame) == 3)
		{
			frame[hl    ] = offset >> 1;
			frame[hl + 1] = ((offset & 1) << 7) | (frame[hl + 1] & 0x7F);
		}
		else
		{
			frame[hl    ] = offset;
		}
	}

	static Int GetMainDataLength(const UnsignedByte *frame)
	{
		Int	 hl   = GetHeaderLength(frame);
		Int	 bits = 0;

		if (GetMode(frame) != 3)
		{
			if (GetSideInfoLength(frame) == 9)
			{
				bits += ((frame[hl +  1] & 0x7F) <<  5) | (frame[hl +  2] >> 3);
			}
			else
			{
				bits += ((frame[hl +  1] & 0x3F) <<  6) | (frame[hl +  2] >> 2);
				bits += ((frame[hl +  9] & 0x7F) <<  5) | (frame[hl + 10] >> 3);
			}
		}
		else
		{
			if (GetSideInfoLength(frame) == 17)
			{
				bits += ((frame[hl +  2] & 0x3F) <<  6) | (frame[hl +  3] >> 2)			       ;
				bits += ((frame[hl +  9] & 0x07) <<  9) | (frame[hl + 10] << 1) | (frame[hl + 11] >> 7);
			}
			else
			{
				bits += ((frame[hl +  2] & 0x0F) <<  8) | (frame[hl +  3]     )			       ;
				bits += ((frame[hl +  9] & 0x01) << 11) | (frame[hl + 10] << 3) | (frame[hl + 11] >> 5);
				bits += ((frame[hl + 17] & 0x3F) <<  6) | (frame[hl + 18] >> 2)			       ;
				bits += ((frame[hl + 24] & 0x07) <<  9) | (frame[hl + 25] << 1) | (frame[hl + 26] >> 7);
			}
		}

		return Math::Ceil(Float(bits) / 8);
	}

	static Bool IsValidFrame(const UnsignedByte *frame)
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

	cbrIndex   = -1;

	minIndex   = 1;
	maxIndex   = 14;
}

BoCA::SuperRepacker::~SuperRepacker()
{
}

Bool BoCA::SuperRepacker::EnableRateControl(Int minRate, Int maxRate)
{
	if (maxRate < minRate) return False;

	minIndex = -minRate;
	maxIndex = -maxRate;

	return True;
}

Bool BoCA::SuperRepacker::UpdateInfoTag(Buffer<UnsignedByte> &frame, Int64 totalSamples) const
{
	if (frame.Size() < 169) return False;

	UnsignedByte	*tag = frame + 4 + GetSideInfoLength(frame);

	/* Set frame count (minus this info frame).
	 */
	UnsignedInt32	 frames = frameCount - 1;

	tag[0x08] =  frames >> 24;
	tag[0x09] = (frames >> 16) & 0xFF;
	tag[0x0A] = (frames >>  8) & 0xFF;
	tag[0x0B] =  frames	   & 0xFF;

	/* Set byte count.
	 */
	UnsignedInt32	 bytes = driver->GetSize() - offset;

	tag[0x0C] =  bytes  >> 24;
	tag[0x0D] = (bytes  >> 16) & 0xFF;
	tag[0x0E] = (bytes  >>  8) & 0xFF;
	tag[0x0F] =  bytes 	   & 0xFF;

	/* Write TOC.
	 */
	for (Int i = 0; i < 100; i++)
	{
		UnsignedInt32	 offset = frameOffsets.GetNth(frameOffsets.Length() * i / 100);

		tag[0x10 + i] = Math::Min(Int64(255), Math::Floor(256.0 * offset / bytes));
	}

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

	/* Calculate music CRC.
	 */
	Hash::CRC16		 hash;

	driver->Seek(offset + GetFrameSize(frame));

	Buffer<UnsignedByte>	 buffer(256 * 1024);
	Int64			 bytesLeft = driver->GetSize() - driver->GetPos();

	while (bytesLeft)
	{
		Int	 bytes = driver->ReadData(buffer, Math::Min(bytesLeft, Int64(256 * 1024)));

		hash.Feed(buffer, bytes);

		bytesLeft -= bytes;
	}

	/* Set music CRC.
	 */
	UnsignedInt16	 musicCRC = hash.Finish();

	tag[0x98] = musicCRC >> 8;
	tag[0x99] = musicCRC	  & 0xFF;

	/* Set info CRC.
	 */
	UnsignedInt16	 tagCRC	  = Hash::CRC16::Compute(frame, 4 + GetSideInfoLength(frame) + 0x9A);

	tag[0x9A] = tagCRC   >> 8;
	tag[0x9B] = tagCRC	  & 0xFF;

	return True;
}

Bool BoCA::SuperRepacker::UnpackFrames(const Buffer<UnsignedByte> &data, Buffer<UnsignedByte> &packets, Array<Int> &packetSizes)
{
	Buffer<UnsignedByte>	 main;

	for (Int n = 0; n < data.Size(); n++)
	{
		if (!IsValidFrame(data + n)) continue;

		UnsignedByte	*frame = data + n;

		Int	 frameb	= GetFrameSize(frame);

		Int	 info	= GetHeaderLength(frame) + GetSideInfoLength(frame);
		Int	 bytes	= GetMainDataLength(frame);
		Int	 pre	= GetMainDataOffset(frame);

		/* Set CBR bitrate index.
		 */
		if	(cbrIndex == -1)				       cbrIndex = GetBitrateIndex(frame);
		else if (cbrIndex !=  0 && cbrIndex != GetBitrateIndex(frame)) cbrIndex = 0;

		/* Set minimum/maximum VBR bitrate index.
		 */
		Int	 mode	= GetMode(frame);

		if (minIndex < 0) for (Int i =	1; i <= 14; i++) if (bitrates[mode][i] >= -minIndex || i == 14) { minIndex = i; break; }
		if (maxIndex < 0) for (Int i = 14; i >=	 1; i--) if (bitrates[mode][i] <= -maxIndex || i ==  1) { maxIndex = i; break; }

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

		SetBitrateIndex(packets + offset, cbrIndex);
		SetPadding(packets + offset, False);
		SetMainDataOffset(packets + offset, 0);

		packetSizes.Add(size);

		n += frameb - 1;
	}

	return True;
}

Bool BoCA::SuperRepacker::WriteFrame(UnsignedByte *iFrame, Int size)
{
	UnsignedByte	 frame[maxFrameSize + 511] = { 0 }; // Frame can exceed maxFrameSize when using reservoir.

	if (frameCount++ == 0 && memcmp(iFrame + GetHeaderLength(iFrame), frame, GetSideInfoLength(iFrame)) == 0) { driver->WriteData(iFrame, size); return True; }

	memcpy(frame, iFrame, size);

	Int	 info  = GetHeaderLength(frame) + GetSideInfoLength(frame);
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
			frameOffsets.Add(driver->GetPos() - offset);

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
		if (!cbrIndex)
		{
			SetBitrateIndex(frame, maxIndex);

			while (GetFrameSize(frame) - info + total - bytes > maxR)
			{
				if (GetBitrateIndex(frame) == minIndex) break;

				SetBitrateIndex(frame, GetBitrateIndex(frame) - 1);
			}
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

		/* Compute frame CRC.
		 */
		if (GetHeaderLength(frame) == 6) FrameCRC::Update(frame);

		/* Process frame depending on reservoir size.
		 */
		if (bytes >= reservoir)
		{
			/* Write frame.
			 */
			frameOffsets.Add(driver->GetPos() - offset);

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
		Int	 info = GetHeaderLength(frameBuffer) + GetSideInfoLength(frameBuffer);

		for (Int i = 0; i < frameBuffer.Size(); i += info) total += GetFrameSize(frameBuffer + i) - info;
	}

	/* Fill superfluous reservoir with zero bytes.
	 */
	UnsignedByte	zero[maxFrameSize] = { 0 };

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
			Int	 info = GetHeaderLength(frameBuffer) + GetSideInfoLength(frameBuffer);

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

		Int	 info	 = GetHeaderLength(frame) + GetSideInfoLength(frame);
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
				/* Compute frame CRC.
				 */
				if (GetHeaderLength(frame) == 6) FrameCRC::Update(frame);

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

		/* Compute frame CRC.
		 */
		if (GetHeaderLength(frame) == 6) FrameCRC::Update(frame);

		/* Write modified frame.
		 */
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
