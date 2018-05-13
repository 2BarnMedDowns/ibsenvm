#ifndef __NORAVM_IBSEN_BYTECODE_H__
#define __NORAVM_IBSEN_BYTECODE_H__

/*
 * The Ibsen byte code language used by the Nora virtual machine.
 * 
 * Opcode prefixes:
 *  000 = No operands
 *  001 = One register operand
 *  010 = Two register operands
 *  011 = Three register operands
 *  100 = Word operand
 *  101 = Register operand and word operand
 *  110 = Two register operands and word operand
 *  111 = Three register operands and word operand
 */
enum 
{
    JUMP        =   0xa0,   // IP := [word] + R[0]
    JUMPEQ      =   0xe1,   // if R[0] == R[1] then IP := [word] + R[2]
    JUMPLT      =   0xe2,   // if R[0] < R[1] then IP := [word] + R[2]
    JUMPGT      =   0xe3,   // if R[0] > R[1] then IP := [word] + R[2]
    JUMPNE      =   0xe4,   // if R[0] != R[1] then IP := [word] + R[2]
    CALL        =   0xc0,   // R[1] := IP, IP := [word] + R[0]
    RETURN      =   0x20,   // IP := R[0]
    LOAD        =   0xc1,   // R[0] := *([word] + R[1])
    LOADWORD    =   0xc3,   // R[0] := *([word] + R[1])
    MOVE        =   0x40,   // R[0] := R[1]
    STORE       =   0xc2,   // *([word] + R[1]) := R[0]
    STOREWORD   =   0xc6,   // *([word] + R[1]) := R[0]
    SET         =   0xa3,   // R[0] := [word]
    SAVEIP      =   0xa7,   // R[0] := IP
    SAVEOFF     =   0xaf,   // R[0] := OFF
    RESTOREOFF  =   0x5e,   // OFF := R[0]
    ADD         =   0x61,   // R[0] := R[1] + R[2]
    MUL         =   0x63,   // R[0] := R[1] * R[2]
    SUB         =   0x60,   // R[0] := R[1] - R[2]
    DIV         =   0x62,   // R[2] := R[0] % R[1], R[0] := R[0] / R[1]
    AND         =   0x64,   // R[0] := R[1] & R[2]
    OR          =   0x68,   // R[0] := R[1] | R[2]
    XOR         =   0x6c,   // R[0] := R[1] ^ R[2]
    SHIFTUP     =   0x21,   // R[0] := R[0] << R[1]
    SHIFTDOWN   =   0x22,   // R[0] := R[0] >> R[1]
    INVERT      =   0x23,   // R[0] := ~R[0]
    HALT        =   0x00,   // exit(R00)
    MASK        =   0x8e,   // Set interrupt mask
    INTR        =   0x8f,   // Trap (software interrupt)
    SAVE        =   0x01,   // Push registers to state stack
    RESTORE     =   0x02,   // Pop registers from state stack
    PUSH        =   0x51,   // *(R[0]) := R[1], R[0] := R[0] + 1
    POP         =   0x52,   // R[0] := R[0] - 1, R[1] := *(R[0])
    NOOP        =   0x0f    // do nothing
};

#endif /* __NORAVM_IBSEN_BYTECODE_H__ */

