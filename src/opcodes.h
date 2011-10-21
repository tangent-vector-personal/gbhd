// Copyright 2011 Tim Foley. All rights reserved.
//
// opcodes.h
//
// This file defines the main instruction list for the Game Boy CPU emulator.
// Each of the opcodes is defined using the OPCODE macro:
//
// OPCODE( code, cycles, action )
//
// Here 'code' gives the opcode value, 'cycles' gives how many clock cycles
// the operation takes to execute, and 'action' gives a concise definition
// of the behavior of the operation.
//
// Before including this file, a source file should define the 'OPCODE'
// macro and the various macros used in the 'action' column appropriately.

// Load (8-bit)

OPCODE( 0x7f, 4, LD8( A, A ) )
OPCODE( 0x78, 4, LD8( A, B ) )
OPCODE( 0x79, 4, LD8( A, C ) )
OPCODE( 0x7a, 4, LD8( A, D ) )
OPCODE( 0x7b, 4, LD8( A, E ) )
OPCODE( 0x7c, 4, LD8( A, H ) )
OPCODE( 0x7d, 4, LD8( A, L ) )

OPCODE( 0x47, 4, LD8( B, A ) )
OPCODE( 0x40, 4, LD8( B, B ) )
OPCODE( 0x41, 4, LD8( B, C ) )
OPCODE( 0x42, 4, LD8( B, D ) )
OPCODE( 0x43, 4, LD8( B, E ) )
OPCODE( 0x44, 4, LD8( B, H ) )
OPCODE( 0x45, 4, LD8( B, L ) )

OPCODE( 0x4f, 4, LD8( C, A ) )
OPCODE( 0x48, 4, LD8( C, B ) )
OPCODE( 0x49, 4, LD8( C, C ) )
OPCODE( 0x4a, 4, LD8( C, D ) )
OPCODE( 0x4b, 4, LD8( C, E ) )
OPCODE( 0x4c, 4, LD8( C, H ) )
OPCODE( 0x4d, 4, LD8( C, L ) )

OPCODE( 0x57, 4, LD8( D, A ) )
OPCODE( 0x50, 4, LD8( D, B ) )
OPCODE( 0x51, 4, LD8( D, C ) )
OPCODE( 0x52, 4, LD8( D, D ) )
OPCODE( 0x53, 4, LD8( D, E ) )
OPCODE( 0x54, 4, LD8( D, H ) )
OPCODE( 0x55, 4, LD8( D, L ) )

OPCODE( 0x5f, 4, LD8( E, A ) )
OPCODE( 0x58, 4, LD8( E, B ) )
OPCODE( 0x59, 4, LD8( E, C ) )
OPCODE( 0x5a, 4, LD8( E, D ) )
OPCODE( 0x5b, 4, LD8( E, E ) )
OPCODE( 0x5c, 4, LD8( E, H ) )
OPCODE( 0x5d, 4, LD8( E, L ) )

OPCODE( 0x67, 4, LD8( H, A ) )
OPCODE( 0x60, 4, LD8( H, B ) )
OPCODE( 0x61, 4, LD8( H, C ) )
OPCODE( 0x62, 4, LD8( H, D ) )
OPCODE( 0x63, 4, LD8( H, E ) )
OPCODE( 0x64, 4, LD8( H, H ) )
OPCODE( 0x65, 4, LD8( H, L ) )

OPCODE( 0x6f, 4, LD8( L, A ) )
OPCODE( 0x68, 4, LD8( L, B ) )
OPCODE( 0x69, 4, LD8( L, C ) )
OPCODE( 0x6a, 4, LD8( L, D ) )
OPCODE( 0x6b, 4, LD8( L, E ) )
OPCODE( 0x6c, 4, LD8( L, H ) )
OPCODE( 0x6d, 4, LD8( L, L ) )

OPCODE( 0x36, 12, LD8( MEM8(HL), IMM8 ) )
OPCODE( 0x77, 8, LD8( MEM8(HL), A ) )
OPCODE( 0x70, 8, LD8( MEM8(HL), B ) )
OPCODE( 0x71, 8, LD8( MEM8(HL), C ) )
OPCODE( 0x72, 8, LD8( MEM8(HL), D ) )
OPCODE( 0x73, 8, LD8( MEM8(HL), E ) )
OPCODE( 0x74, 8, LD8( MEM8(HL), H ) )
OPCODE( 0x75, 8, LD8( MEM8(HL), L ) )

OPCODE( 0x02, 8, LD8( MEM8(BC), A ) )
OPCODE( 0x12, 8, LD8( MEM8(DE), A ) )

