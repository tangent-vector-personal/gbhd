// Copyright 2011 Tim Foley. All rights reserved.
//
// options.h

#ifndef GBHD_OPTIONS_H
#define GBHD_OPTIONS_H

#include  <string>

//
// The Options class provides a very simple implementation of command-line
// option parsing, and is also used to store certain bits of global
// information (like the name of the currently-loaded game).
//
class Options
{
public:
    Options();
    
    void Parse( int argCount, char const * const * args );
    
    std::string inputFileName;
    std::string rawGameName;
    std::string prettyGameName;
    std::string mediaPath;
    bool dumpTiles;
    
private:
    void ParseLongOptionFlag( const char* flag, int* ioArgIndex, int argCount, char const* const* args );
    void ParseShortOptionFlag( const char* flag, int* ioArgIndex, int argCount, char const* const* args );
    void ParseArg( const char* arg );
};


#endif // GBHD_OPTIONS_H
