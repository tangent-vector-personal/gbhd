// Copyright 2011 Tim Foley. All rights reserved.
//
// cbopcodes.h
//
// This file defines the secondary instruction list for the Game Boy CPU
// emulator, comprising those instructions that start with an 0xCB prefix.
// Each of the opcodes is defined using the OPCODE macro:
//
// OPCODE( code, cycles, action )
//
// Here 'code' gives the opcode value that follows 0xCB, 'cycles' gives how
// many clock cycles the operation takes to execute, and 'action' gives a
// concise definition of the behavior of the operation.
//
// Before including this file, a source file should define the 'OPCODE'
// macro and the various macros used in the 'action' column appropriately.

// OPCODE( code, cycles, action )

// Rotate 8-bit register left

OPCODE( 0x07, 8, RLC8( A ) )
OPCODE( 0x00, 8, RLC8( B ) )
OPCODE( 0x01, 8, RLC8( C ) )
OPCODE( 0x02, 8, RLC8( D ) )
OPCODE( 0x03, 8, RLC8( E ) )
OPCODE( 0x04, 8, RLC8( H ) )
OPCODE( 0x05, 8, RLC8( L ) )
OPCODE( 0x06, 16, RLC8( MEM8(HL) ) )

// Rotate 8-bit register right

OPCODE( 0x0F, 8, RRC8( A ) )
OPCODE( 0x08, 8, RRC8( B ) )
OPCODE( 0x09, 8, RRC8( C ) )
OPCODE( 0x0A, 8, RRC8( D ) )
OPCODE( 0x0B, 8, RRC8( E ) )
OPCODE( 0x0C, 8, RRC8( H ) )
OPCODE( 0x0D, 8, RRC8( L ) )
OPCODE( 0x0E, 16, RRC8( MEM8(HL) ) )

// Rotate 8-bit register left through carry

OPCODE( 0x17, 8, RL8( A ) )
OPCODE( 0x10, 8, RL8( B ) )
OPCODE( 0x11, 8, RL8( C ) )
OPCODE( 0x12, 8, RL8( D ) )
OPCODE( 0x13, 8, RL8( E ) )
OPCODE( 0x14, 8, RL8( H ) )
OPCODE( 0x15, 8, RL8( L ) )
OPCODE( 0x16, 16, RL8( MEM8(HL) ) )

// Rotate 8-bit register right through carry

OPCODE( 0x1F, 8, RR8( A ) )
OPCODE( 0x18, 8, RR8( B ) )
OPCODE( 0x19, 8, RR8( C ) )
OPCODE( 0x1A, 8, RR8( D ) )
OPCODE( 0x1B, 8, RR8( E ) )
OPCODE( 0x1C, 8, RR8( H ) )
OPCODE( 0x1D, 8, RR8( L ) )
OPCODE( 0x1E, 16, RR8( MEM8(HL) ) )

// Shift left

OPCODE( 0x27, 8, SLA8( A ) )
OPCODE( 0x20, 8, SLA8( B ) )
OPCODE( 0x21, 8, SLA8( C ) )
OPCODE( 0x22, 8, SLA8( D ) )
OPCODE( 0x23, 8, SLA8( E ) )
OPCODE( 0x24, 8, SLA8( H ) )
OPCODE( 0x25, 8, SLA8( L ) )
OPCODE( 0x26, 16, SLA8( MEM8(HL) ) )

// Arithmetic shift right

OPCODE( 0x2F, 8, SRA8( A ) )
OPCODE( 0x28, 8, SRA8( B ) )
OPCODE( 0x29, 8, SRA8( C ) )
OPCODE( 0x2A, 8, SRA8( D ) )
OPCODE( 0x2B, 8, SRA8( E ) )
OPCODE( 0x2C, 8, SRA8( H ) )
OPCODE( 0x2D, 8, SRA8( L ) )
OPCODE( 0x2E, 16, SRA8( MEM8(HL) ) )

// Swap low and high nibbles

OPCODE( 0x37, 8, SWAP8( A ) )
OPCODE( 0x30, 8, SWAP8( B ) )
OPCODE( 0x31, 8, SWAP8( C ) )
OPCODE( 0x32, 8, SWAP8( D ) )
OPCODE( 0x33, 8, SWAP8( E ) )
OPCODE( 0x34, 8, SWAP8( H ) )
OPCODE( 0x35, 8, SWAP8( L ) )
OPCODE( 0x36, 16, SWAP8( MEM8(HL) ) )

// Logical shift right

