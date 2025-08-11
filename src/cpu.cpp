// Copyright 2011 Theresa Foley. All rights reserved.
//
// cpu.cpp
#include "cpu.h"

#include <cassert>
#include <cstdio>

#define LOG_OP(op) Log( "%08x: %s\n", pc, #op)
//#define LOG_OP(op) do {} while(false)

Z80State::Z80State(MemoryState* memory)
    : memory(memory)
{
    Reset();
}

void Z80State::Reset()
{
    // Rather than try to execute the Game Boy BIOS, we simply initialize
    // the CPU register to the state that they would have after the BIOS
    // is executed.

    af = 0x01B0;
    bc = 0x0013;
    de = 0x00D8;
    hl = 0x014D;
    sp = 0xFFFE;
    pc = 0x0100;

    ime = 1;
    halt = false;
    stop = false;
    
    LOG(Z80, "Reset");
}

static const UInt8 LowestBit( UInt8 val )
{
    UInt8 bit = 0x01;
    while( bit )
    {
        if( val & bit )
            return bit;
        bit <<= 1;
    }
    return 0;
}

int Z80State::Step()
{
    // We will step forward through a single instruction, and accumulate
    // the number of cycles that have elapsed:
    int cyclesElapsed = 0;
    
    if( halt )
    {
        // If the CPU is the "halt" state, then we don't execute
        // an instruction, and simply wait for 4 clock cycles.
        //
        // The CPU will only exit the halt state when an interrupt fires.
        cyclesElapsed += 4;
    }
    else
    {
        // Otherwise, we fetch an instruction from memory, advance the
        // program counter, and then execute the instruction based on
        // its opcode.
        UInt8 opcode = ReadUInt8(pc);
        pc++;
        cyclesElapsed += ExecuteOp(opcode);
    }
    
    // After executing an instruction (or not) we check for any interrupts
    // that have fired.
    UInt8 ie = memory->interruptEnable;
    UInt8 ifs = memory->GetInterruptFlags();
    if( ime && ie && ifs )
    {
        halt = false;
        ime = 0;
        InterruptFlag fired = (InterruptFlag) LowestBit(ie & ifs);
        memory->ClearInterruptFlag(fired);
        switch(fired)
        {
        case kInterruptFlag_VBlank:
            Interrupt(0x40);
            break;
        case kInterruptFlag_LcdStatus:
            Interrupt(0x48);
            break;
        case kInterruptFlag_Timer:
            Interrupt(0x50);
            break;
        case kInterruptFlag_0x08:
            Interrupt(0x58);
            break;
        case kInterruptFlag_0x10:
            Interrupt(0x60);
            break;
        default:
            ime = true;
            break;
        }
    }

//    Log( "a:%d b:%d c:%d d:%d e:%d f:%d h:%d l:%d\n",
//        a, b, c, d, e, f, h, l);
        
    return cyclesElapsed;
}

static inline UInt16 HILO( UInt8 hi, UInt8 lo )
{
    return (UInt16(hi) << 8) | UInt16(lo);
}

UInt8 Z80State::ReadUInt8( UInt16 addr )
{
    return memory->ReadUInt8( addr );
}

void Z80State::WriteUInt8( UInt16 addr, UInt8 value )
{
    memory->WriteUInt8( addr, value );
}

UInt16 Z80State::ReadUInt16( UInt16 addr )
{
    UInt8 lo = ReadUInt8(addr);
    UInt8 hi = ReadUInt8(addr+1);
    return HILO( hi, lo );
}

void Z80State::WriteUInt16( UInt16 addr, UInt16 value )
{
    WriteUInt8( addr, UInt8( value & 0xff ) );
    WriteUInt8( addr+1, UInt8( (value >> 8) & 0xff ) );
}

// Operand type: 8-bit register

UInt8 Z80State::GetVal8( const OpndReg8& reg )
{
    return reg.reg_.value;
}

void Z80State::SetVal8( const OpndReg8& reg, UInt8 value )
{
    reg.reg_.value = value;
}

// Operand type: 16-bit register

