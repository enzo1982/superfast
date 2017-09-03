#ifndef CoreFoundation_H__
#define CoreFoundation_H__

#include "MacTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef UInt32 CFTypeID;
typedef SInt32 CFIndex;
typedef UInt32 CFHashCode;
typedef const void * CFTypeRef;
typedef const struct __CFAllocator * CFAllocatorRef;
typedef const struct __CFString * CFStringRef;
typedef struct __CFString * CFMutableStringRef;
typedef const struct __CFDictionary * CFDictionaryRef;
typedef struct __CFDictionary * CFMutableDictionaryRef;
typedef const struct __CFArray * CFArrayRef;
typedef struct __CFArray * CFMutableArrayRef;
typedef const struct __CFURL * CFURLRef;
typedef const struct __CFAllocator * CFAllocatorRef;

typedef struct {
    CFIndex location;
    CFIndex length;
} CFRange;

typedef const void * (*CFDictionaryRetainCallBack)(CFAllocatorRef allocator, const void *value);
typedef void (*CFDictionaryReleaseCallBack)(CFAllocatorRef allocator, const void *value);
typedef CFStringRef (*CFDictionaryCopyDescriptionCallBack)(const void *value);
typedef Boolean (*CFDictionaryEqualCallBack)(const void *value1, const void *value2);
typedef CFHashCode (*CFDictionaryHashCallBack)(const void *value);
typedef void (*CFDictionaryApplierFunction)(const void *key, const void *value, void *context);

typedef struct {
    CFIndex                             version;
    CFDictionaryRetainCallBack          retain;
    CFDictionaryReleaseCallBack         release;
    CFDictionaryCopyDescriptionCallBack copyDescription;
    CFDictionaryEqualCallBack           equal;
    CFDictionaryHashCallBack            hash;
} CFDictionaryKeyCallBacks;

typedef struct {
    CFIndex                             version;
    CFDictionaryRetainCallBack          retain;
    CFDictionaryReleaseCallBack         release;
    CFDictionaryCopyDescriptionCallBack copyDescription;
    CFDictionaryEqualCallBack           equal;
} CFDictionaryValueCallBacks;

typedef enum {
    kCFURLPOSIXPathStyle = 0,
    kCFURLHFSPathStyle,
    kCFURLWindowsPathStyle
} CFURLPathStyle;

typedef enum  {
    kCFStringEncodingMacRoman = 0,
    kCFStringEncodingWindowsLatin1 = 0x0500,
    kCFStringEncodingISOLatin1 = 0x0201,
    kCFStringEncodingNextStepLatin = 0x0B01,
    kCFStringEncodingASCII = 0x0600,
    kCFStringEncodingUnicode = 0x0100,
    kCFStringEncodingUTF8 = 0x08000100,
    kCFStringEncodingNonLossyASCII = 0x0BFF,
   
    kCFStringEncodingUTF16 = 0x0100,
    kCFStringEncodingUTF16BE = 0x10000100,
    kCFStringEncodingUTF16LE = 0x14000100,
    kCFStringEncodingUTF32 = 0x0c000100,
    kCFStringEncodingUTF32BE = 0x18000100,
    kCFStringEncodingUTF32LE = 0x1c000100
} CFStringEncoding;

#ifdef __cplusplus
}
#endif

#endif
