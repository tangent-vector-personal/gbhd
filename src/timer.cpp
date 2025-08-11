// Copyright 2011 Theresa Foley. All rights reserved.
//
// timer.cpp
#include "timer.h"

#include "memory.h"

//
// The timer is responsible for four hardware registers:
//
// DIV  - The 8-bit divider register, which increments every 256 cycles
//        (kDivLimit) and can be reset under application control. This can be
//        used as a source of pseudo-randomness by the application.
//
// TIMA - The 8-bit timer accumulator. When it overflows, it triggers an
//        interrupt. It can be set to update at various divisiors of the
//        native clock frequency (see kTimaLimits).
//
// TMA  - An 8-bit register that specifies the value to reset TIMA to
//        when TIMA overflows. This allows the application to adjust
//        the frequency of timer interrupts.
//
// TAC  - An 8-bit register that controls the timer (TIMA) register.
//        It has a flag (kTacFlag_Enable) that controls whether the
//        timer advances, and a two-bit field (kTacMask_Mode) that
//        controls the rate at chich TIMA increments (kTimaLimits).
//
// The implementation of the timer introduces two hidden pieces of state
// in addition to these registers: divCounter and timaCounter. These
// counters are advanced every cycle (or rather, every cycle when the
// timer is active, for timaCounter) and when the hit their limit value
// they are reset and the corresponding register is incremented.
//

TimerState::TimerState(
    MemoryState* memory )
    : memory(memory)
{
    Reset();
}

static const int kTimaLimits[] = {
    1024,
    16,
    64,
    256,
};

void TimerState::Reset()
{
    div = 0;
    tima = 0;
    tma = 0;
    tac = 0;

    divCounter = 0;
    timaCounter = 0;
    
    LOG(Timer, "Reset");
}

void TimerState::Inc( UInt8 amount )
{
    // Deal with divider register
    divCounter += amount;
    if( divCounter >= kDivLimit )
    {
        divCounter = 0;
        div++;
    }

    // Now deal with timer register
    if( tac & kTacFlag_Enable )
    {
        timaCounter += amount;
        
        int timaLimit = kTimaLimits[tac & kTacMask_Mode];
        if( timaCounter >= timaLimit )
        {
            timaCounter = 0; // \todo: Do -= timaLimit?
            if( tima == 255 )
            {
                tima = tma;
                memory->RaiseInterruptLine(kInterruptFlag_Timer);
                memory->LowerInterruptLine(kInterruptFlag_Timer);
//                static int count = 0;
//                fprintf(stderr, "Timer #%d\n", count++);               
            }
            else
            {
                tima++;
            }
        }
    }
}

UInt8 TimerState::ReadUInt8( UInt16 addr )
{
    switch( addr )
    {
    case 0xff04: return div;
    case 0xff05: return tima;
    case 0xff06: return tma;
    case 0xff07: return tac;
    default:
        throw 1;
    }
}

void TimerState::WriteUInt8( UInt16 addr, UInt8 value )
{
    switch( addr )
    {
    case 0xff04: div = 0; divCounter = 0; break;
    case 0xff05: tima = value; break;
    case 0xff06: tma = value; break;
    case 0xff07:
        {
            if( (tac & kTacMask_Mode) != (value & kTacMask_Mode) )
            {
                // When switching modes, reset the cylce counter
                timaCounter = 0;
            }
            tac = value & kTacMask_All;
        }
        break;
    }
}
