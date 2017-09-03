#ifndef AudioConverter_H
#define AudioConverter_H

#include "CoreAudioTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OpaqueAudioConverter;
typedef struct OpaqueAudioConverter *AudioConverterRef;
typedef UInt32 AudioConverterPropertyID;

struct AudioConverterPrimeInfo {
   UInt32 leadingFrames;
   UInt32 trailingFrames;
};
typedef struct AudioConverterPrimeInfo AudioConverterPrimeInfo;

enum {
   kAudioConverterPropertyMinimumInputBufferSize    = 'mibs',
   kAudioConverterPropertyMinimumOutputBufferSize   = 'mobs',
   kAudioConverterPropertyMaximumInputBufferSize    = 'xibs',
   kAudioConverterPropertyMaximumInputPacketSize    = 'xips',
   kAudioConverterPropertyMaximumOutputPacketSize   = 'xops',
   kAudioConverterPropertyCalculateInputBufferSize  = 'cibs',
   kAudioConverterPropertyCalculateOutputBufferSize = 'cobs',
   kAudioConverterPropertyInputCodecParameters      = 'icdp',
   kAudioConverterPropertyOutputCodecParameters     = 'ocdp',
   kAudioConverterSampleRateConverterAlgorithm      = 'srci',
   kAudioConverterSampleRateConverterComplexity     = 'srca',
   kAudioConverterSampleRateConverterQuality        = 'srcq',
   kAudioConverterSampleRateConverterInitialPhase   = 'srcp',
   kAudioConverterCodecQuality                      = 'cdqu',
   kAudioConverterPrimeMethod                       = 'prmm',
   kAudioConverterPrimeInfo                         = 'prim',
   kAudioConverterChannelMap                        = 'chmp',
   kAudioConverterDecompressionMagicCookie          = 'dmgc',
   kAudioConverterCompressionMagicCookie            = 'cmgc',
   kAudioConverterEncodeBitRate                     = 'brat',
   kAudioConverterEncodeAdjustableSampleRate        = 'ajsr',
   kAudioConverterInputChannelLayout                = 'icl ',
   kAudioConverterOutputChannelLayout               = 'ocl ',
   kAudioConverterApplicableEncodeBitRates          = 'aebr',
   kAudioConverterAvailableEncodeBitRates           = 'vebr',
   kAudioConverterApplicableEncodeSampleRates       = 'aesr',
   kAudioConverterAvailableEncodeSampleRates        = 'vesr',
   kAudioConverterAvailableEncodeChannelLayoutTags  = 'aecl',
   kAudioConverterCurrentOutputStreamDescription    = 'acod',
   kAudioConverterCurrentInputStreamDescription     = 'acid',
   kAudioConverterPropertySettings                  = 'acps',
   kAudioConverterPropertyBitDepthHint              = 'acbd',
   kAudioConverterPropertyFormatList                = 'flst',
};

enum {
   kAudioConverterQuality_Max     = 0x7F,
   kAudioConverterQuality_High    = 0x60,
   kAudioConverterQuality_Medium  = 0x40,
   kAudioConverterQuality_Low     = 0x20,
   kAudioConverterQuality_Min     = 0
};

enum {
   kAudioConverterSampleRateConverterComplexity_Linear     = 'line',
   kAudioConverterSampleRateConverterComplexity_Normal     = 'norm',
   kAudioConverterSampleRateConverterComplexity_Mastering  = 'bats',
};

typedef OSStatus (*AudioConverterComplexInputDataProc) (
   AudioConverterRef             inAudioConverter,
   UInt32                        *ioNumberDataPackets,
   AudioBufferList               *ioData,
   AudioStreamPacketDescription  **outDataPacketDescription,
   void                          *inUserData
);

#ifdef __cplusplus
}
#endif

#endif
