// Copyright 2011 Tim Foley. All rights reserved.
//
// memory.cpp
#include "memory.h"

#include <cstdio>
#include <cstring>

#include "cpu.h"
#include "gpu.h"
#include "pad.h"
#include "timer.h"

MemoryState::MemoryState( const UInt8* rom )
    : rom(rom)
{
    Reset();
}

void MemoryState::Reset()
{
    memset(wram, 0, sizeof(wram));
    memset(eram, 0, sizeof(eram));
    memset(zram, 0, sizeof(zram));
    
    interruptEnable = 0;
    // \todo: Make sure this makes sense... :(
    interruptFlags = 0x00;
    interruptLines = 0x00;
    
    memset(&mbc1, 0, sizeof(mbc1));
    mbc1.romBank = 1;
    
    romOffset = 0x4000;
    ramOffset = 0;
    
    cartType = rom[0x0147];
    switch( cartType )
    {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x08:
    case 0x09:
        mapperType = kMapperType_MBC1;
        break;
    default:
        throw 1;
        break;
    }
    
    LOG(MMU, "Reset");
}

void MemoryState::RaiseInterruptLine( InterruptFlag flag )
{
    // Trigger interrupt when line transitions low->high
    if( (interruptLines & flag) == 0 )
    {
        interruptFlags |= flag;
    }
    interruptLines |= flag;
}

void MemoryState::LowerInterruptLine( InterruptFlag flag )
{
    interruptLines &= ~flag;
}


UInt8 MemoryState::ReadUInt8( UInt16 addr )
{
    UInt8 value = ReadUInt8Impl( addr );
    Log( "Read: [0x%08x] = 0x%02x (%d)\n",
        addr, value, value );
    return value;
}
    
UInt8 MemoryState::ReadUInt8Impl( UInt16 addr )
{
    switch( addr & 0xf000 )
    {
    // ROM bank 0
    case 0x0000:
    case 0x1000:
    case 0x2000:
    case 0x3000:
        return rom[addr];
        
    // ROM bak 1
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
        return rom[romOffset + (addr & 0x3fff)];
        

    // VRAM
    case 0x8000:
    case 0x9000:
        return gpu->vram[addr & 0x1fff];
        
    // External RAM
    case 0xa000:
    case 0xb000:
        return eram[ramOffset + (addr & 0x1fff)];
        
    // Work RAM
    case 0xc000:
    case 0xd000:
    case 0xe000:
        return wram[addr & 0x1fff];
        
    case 0xf000:
        switch( addr & 0x0f00 )
        {
        // Work RAM (echo)
        case 0x000:
        case 0x100:
        case 0x200:
        case 0x300:
        case 0x400:
        case 0x500:
        case 0x600:
        case 0x700:
        case 0x800:
        case 0x900:
        case 0xA00:
        case 0xB00:
        case 0xC00:
        case 0xD00:
            return wram[addr & 0x1fff];
            
        // OAM
        case 0xe00:
            if( (addr & 0xff) < 0xa0 )
                return gpu->oam[addr & 0xff];
            else
                return 0;
                
        // Zeropage RAM, I/O, interrupts
        case 0xf00:
            if( addr == 0xffff ) return interruptEnable;
            else if( addr > 0xff7f ) return zram[addr & 0x7f];
            else switch( addr & 0xf0 )
            {
            case 0x00:
                switch( addr & 0xf )
                {
                case 0x0:
                    return pad->ReadUInt8();
                case 0x4:
                case 0x5:
                case 0x6:
                case 0x7:
                    return timer->ReadUInt8(addr);
                case 0xf:
                    return interruptFlags;
                default:
                    return 0;
                }
            case 0x10:
            case 0x20:
            case 0x30:
                return 0;
            case 0x40:
            case 0x50:
            case 0x60:
            case 0x70:
                return gpu->ReadUInt8(addr);
            }
        }
    }
    return 0;    
}