OPCODE( 0x3F, 8, SRL8( A ) )
OPCODE( 0x38, 8, SRL8( B ) )
OPCODE( 0x39, 8, SRL8( C ) )
OPCODE( 0x3A, 8, SRL8( D ) )
OPCODE( 0x3B, 8, SRL8( E ) )
OPCODE( 0x3C, 8, SRL8( H ) )
OPCODE( 0x3D, 8, SRL8( L ) )
OPCODE( 0x3E, 16, SRL8( MEM8(HL) ) )

// Test bit

OPCODE( 0x47,  8, BIT8( 0, A ) )
OPCODE( 0x40,  8, BIT8( 0, B ) )
OPCODE( 0x41,  8, BIT8( 0, C ) )
OPCODE( 0x42,  8, BIT8( 0, D ) )
OPCODE( 0x43,  8, BIT8( 0, E ) )
OPCODE( 0x44,  8, BIT8( 0, H ) )
OPCODE( 0x45,  8, BIT8( 0, L ) )
OPCODE( 0x46, 16, BIT8( 0, MEM8(HL) ) )

OPCODE( 0x4F,  8, BIT8( 1, A ) )
OPCODE( 0x48,  8, BIT8( 1, B ) )
OPCODE( 0x49,  8, BIT8( 1, C ) )
OPCODE( 0x4A,  8, BIT8( 1, D ) )
OPCODE( 0x4B,  8, BIT8( 1, E ) )
OPCODE( 0x4C,  8, BIT8( 1, H ) )
OPCODE( 0x4D,  8, BIT8( 1, L ) )
OPCODE( 0x4E, 16, BIT8( 1, MEM8(HL) ) )

OPCODE( 0x57,  8, BIT8( 2, A ) )
OPCODE( 0x50,  8, BIT8( 2, B ) )
OPCODE( 0x51,  8, BIT8( 2, C ) )
OPCODE( 0x52,  8, BIT8( 2, D ) )
OPCODE( 0x53,  8, BIT8( 2, E ) )
OPCODE( 0x54,  8, BIT8( 2, H ) )
OPCODE( 0x55,  8, BIT8( 2, L ) )
OPCODE( 0x56, 16, BIT8( 2, MEM8(HL) ) )

OPCODE( 0x5F,  8, BIT8( 3, A ) )
OPCODE( 0x58,  8, BIT8( 3, B ) )
OPCODE( 0x59,  8, BIT8( 3, C ) )
OPCODE( 0x5A,  8, BIT8( 3, D ) )
OPCODE( 0x5B,  8, BIT8( 3, E ) )
OPCODE( 0x5C,  8, BIT8( 3, H ) )
OPCODE( 0x5D,  8, BIT8( 3, L ) )
OPCODE( 0x5E, 16, BIT8( 3, MEM8(HL) ) )

OPCODE( 0x67,  8, BIT8( 4, A ) )
OPCODE( 0x60,  8, BIT8( 4, B ) )
OPCODE( 0x61,  8, BIT8( 4, C ) )
OPCODE( 0x62,  8, BIT8( 4, D ) )
OPCODE( 0x63,  8, BIT8( 4, E ) )
OPCODE( 0x64,  8, BIT8( 4, H ) )
OPCODE( 0x65,  8, BIT8( 4, L ) )
OPCODE( 0x66, 16, BIT8( 4, MEM8(HL) ) )

OPCODE( 0x6F,  8, BIT8( 5, A ) )
OPCODE( 0x68,  8, BIT8( 5, B ) )
OPCODE( 0x69,  8, BIT8( 5, C ) )
OPCODE( 0x6A,  8, BIT8( 5, D ) )
OPCODE( 0x6B,  8, BIT8( 5, E ) )
OPCODE( 0x6C,  8, BIT8( 5, H ) )
OPCODE( 0x6D,  8, BIT8( 5, L ) )
OPCODE( 0x6E, 16, BIT8( 5, MEM8(HL) ) )

OPCODE( 0x77,  8, BIT8( 6, A ) )
OPCODE( 0x70,  8, BIT8( 6, B ) )
OPCODE( 0x71,  8, BIT8( 6, C ) )
OPCODE( 0x72,  8, BIT8( 6, D ) )
OPCODE( 0x73,  8, BIT8( 6, E ) )
OPCODE( 0x74,  8, BIT8( 6, H ) )
OPCODE( 0x75,  8, BIT8( 6, L ) )
OPCODE( 0x76, 16, BIT8( 6, MEM8(HL) ) )

