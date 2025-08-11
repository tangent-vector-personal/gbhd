// Copyright 2011 Theresa Foley. All rights reserved.
//
// pad.cpp
#include "pad.h"

//
// The keys on the Game Boy are read through 4 bits of a single 8-bit hardware
// register. Since there are 8 keys, it is not possible to check all of the
// keys in a single read.
//
// By writing to the same hardware register, an application can switch the
// "column" of keys that will be scanned. This switch takes several cycles
// to "stabilize" on real hardware, but we do not emulate this delay.
//

Pad::Pad()
{
    Reset();
}

void Pad::Reset()
{
    keys[0] = 0x0F;
    keys[1] = 0x0F;
    colIdx = 0x00;
    LOG(Pad, "Reset");
}


UInt8 Pad::ReadUInt8()
{
    switch( colIdx )
    {
    case 0x00: return 0x00; break;
    case 0x10: return keys[0]; break;
    case 0x20: return keys[1]; break;
    default: return 0x00; break;
    }
}

void Pad::WriteUInt8( UInt8 value )
{
    colIdx = value & 0x30;
}

void Pad::KeyDown( Key key )
{
    switch( key )
    {
    case kKey_A:        keys[0] &= ~0x01; break;
    case kKey_B:        keys[0] &= ~0x02; break;
    case kKey_Select:   keys[0] &= ~0x04; break;
    case kKey_Start:    keys[0] &= ~0x08; break;

    case kKey_Right:    keys[1] &= ~0x01; break;
    case kKey_Left:     keys[1] &= ~0x02; break;
    case kKey_Up:       keys[1] &= ~0x04; break;
    case kKey_Down:     keys[1] &= ~0x08; break;
    }
}

void Pad::KeyUp( Key key )
{
    switch( key )
    {
    case kKey_A:        keys[0] |= 0x01; break;
    case kKey_B:        keys[0] |= 0x02; break;
    case kKey_Select:   keys[0] |= 0x04; break;
    case kKey_Start:    keys[0] |= 0x08; break;

    case kKey_Right:    keys[1] |= 0x01; break;
    case kKey_Left:     keys[1] |= 0x02; break;
    case kKey_Up:       keys[1] |= 0x04; break;
    case kKey_Down:     keys[1] |= 0x08; break;
    }
}