void MemoryState::WriteUInt8( UInt16 addr, UInt8 value )
{
    Log( "Write: [0x%08x] = 0x%02x (%d)\n",
        addr, value, value );

    switch( addr & 0xf000 )
    {
    // ROM bak 0
    case 0x0000:
    case 0x1000:
        switch( mapperType )
        {
        case kMapperType_MBC1:
            mbc1.ramOn = ((value & 0xf) == 0xa);
            break;
        }
        break;
    
    
    case 0x2000:
    case 0x3000:
        // MBC1: ROM bank switch
        switch( mapperType )
        {
        case kMapperType_MBC1:
            mbc1.romBank &= 0xe0;
            value &= 0x1f;
            if( !value ) value = 1;
            mbc1.romBank |= value;
            romOffset = mbc1.romBank * 0x4000;
            
            Log("Switching to ROM bank #%d [0x%04X]\n", mbc1.romBank, romOffset);
        }
        break;
    
    case 0x4000:
    case 0x5000:
        switch( mapperType )
        {
        case kMapperType_MBC1:
            if( mbc1.mode )
            {
                mbc1.ramBank = (value & 0x3);
                ramOffset = mbc1.ramBank * 0x2000;
                Log("Switching to RAM bank #%d [0x%04X]\n", mbc1.ramBank, ramOffset);
            }
            else
            {
                mbc1.ramBank = 0;
                ramOffset = mbc1.ramBank * 0x2000;
                Log("Switching to RAM bank #%d [0x%04X]\n", mbc1.ramBank, ramOffset);
                
                UInt8 data = ((value & 0x03) << 5);
                if( (mbc1.romBank & 0x1f) == 0 )
                {
                    data++;
                }
            
                mbc1.romBank &= 0x1f;
                mbc1.romBank |= data;
                romOffset = mbc1.romBank * 0x4000;
                Log("Switching to ROM bank #%d [0x%04X]\n", mbc1.romBank, romOffset);
            }
            break;
        }
        break;
    
    case 0x6000:
    case 0x7000:
        switch( mapperType )
        {
        case kMapperType_MBC1:
            mbc1.mode = value & 0x1;
            if( mbc1.mode )
            {
                mbc1.ramBank = 0;
                ramOffset = mbc1.ramBank * 0x2000;
                Log("Switching to RAM bank #%d [0x%04X]\n", mbc1.ramBank, ramOffset);
            }
            break;
        }
        break;

    // VRAM
    case 0x8000:
    case 0x9000:
        {
        /*
        if( gpu->vram[addr & 0x1FFF] != value )
        {
            static int count = 0;
            fprintf(stderr, "%d: VRAM[0x%04X] = 0x%02X\n", count++, addr, value);
        }
        */
        gpu->vram[addr & 0x1fff] = value;
        return;
        }
        
    // External RAM
    case 0xa000:
    case 0xb000:
        switch( mapperType )
        {
        case kMapperType_MBC1:
            if( mbc1.ramOn )
            {
                eram[ramOffset + (addr & 0x1fff)] = value;
            }
            break;
        }
        return;
        
    // Work RAM
    case 0xc000:
    case 0xd000:
    case 0xe000:
        wram[addr & 0x1fff] = value;
        return;
        
    case 0xf000:
        switch( addr & 0x0f00 )
        {
        // Work RAM (echo)
        case 0x000:
        case 0x100:
        case 0x200:
        case 0x300:
        case 0x400:
        case 0x500:
        case 0x600:
        case 0x700:
        case 0x800:
        case 0x900:
        case 0xA00:
        case 0xB00:
        case 0xC00:
        case 0xD00:
            wram[addr & 0x1fff] = value;
            return;
            
        // OAM
        case 0xe00:
            if( (addr & 0xff) < 0xa0 )
            {
                gpu->oam[addr & 0xff] = value;
            }
            break;
                
        // Zeropage RAM, I/O, interrupts
        case 0xf00:
            if( addr == 0xffff )
            {
                interruptEnable = value;
                Log("interrupt mask = 0x%02X\n", interruptEnable);
                return;
            }
            else if( addr > 0xff7f )
            {
                zram[addr & 0x7f] = value;
                return;
            }
            else switch( addr & 0xf0 )
            {
            case 0x00:
                switch( addr & 0xf )
                {
                case 0x0:
                    pad->WriteUInt8( value );
                    break;
                case 0x4:
                case 0x5:
                case 0x6:
                case 0x7:
                    timer->WriteUInt8(addr, value);
                    break;
                case 0xf:
                    interruptFlags = value;
                    break;
                default:
                    break;
                }
            case 0x10:
            case 0x20:
            case 0x30:
                break;
            case 0x40:
            case 0x50:
            case 0x60:
            case 0x70:
                gpu->WriteUInt8(addr, value);
                break;
            }
        }
    }
}
