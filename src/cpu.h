// Copyright 2011 Tim Foley. All rights reserved.
//
// cpu.h

#ifndef GBHD_CPU_H
#define GBHD_CPU_H

#include "memory.h"
#include "types.h"

//
// The Z80State class is responsible for storing the CPU registers of the
// Game Boy and advancing their state as instructions are executed.
//
// Its main operation is Step(), which executes a single instruction (if
// possible) and return the number of cycles that the instruction consumed.
//
class Z80State
{
public:
    Z80State(MemoryState* memory);

    MemoryState* memory;
    
    struct Reg8
    {
        UInt8 value;
        
        void operator=( UInt8 value )
        {
            this->value = value;
        }
        
        operator UInt8()
        {
            return value;
        }
    };
    
    struct Reg16
    {
        union
        {
            struct
            {
                // Big assumption about endian-ness here!
                Reg8 lo;
                Reg8 hi;
            };
            UInt16 value;
        };
        
        void operator=( UInt16 value )
        {
            this->value = value;
        }
        
        operator UInt16()
        {
            return value;
        }
        
        void operator++( int )
        {
            value++;
        }
    };
    
    Reg16 af;
    Reg16 bc;
    Reg16 de;
    Reg16 hl;
    Reg16 sp;
    Reg16 pc;
    
    enum Flag
    {
        kFlag_None = 0x00,
        kFlag_Z = 0x80,
        kFlag_N = 0x40,
        kFlag_H = 0x20,
        kFlag_C = 0x10,
    };

private:
    // \todo: Proper handling of 1-instruction
    // delay before interrupt enable/disable
    UInt32 ime;    
    
    bool halt;
    bool stop;

    void Reset();

public:
    bool IsStoppedOrHalted() { return halt || stop; }

    int Step();

    UInt8 ReadUInt8( UInt16 addr );
    void WriteUInt8( UInt16 addr, UInt8 value );
    
    UInt16 ReadUInt16( UInt16 addr );
    void WriteUInt16( UInt16 addr, UInt16 value );
    
private:
    // Operand type: 8-bit register

    struct OpndReg8
    {
        OpndReg8( Reg8& reg )
            : reg_(reg)
        {}
        
        Reg8& reg_;
    };

    UInt8 GetVal8( const OpndReg8& reg );
    void SetVal8( const OpndReg8& reg, UInt8 value );

    // Operand type: 16-bit register

    struct OpndReg16
    {
        OpndReg16( Reg16& reg )
            : reg_(reg)
        {}
        
        Reg16& reg_;
    };

    UInt16 GetVal16( const OpndReg16& reg );
    void SetVal16( const OpndReg16& reg, UInt16 value );
    
    // Operand type: 8-bit immediate

    struct OpndImm8 {};

    UInt8 GetVal8( const OpndImm8& imm8 );

    // Operand type: 16-bit immediate

    struct OpndImm16 {};

    UInt16 GetVal16( const OpndImm16& imm16 );

    // Operand type: 8-bit memory reference

    template<typename T>
    struct Mem8
    {
        Mem8( const T& val )
            : val_(val)
        {
        }
        
        const T& val_;
    };

    template<typename T>
    Mem8<T> MakeMem8( const T& val )
    {
        return Mem8<T>(val);
    }
    
    template<typename T>
    UInt8 GetVal8( const Mem8<T>& opnd );

    template<typename T>
    void SetVal8( const Mem8<T>& opnd, UInt8 value );

    // Operand type: 16-bit memory reference

    template<typename T>
    struct Mem16
    {
        Mem16( const T& val )
            : val_(val)
        {
        }
        
        const T& val_;
    };

    template<typename T>
    Mem16<T> MakeMem16( const T& val )
    {
        return Mem16<T>(val);
    }

    template<typename T>
    UInt16 GetVal16( const Mem16<T>& opnd );

    template<typename T>
    void SetVal16( const Mem16<T>& opnd, UInt16 value );

    // Operand type: 16-bit IO memory address

    template<typename T>
    struct OpndIO16
    {
        OpndIO16( T& val )
            : val_(val)
        {}
        
        const T& val_;
    };

    template<typename T>
    OpndIO16<T> MakeOpndIO16( T& val )
    {
        return OpndIO16<T>(val);
    }
    
    template<typename T>
    UInt16 GetVal16( const OpndIO16<T>& opnd );    

    // Operand type: auto-increment

    struct OpndInc16
    {
        OpndInc16( OpndReg16& val )
            : val_(val)
        {}

        const OpndReg16& val_;
    };

    UInt16 GetVal16( const OpndInc16& opnd );    

    // Operand type: auto-decrement

    struct OpndDec16
    {
        OpndDec16( OpndReg16& val )
            : val_(val)
        {}

        const OpndReg16& val_;
    };

    UInt16 GetVal16( const OpndDec16& opnd );    

    // Operand type: sum

    struct OpndAdd16
    {
        OpndAdd16( OpndReg16& left, OpndImm8& right )
            : left_(left)
            , right_(right)
        {}

