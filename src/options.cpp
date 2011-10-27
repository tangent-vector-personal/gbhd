// Copyright 2011 Tim Foley. All rights reserved.
//
// options.cpp
#include "options.h"

#include <cassert>

Options::Options()
    : dumpTilesOnce(false)
{}

typedef void (*OptionFunc)(Options* options, const char* arg);

static void MediaPathFunc( Options* options, const char* arg )
{
    options->mediaPath = arg;
}

static const struct
{
    const char* longFlag;
    const char* shortFlag;
    int argCount;
    OptionFunc func;
} kOptionFlags[] = {
    { "media-path", NULL, 0, &MediaPathFunc },
    { NULL, NULL, 0, NULL },
};

void Options::Parse( int argCount, char const * const * args )
{
    int ii = 1;
    while( ii < argCount )
    {
        char const* arg = args[ii++];
        
        if( arg[0] == '-' )
        {
            // possibly an option
            
            if( arg[1] == '-' )
            {
                // possibly a long option
                if( arg[2] == 0 )
                {
                    // Option was "--", usually refers to stdin
                    fprintf(stderr, "Unexpected option %s\n", arg);
                }
                else
                {
                    // Look up the appropriate option
                    ParseLongOptionFlag( arg + 2, &ii, argCount, args );
                }
            }
            else
            {
                // seems to be a short option
                ParseShortOptionFlag( arg + 1, &ii, argCount, args );
            }
        }
        else
        {
            // appears to be a non-option argument
            ParseArg( arg );
        }
    }
    
    if( inputFileName.length() == 0 )
    {
        fprintf(stderr, "No input file specified\n");
    }
}

void Options::ParseLongOptionFlag( const char* flag, int* ioArgIndex, int argCount, char const* const* args )
{
    assert( ioArgIndex != NULL );
    int& argIndex = *ioArgIndex;
    
    for( int ii = 0; kOptionFlags[ii].func != NULL; ++ii )
    {
        if( kOptionFlags[ii].longFlag == NULL ) continue;
        if( strcmp(flag, kOptionFlags[ii].longFlag) != 0 ) continue;
        
        int count = kOptionFlags[ii].argCount;
        if( argIndex + count > argCount )
        {
            fprintf(stderr, "Missing argument for option --%s\n", flag);
            return;
        }
        
        assert( count <= 1 );
        
        const char* arg = "";
        if( count > 0 )
        {
            arg = args[argIndex++];
        }
        
        kOptionFlags[ii].func( this, arg );
        return;
    }
    fprintf(stderr, "Unexpected option --%s\n", flag);
}

void Options::ParseShortOptionFlag( const char* flag, int* ioArgIndex, int argCount, char const* const* args )
{
    assert( ioArgIndex != NULL );
    int& argIndex = *ioArgIndex;
    
    for( int ii = 0; kOptionFlags[ii].func != NULL; ++ii )
    {
        if( kOptionFlags[ii].shortFlag == NULL ) continue;
        if( strcmp(flag, kOptionFlags[ii].shortFlag) != 0 ) continue;
        
        int count = kOptionFlags[ii].argCount;
        if( argIndex + count > argCount )
        {
            fprintf(stderr, "Missing argument for option -%s\n", flag);
            return;
        }
        
        assert( count <= 1 );
        
        const char* arg = "";
        if( count > 0 )
        {
            arg = args[argIndex++];
        }
        
        kOptionFlags[ii].func( this, arg );
        return;
    }
    // \todo: Handle the case of "bundled-up" flags?
    fprintf(stderr, "Unexpected option -%s\n", flag);
}

void Options::ParseArg( const char* arg )
{
    if( inputFileName.length() != 0 )
    {
        fprintf(stderr, "Unexpected option %s\n", arg);
        return;
    }
    inputFileName = arg;
}