UInt16 Z80State::GetVal16( const OpndReg16& reg )
{
    return reg.reg_.value;
}

void Z80State::SetVal16( const OpndReg16& reg, UInt16 value )
{
    reg.reg_.value = value;
}

// Operand type: 8-bit immediate

UInt8 Z80State::GetVal8( const OpndImm8& imm8 )
{
    UInt8 value = ReadUInt8( pc );
    pc++;
    return value;
}

// Operand type: 16-bit immediate

UInt16 Z80State::GetVal16( const OpndImm16& imm16 )
{
    UInt16 value = ReadUInt16( pc );
    pc = pc + 2;
    return value;
}

// Operand type: 8-bit memory reference

template<typename T>
UInt8 Z80State::GetVal8( const Mem8<T>& opnd )
{
    UInt16 addr = GetVal16( opnd.val_ );
    return ReadUInt8(addr);
}

template<typename T>
void Z80State::SetVal8( const Mem8<T>& opnd, UInt8 value )
{
    UInt16 addr = GetVal16( opnd.val_ );
    WriteUInt8(addr, value);
}

// Operand type: 16-bit memory reference

template<typename T>
UInt16 Z80State::GetVal16( const Mem16<T>& opnd )
{
    UInt16 addr = GetVal16( opnd.val_ );
    return ReadUInt16(addr);
}

template<typename T>
void Z80State::SetVal16( const Mem16<T>& opnd, UInt16 value )
{
    UInt16 addr = GetVal16( opnd.val_ );
    WriteUInt16(addr, value);
}

// Operand type: 16-bit IO memory address

template<typename T>
UInt16 Z80State::GetVal16( const OpndIO16<T>& opnd )
{
    UInt8 u8 = GetVal8(opnd.val_);
    return 0xff00 + u8;
}

// Operand type: auto-increment

UInt16 Z80State::GetVal16( const OpndInc16& opnd )
{
    UInt16 value = GetVal16( opnd.val_ );
    SetVal16( opnd.val_, value+1 );
    return value;
}

// Operand type: auto-decrement

UInt16 Z80State::GetVal16( const OpndDec16& opnd )
{
    UInt16 value = GetVal16( opnd.val_ );
    SetVal16( opnd.val_, value-1 );
    return value;
}

// Operand type: sum

UInt16 Z80State::GetVal16( const OpndAdd16& opnd )
{
    throw 1;
}

// Operand type: flags

// Operand type: sign-extended

UInt16 Z80State::GetVal16( const OPSIGNED& opnd )
{
    UInt8 u8 = GetVal8( opnd.imm_ );
    return UInt16(SInt16(SInt8(u8)));
}

// 8-bit Load

template<typename Dst, typename Src>
void Z80State::LD8( const Dst& dst, const Src& src )
{
    SInt32 value = GetVal8(src);
    SetVal8(dst, value);
}

// 16-bit Load

template<typename Dst, typename Src>
void Z80State::LD16( const Dst& dst, const Src& src )
{
    UInt16 value = GetVal16(src);
    SetVal16(dst, value);
}

// 16-bit increment

template<typename Dst>
void Z80State::INC16( const Dst& dst )
{
    UInt16 value = GetVal16(dst);
    value++;
    SetVal16(dst, value);
}

// 16-bit decrement

template<typename Dst>
void Z80State::DEC16( const Dst& dst )
{
    UInt16 value = GetVal16(dst);
    value--;
    SetVal16(dst, value);
}

// 8-bit increment

template<typename Dst>
void Z80State::INC8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    UInt8 result = init + 1;
    SetVal8(dst, result);
    
    ClearFlag( kFlag_N );
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_H, (init & 0x0F) == 0x0F );
}

// 8-bit decrement

template<typename Dst>
void Z80State::DEC8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    UInt8 result = init - 1;
    SetVal8(dst, result);
    
    SetFlag( kFlag_N, true );
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_H, (init & 0x0F) == 0x00 );
}

// 8-bit rotate left