        const OpndReg16& left_;
        const OpndImm8& right_;
    };
    
    UInt16 GetVal16( const OpndAdd16& opnd );    

    // Operand type: flags
    struct OpndFlag
    {
        OpndFlag( Flag test, Flag compare )
            : test_(test)
            , compare_(compare)
            {}
        
        Flag test_;
        Flag compare_;
    };
    
    // Operand type: sign-extended
    struct OPSIGNED
    {
        OPSIGNED( const OpndImm8& imm )
            : imm_(imm)
        {}
        
        const OpndImm8& imm_;
    };
    
    UInt16 GetVal16( const OPSIGNED& opnd );

    // 8-bit Load

    template<typename Dst, typename Src>
    void LD8( const Dst& dst, const Src& src );

    // 16-bit Load

    template<typename Dst, typename Src>
    void LD16( const Dst& dst, const Src& src );

    // 16-bit increment

    template<typename Dst>
    void INC16( const Dst& dst );

    // 16-bit decrement

    template<typename Dst>
    void DEC16( const Dst& dst );

    // 8-bit increment

    template<typename Dst>
    void INC8( const Dst& dst );

    // 8-bit decrement

    template<typename Dst>
    void DEC8( const Dst& dst );
    
    // 8-bit rotate left

    template<typename Dst>
    void RLC8( const Dst& dst );
    
    // 8-bit rotate left (with carry)

    template<typename Dst>
    void RL8( const Dst& dst );

    // 8-bit rotate right

    template<typename Dst>
    void RRC8( const Dst& dst );
    
    // 8-bit rotate right (with carry)

    template<typename Dst>
    void RR8( const Dst& dst );
    
    // Shift left
    
    template<typename Dst>
    void SLA8( const Dst& dst );

    // Arithmetic shift right
    
    template<typename Dst>
    void SRA8( const Dst& dst );

    // Swap low/high nibbles
    
    template<typename Dst>
    void SWAP8( const Dst& dst );

    // Logical shift right
    
    template<typename Dst>
    void SRL8( const Dst& dst );

    // 16-bit add
    
    template<typename Dst, typename Src>
    void ADD16( const Dst& dst, const Src& src );
    
    // 8-bit add
    
    template<typename Dst, typename Src>
    void ADD8( const Dst& dst, const Src& src, bool useCarry=false );
    
    // 8-bit add with carry
    
    template<typename Dst, typename Src>
    void ADC8( const Dst& dst, const Src& src );
    
    // 8-bit subtract
    
    template<typename Dst, typename Src>
    void SUB8( const Dst& dst, const Src& src, bool useCarry=false );
    
    // 8-bit subtract with carry
    
    template<typename Dst, typename Src>
    void SBC8( const Dst& dst, const Src& src );
    
    // 8-bit AND
    
    template<typename Src>
    void AND8( const Src& src );
    
    // 8-bit XOR
    
    template<typename Src>
    void XOR8( const Src& src );
    
    // 8-bit OR
    
    template<typename Src>
    void OR8( const Src& src );
    
    // Test bit
    
    template<typename Src>
    void BIT8( int whichBit, const Src& src );
    
    // Reset bit
    
    template<typename Dst>
    void RES8( int whichBit, const Dst& dst );
    
    // Set bit
    
    template<typename Dst>
    void SET8( int whichBit, const Dst& dst );
    
    // 8-bit compare
    
    template<typename Src>
    void CP8( const Src& src );
    
    // 16-bit push
    
    template<typename Src>
    void PUSH16( const Src& src );
    
    // 16-bit pop
    
    template<typename Dst>
    void POP16( const Dst& dst );
    
    // Jump relative
    
    void JR( const OpndFlag& flags, const OPSIGNED& src );
    
    // Jump absolute
    
    template<typename Src>
    void JP( const OpndFlag& flags, const Src& src );
    
    // Call
    
    void CALL( const OpndFlag& flags, const OpndImm16& imm );
    
    // Return
    
    void RET( const OpndFlag& flags );
    
    // Restarts
    
    void RST( UInt16 addr );
    
    // No-op
    
    void NOP();
    
    // Stop
    
    void STOP();
    
    // DAA
    
    void DAA();
    
    // Complement
    
    void CPL();
    
    // Set carry flag
    
    void SCF();
    
    // Complement carry flag
    
    void CCF();
    
    // Halt
    
    void HALT();
    
    // Return from interrupt
    
    void RETI();
    
    // Disable interrupts
    
    void DI();
    
    // Enable interrupts
    
    void EI();
    
    //
    
    bool TestFlag( Flag flag );
    void ClearAllFlags();
    void ClearFlag( Flag flag );
    void SetFlag( Flag flag, bool value );
    
    bool TestFlags( OpndFlag flags );
    
    int ExecuteOp( UInt8 opcode );
    int Interrupt( UInt16 addr );
    int ExecuteCBOp( UInt8 opcode );
};

#endif // GBHD_CPU_H
