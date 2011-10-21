// Copyright 2011 Tim Foley. All rights reserved.
//
// types.h

#ifndef gbemu_type_h
#define gbemu_type_h

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef int8_t SInt8;
typedef uint8_t UInt8;
typedef int16_t SInt16;
typedef uint16_t UInt16;
typedef int32_t SInt32;
typedef uint32_t UInt32;

extern FILE* gLogFile;

static void Log(
    const char* format,
    ... )
{
    if( gLogFile == NULL )
        return;

    va_list args;
    va_start(args, format);
    vfprintf(gLogFile, format, args);
    va_end(args);
}

#define LOG(area, message) \
    Log("%s: %s\n", #area, message)

#endif
