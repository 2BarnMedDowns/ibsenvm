#ifndef __IBSENVM_BYTECODE_H__
#define __IBSENVM_BYTECODE_H__

/*
 * The byte code language used by the Ibsen virtual machine.
 * 
 * Opcode prefixes:
 *  000x = 0x0-1 = No operands
 *  001x = 0x2-3 = One register operand
 *  010x = 0x4-5 = Two register operands
 *  011x = 0x6-7 = Three register operands
 *  100x = 0x8-9 = Word operand
 *  101x = 0xa-b = Register operand and word operand
 *  110x = 0xc-d = Two register operands and word operand
 *  111x = 0xe-f = Three register operands and word operand
 */
enum 
{
    JUMP        =   0xa0,   // IP = [r0] + [word]
    JUMPEQ      =   0xe1,   // if [r0] == [r1] then IP = [r2] + [word]
    JUMPLT      =   0xe2,   // if [r0] < [r1] then IP = [r2] + [word]
    JUMPGT      =   0xe3,   // if [r0] > [r1] then IP = [r2] + [word]
    JUMPNE      =   0xe4,   // if [r0] != [r1] then IP = [r2] + [word]
    CALL        =   0x20,   // *(SP) = IP, SP += 1, IP = [r0] 
    RETURN      =   0x00,   // SP -= 1, IP = *(SP)
    LOAD        =   0xc7,   // [r0] = *(BP + [r1] + [word])
    LOADWORD    =   0xd7,   // [r0] = *(BP + [r1] + [word])
    STORE       =   0xc6,   // *(BP + [r1] + [word]) = [r0]
    STOREWORD   =   0xd6,   // *(BP + [r1] + [word]) = [r0]
    POP         =   0x27,   // *(SP) = [r0], SP += 1
    PUSH        =   0x26,   // SP -= 1, [r0] = *(SP)
    MOVE        =   0x47,   // [r0] = [r1]
    SET         =   0xa7,   // [r0] = [word]
    ZERO        =   0x17,   // [r0] = 0
    INVERT      =   0x28,   // [r0] = ~[r0]
    XOR         =   0x48,   // [r0] = [r0] ^ [r1]
    AND         =   0x4c,   // [r0] = [r0] & [r1]
    OR          =   0x4d,   // [r0] = [r0] | [r1]
    SHIFTUP     =   0x44,   // [r0] = [r0] << [r1]
    SHIFTDOWN   =   0x45,   // [r0] = [r0] >> [r1]
    SUB         =   0x6c,   // [r0] = [r1] + [r2]
    ADD         =   0x6d,   // [r0] = [r1] + [r2]
    MUL         =   0x64,   // [r0] = [r1] * [r2]
    DIVMOD      =   0x65,   // [r2] = [r0] % [r1], [r0] = [r0] / [r1]
    SETBP       =   0xb3,   // BP = [r0] + [word]
    SETSB       =   0xb5,   // SB = [r0] + [word]
    SETSP       =   0xbf,   // SP = [r0] + [word]
    MOVEBP      =   0x22,   // [r0] = BP
    MOVESB      =   0x24,   // [r0] = SB
    MOVESP      =   0x2e,   // [r0] = SP
    MOVEIP      =   0x30,   // [r0] = IP
    ENTER       =   0x07,   // SP = SB, SP -= 1, SB = *(SP)
    LEAVE       =   0x06,   // *(SP) = SB, SP += 1, SB = SP
    POPALL      =   0x17,   // SP = SB, SP -= 257, SB = *(SP + 256), R00-RFF = *(SP)
    PUSHALL     =   0x16,   // *(SP) = R00-RFF, *(SP + 256) = SB, SP += 257, SB = SP
    HALT        =   0x00,   // exit(R00)
    NOOP        =   0x1f,   // do nothing
    DISABLE     =   0xae,   // IMASK &= ~(1 << [r0])
    ENABLE      =   0x2f,   // IMASK |= 1 << [r0]
    VECTOR      =   0xde,   // IV = [r0] + [word]
    TRAP        =   0xaf,   // R00 = IP + 2, IP = (IV + (1 << ([r0] + [word])))
    RESTORE     =   0x10,   // IP = R00
};



/*
 * Get prefix from opcode, indicating operands.
 */
#define IVM_PREFIX(opcode) ((opcode) >> 5)


#endif /* __IBSENVM_BYTECODE_H__ */

