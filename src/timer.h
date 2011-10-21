// Copyright 2011 Tim Foley. All rights reserved.
//
// timer.h

#ifndef GBHD_TIMER_H
#define GBHD_TIMER_H

#include "types.h"

class MemoryState;

//
// The TimerState class handles the timer-related hardware registers
// in the Game Boy. It is advanced using the Inc() function whenever
// we process an instruction, and the values of its registers can
// be read/written with {Read|Write}UInt8().
//
class TimerState
{
public:
    TimerState(
        MemoryState* memory );

    void Reset();
    void Inc( UInt8 amount );
    
    UInt8 ReadUInt8( UInt16 addr );
    void WriteUInt8( UInt16 addr, UInt8 value );

private:
    UInt8 div;
    UInt8 tima;
    UInt8 tma;
    UInt8 tac;
    
    
    int divCounter;
    enum
    {
        kDivLimit = 256,
        kTacFlag_Enable = 0x04,
        kTacMask_Mode = 0x03,
        kTacMask_All = kTacFlag_Enable | kTacMask_Mode,
    };
    
    int timaCounter;
    
    MemoryState* memory;
};

#endif // GBHD_TIMER_H
