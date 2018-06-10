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

#ifdef __WIN32__
#	include <windows.h>
#	include <shlobj.h>
#endif

using namespace BoCA;

#ifndef __APPLE__
using namespace CA;

namespace CA
{
	AUDIOFILEINITIALIZEWITHCALLBACKS	 AudioFileInitializeWithCallbacks	= NIL;
	AUDIOFILECLOSE				 AudioFileClose				= NIL;
	AUDIOFILESETPROPERTY			 AudioFileSetProperty			= NIL;
	AUDIOFILEWRITEPACKETS			 AudioFileWritePackets			= NIL;

	AUDIOCONVERTERNEW			 AudioConverterNew			= NIL;
	AUDIOCONVERTERDISPOSE			 AudioConverterDispose			= NIL;
	AUDIOCONVERTERGETPROPERTY		 AudioConverterGetProperty		= NIL;
	AUDIOCONVERTERGETPROPERTYINFO		 AudioConverterGetPropertyInfo		= NIL;
	AUDIOCONVERTERSETPROPERTY		 AudioConverterSetProperty		= NIL;
	AUDIOCONVERTERFILLCOMPLEXBUFFER		 AudioConverterFillComplexBuffer	= NIL;

	AUDIOFORMATGETPROPERTY			 AudioFormatGetProperty			= NIL;
	AUDIOFORMATGETPROPERTYINFO		 AudioFormatGetPropertyInfo		= NIL;
};

DynamicLoader *coreaudiodll	= NIL;

#ifdef __WIN32__
String GetCommonFilesDirectory()
{
	String	 commonFilesDir;

	ITEMIDLIST	*idlist;
	Buffer<wchar_t>	 buffer(32768 + 1);

	SHGetSpecialFolderLocation(NIL, CSIDL_PROGRAM_FILES_COMMON, &idlist);
	SHGetPathFromIDList(idlist, buffer);

	commonFilesDir = buffer;

	CoTaskMemFree(idlist);

	if (commonFilesDir != NIL && !commonFilesDir.EndsWith(Directory::GetDirectoryDelimiter())) commonFilesDir.Append(Directory::GetDirectoryDelimiter());

	return commonFilesDir;
}
#endif

Bool LoadCoreAudioDLL()
{
#	ifdef __WIN32__
	String	 aasDir = GetCommonFilesDirectory().Append("Apple\\Apple Application Support\\");

	/* Add Apple Application Services directory to path.
	 */
	char	*buffer = new char [32768];

	GetEnvironmentVariableA("PATH", buffer, 32768);
	SetEnvironmentVariableA("PATH", String(buffer).Append(";").Append(aasDir));

	delete [] buffer;

	coreaudiodll	  = new DynamicLoader(String(aasDir).Append("CoreAudioToolbox.dll"));
#	endif

	if (coreaudiodll == NIL) return False;

	AudioFileInitializeWithCallbacks	= (AUDIOFILEINITIALIZEWITHCALLBACKS) coreaudiodll->GetFunctionAddress("AudioFileInitializeWithCallbacks");
	AudioFileClose				= (AUDIOFILECLOSE) coreaudiodll->GetFunctionAddress("AudioFileClose");
	AudioFileSetProperty			= (AUDIOFILESETPROPERTY) coreaudiodll->GetFunctionAddress("AudioFileSetProperty");
	AudioFileWritePackets			= (AUDIOFILEWRITEPACKETS) coreaudiodll->GetFunctionAddress("AudioFileWritePackets");

	AudioConverterNew			= (AUDIOCONVERTERNEW) coreaudiodll->GetFunctionAddress("AudioConverterNew");
	AudioConverterDispose			= (AUDIOCONVERTERDISPOSE) coreaudiodll->GetFunctionAddress("AudioConverterDispose");
	AudioConverterGetProperty		= (AUDIOCONVERTERGETPROPERTY) coreaudiodll->GetFunctionAddress("AudioConverterGetProperty");
	AudioConverterGetPropertyInfo		= (AUDIOCONVERTERGETPROPERTYINFO) coreaudiodll->GetFunctionAddress("AudioConverterGetPropertyInfo");
	AudioConverterSetProperty		= (AUDIOCONVERTERSETPROPERTY) coreaudiodll->GetFunctionAddress("AudioConverterSetProperty");
	AudioConverterFillComplexBuffer		= (AUDIOCONVERTERFILLCOMPLEXBUFFER) coreaudiodll->GetFunctionAddress("AudioConverterFillComplexBuffer");

	AudioFormatGetProperty			= (AUDIOFORMATGETPROPERTY) coreaudiodll->GetFunctionAddress("AudioFormatGetProperty");
	AudioFormatGetPropertyInfo		= (AUDIOFORMATGETPROPERTYINFO) coreaudiodll->GetFunctionAddress("AudioFormatGetPropertyInfo");

	if (AudioFileInitializeWithCallbacks	== NIL ||
	    AudioFileClose			== NIL ||
	    AudioFileSetProperty		== NIL ||
	    AudioFileWritePackets		== NIL ||

	    AudioConverterNew			== NIL ||
	    AudioConverterDispose		== NIL ||
	    AudioConverterGetProperty		== NIL ||
	    AudioConverterGetPropertyInfo	== NIL ||
	    AudioConverterSetProperty		== NIL ||
	    AudioConverterFillComplexBuffer	== NIL ||

	    AudioFormatGetProperty		== NIL ||
	    AudioFormatGetPropertyInfo		== NIL) { FreeCoreAudioDLL(); return False; }

	return True;
}

Void FreeCoreAudioDLL()
{
	BoCA::Utilities::FreeCodecDLL(coreaudiodll);

	coreaudiodll	  = NIL;
}
#endif
