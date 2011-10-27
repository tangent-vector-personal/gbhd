// Copyright 2011 Tim Foley. All rights reserved.
//
// pad.h

#ifndef gbemu_pad_h
#define gbemu_pad_h

#include "keys.h"
#include "types.h"


//
// The Pad class defines an interface that both the application and the
// emulator core may use. The application registers key up/down events
// using the Key{Up|Down}() functions, and the emulated program reads
// and writes the state of the key-related hardware registers using
// the {Read|Write}UInt8() functions.
//
class Pad
{
public:
    Pad();
    
    void Reset();

    UInt8 ReadUInt8();
    void WriteUInt8( UInt8 value );
        
    void KeyDown( Key key );
    void KeyUp( Key key );
    
private:
    UInt8 keys[2];
    UInt8 colIdx;
};


#endif