template<typename Dst>
void Z80State::RLC8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    bool carryOut = (init & 0x80) != 0;
    UInt8 carryIn = carryOut ? 0x01 : 0x00;
    
    UInt8 result = (init << 1) | carryIn;
    SetVal8(dst, result);
    
    ClearAllFlags();
    SetFlag( kFlag_C, carryOut );
    SetFlag( kFlag_Z, result == 0 );
}

// 8-bit rotate left (with carry)

template<typename Dst>
void Z80State::RL8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    bool carryOut = (init & 0x80) != 0;
    UInt8 carryIn = TestFlag(kFlag_C) ? 0x01 : 0x00;
    
    UInt8 result = (init << 1) | carryIn;
    SetVal8(dst, result);
    
    ClearAllFlags();    
    SetFlag( kFlag_C, carryOut );
    SetFlag( kFlag_Z, result == 0 );
}

// 8-bit rotate right

template<typename Dst>
void Z80State::RRC8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    bool carryOut = (init & 0x01) != 0;
    UInt8 carryIn = carryOut ? 0x80 : 0x00;
    
    UInt8 result = (init >> 1) | carryIn;
    SetVal8(dst, result);
    
    ClearAllFlags();
    
    SetFlag( kFlag_C, carryOut );
    SetFlag( kFlag_Z, result == 0 );
}

// 8-bit rotate right (with carry)

template<typename Dst>
void Z80State::RR8( const Dst& dst )
{
    UInt8 init = GetVal8(dst);
    bool carryOut = (init & 0x01) != 0;
    UInt8 carryIn = TestFlag(kFlag_C) ? 0x80 : 0x00;
    
    UInt8 result = (init >> 1) | carryIn;
    SetVal8(dst, result);
    
    ClearAllFlags();
    
    SetFlag( kFlag_C, carryOut );
    SetFlag( kFlag_Z, result == 0 );
}

// Shift left

template<typename Dst>
void Z80State::SLA8( const Dst& dst )
{
    UInt8 init = GetVal8( dst );
    UInt8 result = init << 1;
    SetVal8(dst, result);
    
    bool carryOut = (init & 0x80) != 0;

    ClearAllFlags();
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, carryOut );
}

// Arithmetic shift right

template<typename Dst>
void Z80State::SRA8( const Dst& dst )
{
    UInt8 init = GetVal8( dst );
    UInt8 result = (init >> 1) | (init & 0x80);
    SetVal8(dst, result);
    
    bool carryOut = (init & 0x01) != 0;

    ClearAllFlags();
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, carryOut );
}

// Swap low/high nibbles

template<typename Dst>
void Z80State::SWAP8( const Dst& dst )
{
    UInt8 value = GetVal8( dst );
    UInt8 result = ((value & 0x0f) << 4) | ((value & 0xf0) >> 4);
    SetVal8(dst, result);

    ClearAllFlags();
    SetFlag(kFlag_Z, result == 0);
}

// Logical shift right

template<typename Dst>
void Z80State::SRL8( const Dst& dst )
{
    UInt8 init = GetVal8( dst );
    UInt8 result = init >> 1;
    SetVal8(dst, result);
    
    bool carryOut = (init & 0x01) != 0;

    ClearAllFlags();
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, carryOut );
}


// 16-bit add

template<typename Dst, typename Src>
void Z80State::ADD16( const Dst& dst, const Src& src )
{
    UInt16 left = GetVal16(dst);
    UInt16 right = GetVal16(src);
    
    UInt16 result = left + right;
    SetVal16(dst, result);
    
    ClearFlag( kFlag_N );
    SetFlag( kFlag_C, (UInt32(left) + UInt32(right)) > 0xFFFF );
    SetFlag( kFlag_H, (UInt32(left & 0x0FFF) + UInt32(right & 0x0FFF)) > 0x0FFF );
}

// 8-bit add

template<typename Dst, typename Src>
void Z80State::ADD8( const Dst& dst, const Src& src, bool useCarry )
{
    UInt16 left = GetVal8(dst);
    UInt16 right = GetVal8(src);
    if( useCarry && TestFlag(kFlag_C) )
        right++;
    UInt8 result = left + right;
    SetVal8(dst, result);
    
    ClearFlag( kFlag_N );
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, (UInt32(left) + UInt32(right)) > 0xff );
    SetFlag( kFlag_H, ((left & 0x0f) + (right & 0x0f)) > 0x0f );
}

