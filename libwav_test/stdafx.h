// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:


// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <windows.h>

// TODO: reference additional headers your program requires here
#include "../libwav/libwav.h"
#ifdef _DEBUG	//vs
#pragma comment(lib, "../Debug/libwav.lib")
#else
#pragma comment(lib, "../Release/libwav.lib")
#endif

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#include <sstream>
#include <string>