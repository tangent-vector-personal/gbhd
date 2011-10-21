// Copyright 2011 Tim Foley. All rights reserved.
//
// memory.h

#ifndef gbemu_memory_h
#define gbemu_memory_h

#include "types.h"

class GPUState;
class Pad;
class TimerState;
class Z80State;

enum InterruptFlag
{
    kInterruptFlag_None         = 0x00,
    kInterruptFlag_VBlank       = 0x01,
    kInterruptFlag_LcdStatus    = 0x02,
    kInterruptFlag_Timer        = 0x04,
    kInterruptFlag_0x08         = 0x08,
    kInterruptFlag_0x10         = 0x10,
};

class MemoryState
{
public:
    void RaiseInterruptLine( InterruptFlag flag );
    void LowerInterruptLine( InterruptFlag flag );
    
    UInt8 GetInterruptFlags() { return interruptFlags; }
    void ClearInterruptFlag( InterruptFlag flag )
    {
        interruptFlags &= ~flag;
    }
    UInt8 interruptEnable;

private:
    UInt8 interruptFlags;
    UInt8 interruptLines;

    const UInt8* rom;
    UInt32 cartType;
    
    
    UInt8 wram[8192];
    UInt8 eram[32768];
    UInt8 zram[127];
    
//    UInt8 oam[1024];
    
    enum MapperType
    {
        kMapperType_MBC1,
    };
    MapperType mapperType;

    struct MBC1
    {
        UInt8 romBank;
        UInt8 ramBank;
        bool ramOn;
        UInt8 mode;
    };
    
    MBC1 mbc1;
    
    UInt32 romOffset;
    UInt32 ramOffset;
    
    Z80State* cpu;
    GPUState* gpu;
    Pad* pad;
    TimerState* timer;
        
public:
    MemoryState( const UInt8* rom );
    
    void SetCpu( Z80State* cpu ) { this->cpu = cpu; }
    void SetGpu( GPUState* gpu ) { this->gpu = gpu; }
    void SetPad( Pad* pad ) { this->pad = pad; }
    void SetTimer( TimerState* timer ) { this->timer = timer; }

    void Reset();
    
    UInt8 ReadUInt8( UInt16 addr );
    UInt8 ReadUInt8Impl( UInt16 addr );
    void WriteUInt8( UInt16 addr, UInt8 value );
};

#endif