// 8-bit add with carry

template<typename Dst, typename Src>
void Z80State::ADC8( const Dst& dst, const Src& src )
{
    ADD8( dst, src, true );
}

// 8-bit subtract

template<typename Dst, typename Src>
void Z80State::SUB8( const Dst& dst, const Src& src, bool useCarry )
{
    UInt16 left = GetVal8(dst);
    UInt16 right = GetVal8(src);
    if( useCarry && TestFlag(kFlag_C) )
        right++;
    UInt8 result = left - right;
    SetVal8(dst, result);
    
    SetFlag( kFlag_N, true );
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, left < right );
    SetFlag( kFlag_H, (left & 0x0f) < (right & 0x0f) );
}

// 8-bit subtract with carry

template<typename Dst, typename Src>
void Z80State::SBC8( const Dst& dst, const Src& src )
{
    SUB8( dst, src, true );
}

// 8-bit AND

template<typename Src>
void Z80State::AND8( const Src& src )
{
    UInt8& A = af.hi.value;
    UInt8 left = A;
    UInt8 right = GetVal8(src);
    UInt8 result = left & right;
    A = result;

    ClearAllFlags();
    
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_H, true );
}

// 8-bit XOR

template<typename Src>
void Z80State::XOR8( const Src& src )
{
    UInt8& A = af.hi.value;
    UInt8 left = A;
    UInt8 right = GetVal8(src);
    UInt8 result = left ^ right;
    A = result;

    ClearAllFlags();
    
    SetFlag( kFlag_Z, result == 0 );
}

// 8-bit OR

template<typename Src>
void Z80State::OR8( const Src& src )
{
    UInt8& A = af.hi.value;
    UInt8 left = A;
    UInt8 right = GetVal8(src);
    UInt8 result = left | right;
    A = result;

    ClearAllFlags();
    
    SetFlag( kFlag_Z, result == 0 );
}

// Test bit

template<typename Src>
void Z80State::BIT8( int whichBit, const Src& src )
{
    UInt8 mask = 0x01 << whichBit;
    UInt8 value = GetVal8(src);
    
    SetFlag(kFlag_Z, (value & mask) == 0);    
    ClearFlag(kFlag_N);
    SetFlag(kFlag_H, true);
}

// Reset bit

template<typename Dst>
void Z80State::RES8( int whichBit, const Dst& dst )
{
    UInt8 mask = 0x01 << whichBit;
    UInt8 value = GetVal8(dst);
    value &= ~mask;
    SetVal8(dst, value);
}

// Set bit

template<typename Dst>
void Z80State::SET8( int whichBit, const Dst& dst )
{
    UInt8 mask = 0x01 << whichBit;
    UInt8 value = GetVal8(dst);
    value |= mask;
    SetVal8(dst, value);
}

// 8-bit compare

template<typename Src>
void Z80State::CP8( const Src& src )
{
    UInt8& A = af.hi.value;
    UInt8 left = A;
    UInt8 right = GetVal8(src);
    UInt8 result = left - right;
    
    ClearAllFlags();
    SetFlag( kFlag_N, true );
    SetFlag( kFlag_Z, result == 0 );
    SetFlag( kFlag_C, left < right );
    SetFlag( kFlag_H, (left & 0x0f) < (right & 0x0f) );
}

// 16-bit push

template<typename Src>
void Z80State::PUSH16( const Src& src )
{
    UInt16 value = GetVal16( src );
    sp = sp - 2;
    WriteUInt16( sp, value );
}

// 16-bit pop

template<typename Dst>
void Z80State::POP16( const Dst& dst )
{
    UInt16 value = ReadUInt16( sp );
    SetVal16( dst, value );
    sp = sp + 2;
}

// Jump relative

void Z80State::JR( const OpndFlag& flags, const OPSIGNED& src )
{
    UInt16 offset = GetVal16( src );
    
    if( TestFlags(flags) )
    {
        pc = pc + offset;
    }
}

