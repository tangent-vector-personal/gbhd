// Copyright 2011 Tim Foley. All rights reserved.
//
// png.h

#ifndef GBHD_PNG_H
#define GBHD_PNG_H

#ifdef _MSC_VER

#include <png.h>
#pragma comment(lib, "libpng15.lib")

#else
#error "Unknown Compiler/Platform"
#endif

#endif // GBHD_PNG_H