OPCODE( 0x7F,  8, BIT8( 7, A ) )
OPCODE( 0x78,  8, BIT8( 7, B ) )
OPCODE( 0x79,  8, BIT8( 7, C ) )
OPCODE( 0x7A,  8, BIT8( 7, D ) )
OPCODE( 0x7B,  8, BIT8( 7, E ) )
OPCODE( 0x7C,  8, BIT8( 7, H ) )
OPCODE( 0x7D,  8, BIT8( 7, L ) )
OPCODE( 0x7E, 16, BIT8( 7, MEM8(HL) ) )

// Reset bit

OPCODE( 0x87,  8, RES8( 0, A ) )
OPCODE( 0x80,  8, RES8( 0, B ) )
OPCODE( 0x81,  8, RES8( 0, C ) )
OPCODE( 0x82,  8, RES8( 0, D ) )
OPCODE( 0x83,  8, RES8( 0, E ) )
OPCODE( 0x84,  8, RES8( 0, H ) )
OPCODE( 0x85,  8, RES8( 0, L ) )
OPCODE( 0x86, 16, RES8( 0, MEM8(HL) ) )

OPCODE( 0x8F,  8, RES8( 1, A ) )
OPCODE( 0x88,  8, RES8( 1, B ) )
OPCODE( 0x89,  8, RES8( 1, C ) )
OPCODE( 0x8A,  8, RES8( 1, D ) )
OPCODE( 0x8B,  8, RES8( 1, E ) )
OPCODE( 0x8C,  8, RES8( 1, H ) )
OPCODE( 0x8D,  8, RES8( 1, L ) )
OPCODE( 0x8E, 16, RES8( 1, MEM8(HL) ) )

OPCODE( 0x97,  8, RES8( 2, A ) )
OPCODE( 0x90,  8, RES8( 2, B ) )
OPCODE( 0x91,  8, RES8( 2, C ) )
OPCODE( 0x92,  8, RES8( 2, D ) )
OPCODE( 0x93,  8, RES8( 2, E ) )
OPCODE( 0x94,  8, RES8( 2, H ) )
OPCODE( 0x95,  8, RES8( 2, L ) )
OPCODE( 0x96, 16, RES8( 2, MEM8(HL) ) )

OPCODE( 0x9F,  8, RES8( 3, A ) )
OPCODE( 0x98,  8, RES8( 3, B ) )
OPCODE( 0x99,  8, RES8( 3, C ) )
OPCODE( 0x9A,  8, RES8( 3, D ) )
OPCODE( 0x9B,  8, RES8( 3, E ) )
OPCODE( 0x9C,  8, RES8( 3, H ) )
OPCODE( 0x9D,  8, RES8( 3, L ) )
OPCODE( 0x9E, 16, RES8( 3, MEM8(HL) ) )

OPCODE( 0xA7,  8, RES8( 4, A ) )
OPCODE( 0xA0,  8, RES8( 4, B ) )
OPCODE( 0xA1,  8, RES8( 4, C ) )
OPCODE( 0xA2,  8, RES8( 4, D ) )
OPCODE( 0xA3,  8, RES8( 4, E ) )
OPCODE( 0xA4,  8, RES8( 4, H ) )
OPCODE( 0xA5,  8, RES8( 4, L ) )
OPCODE( 0xA6, 16, RES8( 4, MEM8(HL) ) )

OPCODE( 0xAF,  8, RES8( 5, A ) )
OPCODE( 0xA8,  8, RES8( 5, B ) )
OPCODE( 0xA9,  8, RES8( 5, C ) )
OPCODE( 0xAA,  8, RES8( 5, D ) )
OPCODE( 0xAB,  8, RES8( 5, E ) )
OPCODE( 0xAC,  8, RES8( 5, H ) )
OPCODE( 0xAD,  8, RES8( 5, L ) )
OPCODE( 0xAE, 16, RES8( 5, MEM8(HL) ) )

OPCODE( 0xB7,  8, RES8( 6, A ) )
OPCODE( 0xB0,  8, RES8( 6, B ) )
OPCODE( 0xB1,  8, RES8( 6, C ) )
OPCODE( 0xB2,  8, RES8( 6, D ) )
OPCODE( 0xB3,  8, RES8( 6, E ) )
OPCODE( 0xB4,  8, RES8( 6, H ) )
OPCODE( 0xB5,  8, RES8( 6, L ) )
OPCODE( 0xB6, 16, RES8( 6, MEM8(HL) ) )

OPCODE( 0xBF,  8, RES8( 7, A ) )
OPCODE( 0xB8,  8, RES8( 7, B ) )
OPCODE( 0xB9,  8, RES8( 7, C ) )
OPCODE( 0xBA,  8, RES8( 7, D ) )
OPCODE( 0xBB,  8, RES8( 7, E ) )
OPCODE( 0xBC,  8, RES8( 7, H ) )
OPCODE( 0xBD,  8, RES8( 7, L ) )
OPCODE( 0xBE, 16, RES8( 7, MEM8(HL) ) )