// Jump absolute

template<typename Src>
void Z80State::JP( const OpndFlag& flags, const Src& src )
{
    UInt16 addr = GetVal16( src );
    
    if( TestFlags(flags) )
    {
        pc = addr;
    }
}

// Call

void Z80State::CALL( const OpndFlag& flags, const OpndImm16& imm )
{
    UInt16 addr = GetVal16( imm );
    
    if( TestFlags(flags) )
    {
        sp = sp - 2;
        WriteUInt16( sp, pc );
        pc = addr;
    }
}

// Return

void Z80State::RET( const OpndFlag& flags )
{
    if( TestFlags(flags) )
    {
        UInt16 addr = ReadUInt16( sp );
        sp = sp + 2;
        pc = addr;
    }
}

// Restarts

void Z80State::RST( UInt16 addr )
{
    sp = sp - 2;
    WriteUInt16( sp, pc );
    pc = addr;
}

// No-op

void Z80State::NOP()
{}

// Stop

void Z80State::STOP()
{
    throw 1;
}

// DAA

void Z80State::DAA()
{
    // This instruction cleans up a value
    // to allow for BCD (binary-coded decimal)
    // arithmetic.
    //
    // The basic idea is that each nibble of
    // an 8-bit register holds a decimal
    // digit. When an operation causes one
    // of those nibbles to overflow, then
    // we need to carry that over into the
    // next nibble.
    
    UInt8& A = af.hi.value;
    UInt8 init = A;
    
    bool N = TestFlag( kFlag_N );
    bool C = TestFlag( kFlag_C );
    bool H = TestFlag( kFlag_H );
    
    UInt8 hi = (init >> 4) & 0x0F;
    UInt8 lo = init & 0x0F;
    
    bool postC = false;
    UInt16 tmp = init;
    
    // We handle the cleanup after a subtraction
    // quite differently from an addition:
    if( N )
    {
//  N C hi  H lo add  post-C
//  1 1 6-F 1 6-F -66 1
//  1 1 7-F 0 0-9 -60 1
//  1 0 0-8 1 6-F -06 0
//  1 0 0-9 0 0-9  00 0

        if( H )
        {
            tmp -= 0x06;
        }
        if( C )
        {
            tmp -= 0x60;
            postC = true;
        }
    }
    else
    {
//  N C hi  H lo add post-C
//  0 1 0-3 1 0-3 66 1
//  0 1 0-2 0 0-9 60 1

//  0 0 A-F 1 0-3 66 1
//  0 0 9-F 0 A-F 66 1
//  0 0 A-F 0 0-9 60 1

//  0 0 0-9 1 0-3 06 0

//  0 0 0-8 0 A-F 06 0

//  0 0 0-9 0 0-9 00 0
        if( H || (lo > 0x09) )
        {
            tmp += 0x06;
        }
        if( C || (tmp > 0x99) )
        {
            tmp += 0x60;
            postC = true;
        }
    }
    UInt8 result = tmp;
    A = result;
    
    SetFlag( kFlag_Z, result == 0 );
    ClearFlag( kFlag_H );
    SetFlag( kFlag_C, postC );
}

// Complement

void Z80State::CPL()
{
    UInt8& A = af.hi.value;
    
    A = ~A;

    SetFlag(kFlag_N, true);
    SetFlag(kFlag_H, true);
}

// Set carry flag

void Z80State::SCF()
{
    SetFlag( kFlag_C, true );
    ClearFlag( kFlag_H );
    ClearFlag( kFlag_N );
}

// Complement carry flag

void Z80State::CCF()
{
    SetFlag( kFlag_C, !TestFlag( kFlag_C ) );
    ClearFlag( kFlag_H );
    ClearFlag( kFlag_N );
}

// Halt

void Z80State::HALT()
{
    halt = true;
}

// Return from interrupt

void Z80State::RETI()
{
    UInt16 addr = ReadUInt16( sp );
    sp = sp + 2;
    pc = addr;
    
    ime = true;
}