OPCODE( 0x7e, 8, LD8( A, MEM8(HL) ) )
OPCODE( 0x46, 8, LD8( B, MEM8(HL) ) )
OPCODE( 0x4e, 8, LD8( C, MEM8(HL) ) )
OPCODE( 0x56, 8, LD8( D, MEM8(HL) ) )
OPCODE( 0x5e, 8, LD8( E, MEM8(HL) ) )
OPCODE( 0x66, 8, LD8( H, MEM8(HL) ) )
OPCODE( 0x6e, 8, LD8( L, MEM8(HL) ) )

OPCODE( 0x0a, 8, LD8( A, MEM8(BC) ) )
OPCODE( 0x1a, 8, LD8( A, MEM8(DE) ) )

OPCODE( 0x3e, 8, LD8( A, IMM8 ) )
OPCODE( 0x06, 8, LD8( B, IMM8 ) )
OPCODE( 0x0e, 8, LD8( C, IMM8 ) )
OPCODE( 0x16, 8, LD8( D, IMM8 ) )
OPCODE( 0x1e, 8, LD8( E, IMM8 ) )
OPCODE( 0x26, 8, LD8( H, IMM8 ) )
OPCODE( 0x2e, 8, LD8( L, IMM8 ) )

OPCODE( 0xe0, 12, LD8( MEM8( OPIO(IMM8) ), A ) )
OPCODE( 0xf0, 12, LD8( A, MEM8(OPIO(IMM8)) ) )
OPCODE( 0xe2, 8, LD8( MEM8( OPIO(C) ), A ) )
OPCODE( 0xf2, 8, LD8( A, MEM8( OPIO(C) ) ) )

OPCODE( 0xea, 16, LD8( MEM8( IMM16 ), A ) )
OPCODE( 0xfa, 16, LD8( A, MEM8( IMM16 ) ) )

OPCODE( 0x22, 8, LD8( MEM8(OPINC(HL)), A ) )
OPCODE( 0x2a, 8, LD8( A, MEM8(OPINC(HL)) ) )
OPCODE( 0x32, 8, LD8( MEM8(OPDEC(HL)), A ) )
OPCODE( 0x3a, 8, LD8( A, MEM8(OPDEC(HL)) ) )

// Load (16-bit)

OPCODE( 0x01, 12, LD16( BC, IMM16 ) )
OPCODE( 0x11, 12, LD16( DE, IMM16 ) )
OPCODE( 0x21, 12, LD16( HL, IMM16 ) )
OPCODE( 0x31, 12, LD16( SP, IMM16 ) )

OPCODE( 0xf8, 12, LD16( HL, OPADD( SP, IMM8 ) ) )
OPCODE( 0xf9, 8, LD16( SP, HL ) )

OPCODE( 0x08, 20, LD16( MEM16(IMM16), SP ) )

// Increment 16-bit register

OPCODE( 0x03, 8, INC16( BC ) )
OPCODE( 0x13, 8, INC16( DE ) )
OPCODE( 0x23, 8, INC16( HL ) )
OPCODE( 0x33, 8, INC16( SP ) )

// Decrement 16-bit register

OPCODE( 0x0b, 8, DEC16( BC ) )
OPCODE( 0x1b, 8, DEC16( DE ) )
OPCODE( 0x2b, 8, DEC16( HL ) )
OPCODE( 0x3b, 8, DEC16( SP ) )

// Increment 8-bit register

OPCODE( 0x3c, 4, INC8( A ) )
OPCODE( 0x04, 4, INC8( B ) )
OPCODE( 0x0c, 4, INC8( C ) )
OPCODE( 0x14, 4, INC8( D ) )
OPCODE( 0x1c, 4, INC8( E ) )
OPCODE( 0x24, 4, INC8( H ) )
OPCODE( 0x2c, 4, INC8( L ) )

OPCODE( 0x34, 12, INC8( MEM8(HL) ) )

// Decrement 8-bit register

OPCODE( 0x3d, 4, DEC8( A ) )
OPCODE( 0x05, 4, DEC8( B ) )
OPCODE( 0x0d, 4, DEC8( C ) )
OPCODE( 0x15, 4, DEC8( D ) )
OPCODE( 0x1d, 4, DEC8( E ) )
OPCODE( 0x25, 4, DEC8( H ) )
OPCODE( 0x2d, 4, DEC8( L ) )

OPCODE( 0x35, 12, DEC8( MEM8(HL) ) )

// Rotate 8-bit register left

OPCODE( 0x07, 8, RLC8( A ) )

