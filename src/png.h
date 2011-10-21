// Copyright 2011 Tim Foley. All rights reserved.
//
// png.h

#ifndef GBHD_PNG_H
#define GBHD_PNG_H

#ifdef _MSC_VER

#include <png.h>
#pragma comment(lib, "libpng15.lib")

#elif defined(__APPLE__)

#include <libpng/png.h>

#else
#error "Unknown Compiler/Platform"
#endif

#endif // GBHD_PNG_H