// Disable interrupts

void Z80State::DI()
{
    ime = false;
}

// Enable interrupts

void Z80State::EI()
{
    ime = true;
}

//

bool Z80State::TestFlag( Flag flag )
{
    UInt8& f = af.lo.value;
    return (f & flag) != 0;
}

void Z80State::ClearAllFlags()
{
    UInt8& f = af.lo.value;
    f = 0;
}

void Z80State::ClearFlag( Flag flag )
{
    UInt8& f = af.lo.value;
    f &= ~flag;
}

void Z80State::SetFlag( Flag flag, bool value )
{
    UInt8& f = af.lo.value;
    f &= ~flag;
    f |= value ? flag : 0x00;
}

bool Z80State::TestFlags( OpndFlag flags )
{
    UInt8& F = af.lo.value;
    
    return (F & flags.test_) == flags.compare_;
}
    

//

int Z80State::ExecuteOp( UInt8 opcode )
{
    OpndReg8 A( af.hi );
    OpndReg8 F( af.lo );
    OpndReg8 B( bc.hi );
    OpndReg8 C( bc.lo );
    OpndReg8 D( de.hi );
    OpndReg8 E( de.lo );
    OpndReg8 H( hl.hi );
    OpndReg8 L( hl.lo );
    
    OpndReg16 AF( af );
    OpndReg16 BC( bc );
    OpndReg16 DE( de );
    OpndReg16 HL( hl );
    OpndReg16 SP( sp );
    OpndReg16 PC( pc );
    
    OpndFlag FLAG_NONE( kFlag_None, kFlag_None );
    OpndFlag FLAG_NZ( kFlag_Z, kFlag_None );
    OpndFlag FLAG_Z( kFlag_Z, kFlag_Z );
    OpndFlag FLAG_NC( kFlag_C, kFlag_None );
    OpndFlag FLAG_C( kFlag_C, kFlag_C );
    
    OpndImm8 IMM8;
    OpndImm16 IMM16;

#define MEM8(val_)  (MakeMem8(val_))
#define MEM16(val_)  (MakeMem16(val_))
#define OPIO(val_)  (MakeOpndIO16(val_))
#define OPINC(val_) (OpndInc16(val_))
#define OPDEC(val_) (OpndDec16(val_))
#define OPADD(left_, right_)    (OpndAdd16(left_, right_))

    switch( opcode )
    {
    // No-op

#define OPCODE( code_, cycles_, action_ )   \
    case code_:                             \
        action_;                            \
        return cycles_;                     \
        break;

#include "opcodes.h"

#undef OPCODE

    case 0xCB: {
        UInt8 cbOpcode = ReadUInt8(pc);
        pc++;
        return ExecuteCBOp(cbOpcode);
        }
        break;
        
        
    default:
        LOG(Z80, "Unknown opcode");
        stop = true;
        throw 1;
        break;
    }
}

int Z80State::ExecuteCBOp( UInt8 opcode )
{
    OpndReg8 A( af.hi );
    OpndReg8 F( af.lo );
    OpndReg8 B( bc.hi );
    OpndReg8 C( bc.lo );
    OpndReg8 D( de.hi );
    OpndReg8 E( de.lo );
    OpndReg8 H( hl.hi );
    OpndReg8 L( hl.lo );

    OpndReg16 HL( hl );

#define MEM8(val_)  (MakeMem8(val_))

    switch( opcode )
    {
    // No-op

#define OPCODE( code_, cycles_, action_ )   \
    case code_:                             \
        action_;                            \
        return cycles_;                     \
        break;

#include "cbopcodes.h"

#undef OPCODE

    default:
        LOG(Z80, "Unknown opcode");
        stop = true;
        throw 1;
        break;
    }
}

int Z80State::Interrupt( UInt16 addr )
{
    Log("Interrupt: 0x%08X\n", addr);
//    fprintf(stderr, "Interrupt: 0x%08X\n", addr);
//    SaveRegisters();
    sp = sp - 2;
    WriteUInt16(sp, pc);
    pc = addr;
    return 12;
//    m = 3;
}