OPCODE( 0x17, 8, RL8( A ) )

// Roate 8-bit register right

OPCODE( 0x0f, 8, RRC8( A ) )

OPCODE( 0x1f, 8, RR8( A ) )


// Add 16-bit registers

OPCODE( 0x09, 8, ADD16( HL, BC ) )
OPCODE( 0x19, 8, ADD16( HL, DE ) )
OPCODE( 0x29, 8, ADD16( HL, HL ) )
OPCODE( 0x39, 8, ADD16( HL, SP ) )

OPCODE( 0xe8, 16, ADD16( SP, OPSIGNED(IMM8) ) )

// Jump relative

OPCODE( 0x18, 8, JR( FLAG_NONE, OPSIGNED(IMM8) ) )
OPCODE( 0x20, 8, JR( FLAG_NZ, OPSIGNED(IMM8) ) )
OPCODE( 0x28, 8, JR( FLAG_Z, OPSIGNED(IMM8) ) )
OPCODE( 0x30, 8, JR( FLAG_NC, OPSIGNED(IMM8) ) )
OPCODE( 0x38, 8, JR( FLAG_C, OPSIGNED(IMM8) ) )

// Miscellaneous

OPCODE( 0x00, 4, NOP() )
OPCODE( 0x10, 4, STOP() )
OPCODE( 0x27, 4, DAA() )
OPCODE( 0x2f, 4, CPL() )
OPCODE( 0x37, 4, SCF() )
OPCODE( 0x3f, 4, CCF() )
OPCODE( 0x76, 4, HALT() )
OPCODE( 0xd9, 8, RETI() )
OPCODE( 0xf3, 4, DI() )
OPCODE( 0xfb, 4, EI() )

// Add 8-bit

OPCODE( 0x87, 4, ADD8( A, A ) )
OPCODE( 0x80, 4, ADD8( A, B ) )
OPCODE( 0x81, 4, ADD8( A, C ) )
OPCODE( 0x82, 4, ADD8( A, D ) )
OPCODE( 0x83, 4, ADD8( A, E ) )
OPCODE( 0x84, 4, ADD8( A, H ) )
OPCODE( 0x85, 4, ADD8( A, L ) )
OPCODE( 0x86, 8, ADD8( A, MEM8(HL) ) )
OPCODE( 0xc6, 8, ADD8( A, IMM8 ) )

// Add 8-bit with carry

OPCODE( 0x8f, 4, ADC8( A, A ) )
OPCODE( 0x88, 4, ADC8( A, B ) )
OPCODE( 0x89, 4, ADC8( A, C ) )
OPCODE( 0x8a, 4, ADC8( A, D ) )
OPCODE( 0x8b, 4, ADC8( A, E ) )
OPCODE( 0x8c, 4, ADC8( A, H ) )
OPCODE( 0x8d, 4, ADC8( A, L ) )
OPCODE( 0x8e, 8, ADC8( A, MEM8(HL) ) )
OPCODE( 0xce, 8, ADC8( A, IMM8 ) )

// Subtract 8-bit

OPCODE( 0x97, 4, SUB8( A, A ) )
OPCODE( 0x90, 4, SUB8( A, B ) )
OPCODE( 0x91, 4, SUB8( A, C ) )
OPCODE( 0x92, 4, SUB8( A, D ) )
OPCODE( 0x93, 4, SUB8( A, E ) )
OPCODE( 0x94, 4, SUB8( A, H ) )
OPCODE( 0x95, 4, SUB8( A, L ) )
OPCODE( 0x96, 8, SUB8( A, MEM8(HL) ) )
OPCODE( 0xd6, 8, SUB8( A, IMM8 ) )

// Subtract 8-bit with carry

OPCODE( 0x9f, 4, SBC8( A, A ) )
OPCODE( 0x98, 4, SBC8( A, B ) )
OPCODE( 0x99, 4, SBC8( A, C ) )
OPCODE( 0x9a, 4, SBC8( A, D ) )
OPCODE( 0x9b, 4, SBC8( A, E ) )
OPCODE( 0x9c, 4, SBC8( A, H ) )
OPCODE( 0x9d, 4, SBC8( A, L ) )
OPCODE( 0x9e, 8, SBC8( A, MEM8(HL) ) )
OPCODE( 0xde, 8, SBC8( A, IMM8 ) )

// Logical AND