// Set bit

OPCODE( 0xC7,  8, SET8( 0, A ) )
OPCODE( 0xC0,  8, SET8( 0, B ) )
OPCODE( 0xC1,  8, SET8( 0, C ) )
OPCODE( 0xC2,  8, SET8( 0, D ) )
OPCODE( 0xC3,  8, SET8( 0, E ) )
OPCODE( 0xC4,  8, SET8( 0, H ) )
OPCODE( 0xC5,  8, SET8( 0, L ) )
OPCODE( 0xC6, 16, SET8( 0, MEM8(HL) ) )

OPCODE( 0xCF,  8, SET8( 1, A ) )
OPCODE( 0xC8,  8, SET8( 1, B ) )
OPCODE( 0xC9,  8, SET8( 1, C ) )
OPCODE( 0xCA,  8, SET8( 1, D ) )
OPCODE( 0xCB,  8, SET8( 1, E ) )
OPCODE( 0xCC,  8, SET8( 1, H ) )
OPCODE( 0xCD,  8, SET8( 1, L ) )
OPCODE( 0xCE, 16, SET8( 1, MEM8(HL) ) )

OPCODE( 0xD7,  8, SET8( 2, A ) )
OPCODE( 0xD0,  8, SET8( 2, B ) )
OPCODE( 0xD1,  8, SET8( 2, C ) )
OPCODE( 0xD2,  8, SET8( 2, D ) )
OPCODE( 0xD3,  8, SET8( 2, E ) )
OPCODE( 0xD4,  8, SET8( 2, H ) )
OPCODE( 0xD5,  8, SET8( 2, L ) )
OPCODE( 0xD6, 16, SET8( 2, MEM8(HL) ) )

OPCODE( 0xDF,  8, SET8( 3, A ) )
OPCODE( 0xD8,  8, SET8( 3, B ) )
OPCODE( 0xD9,  8, SET8( 3, C ) )
OPCODE( 0xDA,  8, SET8( 3, D ) )
OPCODE( 0xDB,  8, SET8( 3, E ) )
OPCODE( 0xDC,  8, SET8( 3, H ) )
OPCODE( 0xDD,  8, SET8( 3, L ) )
OPCODE( 0xDE, 16, SET8( 3, MEM8(HL) ) )

OPCODE( 0xE7,  8, SET8( 4, A ) )
OPCODE( 0xE0,  8, SET8( 4, B ) )
OPCODE( 0xE1,  8, SET8( 4, C ) )
OPCODE( 0xE2,  8, SET8( 4, D ) )
OPCODE( 0xE3,  8, SET8( 4, E ) )
OPCODE( 0xE4,  8, SET8( 4, H ) )
OPCODE( 0xE5,  8, SET8( 4, L ) )
OPCODE( 0xE6, 16, SET8( 4, MEM8(HL) ) )

OPCODE( 0xEF,  8, SET8( 5, A ) )
OPCODE( 0xE8,  8, SET8( 5, B ) )
OPCODE( 0xE9,  8, SET8( 5, C ) )
OPCODE( 0xEA,  8, SET8( 5, D ) )
OPCODE( 0xEB,  8, SET8( 5, E ) )
OPCODE( 0xEC,  8, SET8( 5, H ) )
OPCODE( 0xED,  8, SET8( 5, L ) )
OPCODE( 0xEE, 16, SET8( 5, MEM8(HL) ) )

OPCODE( 0xF7,  8, SET8( 6, A ) )
OPCODE( 0xF0,  8, SET8( 6, B ) )
OPCODE( 0xF1,  8, SET8( 6, C ) )
OPCODE( 0xF2,  8, SET8( 6, D ) )
OPCODE( 0xF3,  8, SET8( 6, E ) )
OPCODE( 0xF4,  8, SET8( 6, H ) )
OPCODE( 0xF5,  8, SET8( 6, L ) )
OPCODE( 0xF6, 16, SET8( 6, MEM8(HL) ) )

OPCODE( 0xFF,  8, SET8( 7, A ) )
OPCODE( 0xF8,  8, SET8( 7, B ) )
OPCODE( 0xF9,  8, SET8( 7, C ) )
OPCODE( 0xFA,  8, SET8( 7, D ) )
OPCODE( 0xFB,  8, SET8( 7, E ) )
OPCODE( 0xFC,  8, SET8( 7, H ) )
OPCODE( 0xFD,  8, SET8( 7, L ) )
OPCODE( 0xFE, 16, SET8( 7, MEM8(HL) ) )