OPCODE( 0xa7, 4, AND8( A ) )
OPCODE( 0xa0, 4, AND8( B ) )
OPCODE( 0xa1, 4, AND8( C ) )
OPCODE( 0xa2, 4, AND8( D ) )
OPCODE( 0xa3, 4, AND8( E ) )
OPCODE( 0xa4, 4, AND8( H ) )
OPCODE( 0xa5, 4, AND8( L ) )
OPCODE( 0xa6, 8, AND8( MEM8(HL) ) )
OPCODE( 0xe6, 8, AND8( IMM8 ) )

// Logical XOR

OPCODE( 0xaf, 4, XOR8( A ) )
OPCODE( 0xa8, 4, XOR8( B ) )
OPCODE( 0xa9, 4, XOR8( C ) )
OPCODE( 0xaa, 4, XOR8( D ) )
OPCODE( 0xab, 4, XOR8( E ) )
OPCODE( 0xac, 4, XOR8( H ) )
OPCODE( 0xad, 4, XOR8( L ) )
OPCODE( 0xae, 8, XOR8( MEM8(HL) ) )
OPCODE( 0xee, 8, XOR8( IMM8 ) )

// Logical OR

OPCODE( 0xb7, 4, OR8( A ) )
OPCODE( 0xb0, 4, OR8( B ) )
OPCODE( 0xb1, 4, OR8( C ) )
OPCODE( 0xb2, 4, OR8( D ) )
OPCODE( 0xb3, 4, OR8( E ) )
OPCODE( 0xb4, 4, OR8( H ) )
OPCODE( 0xb5, 4, OR8( L ) )
OPCODE( 0xb6, 8, OR8( MEM8(HL) ) )
OPCODE( 0xf6, 8, OR8( IMM8 ) )

// Compare

OPCODE( 0xbf, 4, CP8( A ) )
OPCODE( 0xb8, 4, CP8( B ) )
OPCODE( 0xb9, 4, CP8( C ) )
OPCODE( 0xba, 4, CP8( D ) )
OPCODE( 0xbb, 4, CP8( E ) )
OPCODE( 0xbc, 4, CP8( H ) )
OPCODE( 0xbd, 4, CP8( L ) )
OPCODE( 0xbe, 8, CP8( MEM8(HL) ) )
OPCODE( 0xfe, 8, CP8( IMM8 ) )

// Return

OPCODE( 0xc0, 8, RET( FLAG_NZ ) )
OPCODE( 0xc8, 8, RET( FLAG_Z ) )
OPCODE( 0xc9, 8, RET( FLAG_NONE ) )
OPCODE( 0xd0, 8, RET( FLAG_NC ) )
OPCODE( 0xd8, 8, RET( FLAG_C ))

// Pop

OPCODE( 0xc1, 12, POP16( BC ) )
OPCODE( 0xd1, 12, POP16( DE ) )
OPCODE( 0xe1, 12, POP16( HL ) )
OPCODE( 0xf1, 12, POP16( AF ) )

// Push

OPCODE( 0xc5, 16, PUSH16( BC ) )
OPCODE( 0xd5, 16, PUSH16( DE ) )
OPCODE( 0xe5, 16, PUSH16( HL ) )
OPCODE( 0xf5, 16, PUSH16( AF ) )

// Jump Absoluate

OPCODE( 0xc2, 12, JP( FLAG_NZ, IMM16 ) )
OPCODE( 0xc3, 12, JP( FLAG_NONE, IMM16 ) )
OPCODE( 0xca, 12, JP( FLAG_Z, IMM16 ) )
OPCODE( 0xd2, 12, JP( FLAG_NC, IMM16 ) )
OPCODE( 0xda, 12, JP( FLAG_C, IMM16 ) )
OPCODE( 0xe9, 12, JP( FLAG_NONE, HL ) )

// Call

OPCODE( 0xc4, 12, CALL( FLAG_NZ, IMM16 ) )
OPCODE( 0xcc, 12, CALL( FLAG_Z, IMM16 ) )
OPCODE( 0xcd, 12, CALL( FLAG_NONE, IMM16 ) )
OPCODE( 0xd4, 12, CALL( FLAG_NC, IMM16 ) )
OPCODE( 0xdc, 12, CALL( FLAG_C, IMM16 ) )

// Restart

OPCODE( 0xc7, 32, RST(0x00) )
OPCODE( 0xcf, 32, RST(0x08) )
OPCODE( 0xd7, 32, RST(0x10) )
OPCODE( 0xdf, 32, RST(0x18) )
OPCODE( 0xe7, 32, RST(0x20) )
OPCODE( 0xef, 32, RST(0x28) )
OPCODE( 0xf7, 32, RST(0x30) )
OPCODE( 0xff, 32, RST(0x38) )

//
