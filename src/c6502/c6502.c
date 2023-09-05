#include <c6502.h>
#include <stddef.h>

static const uint16_t RESET_ADDR = 0xFFFC;
static const uint8_t RESET_SP = 0xFD;
static const uint16_t STACK_BASE = 0x100;
static const uint16_t IRQ_ADDR = 0xFFFE;

/** read a byte from the bus at the specified address */
static uint8_t read(const C6502 *const c, const uint16_t addr) {
    return c->bus->read(c->bus->ctx, addr);
}

/** write a byte to the bus at the specified address */
static bool write(const C6502 *const c, const uint16_t addr, const uint8_t val) {
    return c->bus->write(c->bus->ctx, addr, val);
}

/** return true if there's a page-break between the given addresses */
static bool page_break(const uint16_t a, const uint16_t b) {
    return (0xFF00 & a) != (0xFF00 & b);
}

/** Read 2 bytes at the given address as LE u16 */
static uint16_t read_u16(const C6502 *const c, const uint16_t addr) {
    const uint16_t lo = read(c, addr);
    const uint16_t hi = read(c, addr + 1);
    return (hi << 8) | lo;
}

/** Read next 2 bytes @PC as LE u16, and increment PC by 2 */
static uint16_t read_u16_pc(C6502 *const c) {
    const uint16_t lo = read(c, c->PC++);
    const uint16_t hi = read(c, c->PC++);
    return (hi << 8) | lo;
}

static void stack_push(C6502 *const c, uint8_t val) {
    write(c, STACK_BASE + c->SP, val);
    c->SP--;
}

static void stack_push_u16(C6502 *const c, uint16_t u16) {
    stack_push(c, (uint8_t)(u16 >> 8));
    stack_push(c, (uint8_t)(u16));
}

static uint8_t stack_pop(C6502 *const c) {
    c->SP++;
    return read(c, STACK_BASE + c->SP);
}

static uint16_t stack_pop_u16(C6502 *const c) {
    const uint16_t lo = stack_pop(c);
    return (((uint16_t)stack_pop(c)) << 8) | lo;
}

typedef struct Op {
    void (*op_handler)(C6502 *, const struct Op *);
    /** loads or computes the appropriate target address for an instruction. */
    uint16_t (*address_mode_handler)(C6502 *, const struct Op *);
    int cycles;
    bool page_break_extra_cycle;
} Op;

/** operating on the accumulator */
static uint16_t AM_ACC(C6502 *, const Op *) {
    return 0;
}

/** Implied - no additional data */
static uint16_t AM_IMP(C6502 *, const Op *) {
    return 0;
}

/** Immediate - next byte */
static uint16_t AM_IMM(C6502 *const c, const Op *) {
    return (c->PC)++;
}

/** Zero-page - zero page - next byte is offset into page 0 */
static uint16_t AM_ZP(C6502 *const c, const Op *) {
    return read(c, c->PC++);
}

/** zero page w/ X offset */
static uint16_t AM_ZPX(C6502 *const c, const Op *) {
    return 0xFF & (c->X + read(c, c->PC++));
}

/** zero page w/ Y offset */
static uint16_t AM_ZPY(C6502 *const c, const Op *) {
    return 0xFF & (c->Y + read(c, c->PC++));
}

/** Relative - signed 8-bit offset from PC */
static uint16_t AM_REL(C6502 *const c, const Op *) {
    const int8_t offset = (int8_t)read(c, c->PC++);
    return c->PC + offset;
}

/** Absolute - full 16 address */
static uint16_t AM_ABS(C6502 *const c, const Op *) {
    return read_u16_pc(c);
}

/** Absolute w/ X offset - full 16b addr +X */
static uint16_t AM_ABX(C6502 *const c, const Op *const op) {
    const uint16_t addr = read_u16_pc(c);
    const uint16_t ret = addr + c->X;
    if (op->page_break_extra_cycle && page_break(ret, addr)) {
        c->current_op_cycles_remaining++;
    }
    return ret;
}

/** Absolute w/ Y offset - full 16b addr +Y */
static uint16_t AM_ABY(C6502 *const c, const Op *const op) {
    const uint16_t addr = read_u16_pc(c);
    const uint16_t ret = addr + c->Y;
    if (op->page_break_extra_cycle && page_break(ret, addr)) {
        c->current_op_cycles_remaining++;
    }
    return ret;
}

/** Indirect - 16b pointer */
static uint16_t AM_IND(C6502 *const c, const Op *) {
    const uint16_t ptr = read_u16_pc(c);
    const uint16_t lo = read(c, ptr);
    // emulate page-wrap bug when reading from xxFF
    const uint16_t hi = read(c, (ptr & 0xFF00) | ((ptr + 1) & 0xFF));
    return (hi << 8) | lo;
}

/** Indirect w/ X offset - 16b pointer +X */
static uint16_t AM_INX(C6502 *const c, const Op *) {
    uint8_t zpi = read(c, c->PC++) + c->X;  // add to x (no carry)
    const uint16_t lo = read(c, zpi++);
    const uint16_t hi = read(c, zpi);  // 2nd byte may wrap around to start of zero-page
    return (hi << 8) | lo;
}

/** */
static uint16_t AM_INY(C6502 *const c, const Op *const op) {
    uint8_t zpi = read(c, c->PC++);
    const uint16_t lo = read(c, zpi++);
    const uint16_t hi = read(c, zpi);  // 2nd byte may wrap around to start of zero-page
    const uint16_t addr = (hi << 8) | lo;
    const uint16_t ret = addr + c->Y;
    if (op->page_break_extra_cycle && page_break(ret, addr)) {
        c->current_op_cycles_remaining++;
    }
    return ret;
}

////////////////////

static void update_ZN(C6502 *const c, uint8_t val) {
    c->SR.Z = (0 == val) ? 1 : 0;
    c->SR.N = val >> 7;
}

static void add(C6502 *const c, const uint8_t val) {
    const uint16_t sum = c->AC + val + c->SR.C;
    c->SR.V = ((c->AC ^ sum) & (val ^ sum)) >> 7;
    c->SR.C = sum >> 8;
    c->AC = sum;
    update_ZN(c, c->AC);
}

static void OP_ADC(C6502 *const c, const Op *) {
    add(c, read(c, c->addr));
}

static void OP_AND(C6502 *const c, const Op *) {
    c->AC &= read(c, c->addr);
    update_ZN(c, c->AC);
}

static uint8_t shift_left(C6502 *const c, uint8_t val) {
    c->SR.C = val >> 7;
    val <<= 1;
    update_ZN(c, val);
    return val;
}

static void OP_ASL(C6502 *const c, const Op *const op) {
    if (AM_ACC == op->address_mode_handler) {
        c->AC = shift_left(c, c->AC);
    } else {
        write(c, c->addr, shift_left(c, read(c, c->addr)));
    }
}

static void branch_if(C6502 *const c, const bool cond) {
    if (!cond) {
        return;
    }
    c->current_op_cycles_remaining++;
    if (page_break(c->PC, c->addr)) {
        c->current_op_cycles_remaining++;
    }
    c->PC = c->addr;
}

/** branch on carry clear */
static void OP_BCC(C6502 *const c, const Op *) {
    branch_if(c, (0 == c->SR.C));
}

/** branch on carry set*/
static void OP_BCS(C6502 *const c, const Op *) {
    branch_if(c, (0 != c->SR.C));
}

/** branch on result 0 */
static void OP_BEQ(C6502 *const c, const Op *) {
    branch_if(c, (0 != c->SR.Z));
}

static void OP_BIT(C6502 *const c, const Op *) {
    const uint8_t val = read(c, c->addr);
    const uint8_t result = c->AC & val;
    c->SR.Z = (0 == result) ? 1 : 0;
    c->SR.N = val >> 7;
    c->SR.V = (val >> 6) & 1;
}

/** branch on result minus */
static void OP_BMI(C6502 *const c, const Op *) {
    branch_if(c, (0 != c->SR.N));
}

/** branch on result not zero */
static void OP_BNE(C6502 *const c, const Op *) {
    branch_if(c, (0 == c->SR.Z));
}

/** branch on result positive*/
static void OP_BPL(C6502 *const c, const Op *) {
    branch_if(c, (0 == c->SR.N));
}

/** software interrupt */
static void OP_BRK(C6502 *const c, const Op *) {
    c->PC++;
    c->SR.I = 1;
    stack_push_u16(c, c->PC);
    c->SR.B = 1;
    stack_push(c, c->SR.u8);
    c->SR.B = 0;
    c->PC = read_u16(c, IRQ_ADDR);
}

/** branch on carry clear */
static void OP_BVC(C6502 *const c, const Op *) {
    branch_if(c, (0 == c->SR.V));
}

/** branch on carry set */
static void OP_BVS(C6502 *const c, const Op *) {
    branch_if(c, (0 != c->SR.V));
}

/** clear carry flag */
static void OP_CLC(C6502 *const c, const Op *) {
    c->SR.C = 0;
}

/** clear decimal flag*/
static void OP_CLD(C6502 *const c, const Op *) {
    c->SR.D = 0;
}

/** clear interrupt-disable flag */
static void OP_CLI(C6502 *const c, const Op *) {
    c->SR.I = 0;
}

/** clear overflow flag */
static void OP_CLV(C6502 *const c, const Op *) {
    c->SR.V = 0;
}

static void cmp_reg(C6502 *const c, const uint8_t reg) {
    const uint8_t mem = read(c, c->addr);
    const uint8_t sub = reg - mem;
    c->SR.C = (reg >= mem) ? 1 : 0;
    update_ZN(c, sub);
}

/** compare accumulator */
static void OP_CMP(C6502 *const c, const Op *) {
    cmp_reg(c, c->AC);
}

/** compare X reg */
static void OP_CPX(C6502 *const c, const Op *) {
    cmp_reg(c, c->X);
}

/** compare Y reg */
static void OP_CPY(C6502 *const c, const Op *) {
    cmp_reg(c, c->Y);
}

/** decrement mem, flags ZN */
static void OP_DEC(C6502 *const c, const Op *) {
    const uint8_t val = read(c, c->addr) - 1;
    write(c, c->addr, val);
    update_ZN(c, val);
}

/** decrement X, flags ZN */
static void OP_DEX(C6502 *const c, const Op *) {
    update_ZN(c, --(c->X));
}

/** decrement Y, flags ZN */
static void OP_DEY(C6502 *const c, const Op *) {
    update_ZN(c, --(c->Y));
}

static void OP_EOR(C6502 *const c, const Op *) {
    c->AC ^= read(c, c->addr);
    update_ZN(c, c->AC);
}

static void OP_INC(C6502 *const c, const Op *) {
    const uint8_t val = read(c, c->addr) + 1;
    write(c, c->addr, val);
    update_ZN(c, val);
}

static void OP_INX(C6502 *const c, const Op *) {
    update_ZN(c, ++(c->X));
}

static void OP_INY(C6502 *const c, const Op *) {
    update_ZN(c, ++(c->Y));
}

/** jump to address */
static void OP_JMP(C6502 *const c, const Op *) {
    c->PC = c->addr;
}

static void OP_JSR(C6502 *const c, const Op *) {
    stack_push_u16(c, c->PC - 1);
    c->PC = c->addr;
}

static void OP_LDA(C6502 *const c, const Op *) {
    c->AC = read(c, c->addr);
    update_ZN(c, c->AC);
}

static void OP_LDX(C6502 *const c, const Op *) {
    c->X = read(c, c->addr);
    update_ZN(c, c->X);
}

static void OP_LDY(C6502 *const c, const Op *) {
    c->Y = read(c, c->addr);
    update_ZN(c, c->Y);
}

static uint8_t shift_right(C6502 *const c, uint8_t val) {
    c->SR.C = val & 1;
    val >>= 1;
    update_ZN(c, val);
    return val;
}

static void OP_LSR(C6502 *const c, const Op *const op) {
    if (AM_ACC == op->address_mode_handler) {
        c->AC = shift_right(c, c->AC);
    } else {
        write(c, c->addr, shift_right(c, read(c, c->addr)));
    }
}

static void OP_NOP(C6502 *, const Op *) {
    // NOP
}

static void OP_ORA(C6502 *const c, const Op *) {
    c->AC |= read(c, c->addr);
    update_ZN(c, c->AC);
}

static void OP_PHA(C6502 *const c, const Op *) {
    stack_push(c, c->AC);
}

static void OP_PHP(C6502 *const c, const Op *) {
    typeof(c->SR) sr = c->SR;
    sr.B = 1;
    sr._unused = 1;
    stack_push(c, sr.u8);
}

static void OP_PLA(C6502 *const c, const Op *) {
    c->AC = stack_pop(c);
    update_ZN(c, c->AC);
}

static void OP_PLP(C6502 *const c, const Op *) {
    typeof(c->SR) sr = c->SR;

    c->SR.u8 = stack_pop(c);
    c->SR._unused = sr._unused;
    c->SR.B = sr.B;
}

static uint8_t rotate_left(C6502 *const c, const uint8_t val) {
    const uint8_t shifted = c->SR.C | (val << 1);
    c->SR.C = val >> 7;
    update_ZN(c, shifted);
    return shifted;
}

static void OP_ROL(C6502 *const c, const Op *const op) {
    if (AM_ACC == op->address_mode_handler) {
        c->AC = rotate_left(c, c->AC);
    } else {
        write(c, c->addr, rotate_left(c, read(c, c->addr)));
    }
}

static uint8_t rotate_right(C6502 *const c, const uint8_t val) {
    const uint8_t shifted = (c->SR.C << 7) | (val >> 1);
    c->SR.C = val & 1;
    update_ZN(c, shifted);
    return shifted;
}

static void OP_ROR(C6502 *const c, const Op *const op) {
    if (AM_ACC == op->address_mode_handler) {
        c->AC = rotate_right(c, c->AC);
    } else {
        write(c, c->addr, rotate_right(c, read(c, c->addr)));
    }
}

static void OP_RTI(C6502 *const c, const Op *) {
    typeof(c->SR) sr_prev = c->SR;
    c->SR.u8 = stack_pop(c);
    c->SR.B = sr_prev.B;
    c->SR._unused = sr_prev._unused;
    c->PC = stack_pop_u16(c);
}

static void OP_RTS(C6502 *const c, const Op *) {
    c->PC = stack_pop_u16(c) + 1;
}

static void sub(C6502 *const c, const uint8_t val) {
    const uint16_t diff = c->AC - val - (c->SR.C ^ 1);
    c->SR.C = (diff >> 8) ^ 1;
    c->SR.V = ((c->AC ^ diff) & (~val ^ diff)) >> 7;
    c->AC = diff;
    update_ZN(c, c->AC);
}

static void OP_SBC(C6502 *const c, const Op *) {
    sub(c, read(c, c->addr));
}

static void OP_SEC(C6502 *const c, const Op *) {
    c->SR.C = 1;
}

static void OP_SED(C6502 *const c, const Op *) {
    c->SR.D = 1;
}

static void OP_SEI(C6502 *const c, const Op *) {
    c->SR.I = 1;
}

/** store accumulator */
static void OP_STA(C6502 *const c, const Op *) {
    write(c, c->addr, c->AC);
}

/** store X */
static void OP_STX(C6502 *const c, const Op *) {
    write(c, c->addr, c->X);
}

/** store y*/
static void OP_STY(C6502 *const c, const Op *) {
    write(c, c->addr, c->Y);
}

/** acc to X, flags NZ */
static void OP_TAX(C6502 *const c, const Op *) {
    c->X = c->AC;
    update_ZN(c, c->X);
}

/** acc to Y, flags NZ */
static void OP_TAY(C6502 *const c, const Op *) {
    c->Y = c->AC;
    update_ZN(c, c->Y);
}

/** SP to X, flags NZ */
static void OP_TSX(C6502 *const c, const Op *) {
    c->X = c->SP;
    update_ZN(c, c->X);
}

static void OP_TXA(C6502 *const c, const Op *) {
    c->AC = c->X;
    update_ZN(c, c->AC);
}

static void OP_TXS(C6502 *const c, const Op *) {
    c->SP = c->X;
}
static void OP_TYA(C6502 *const c, const Op *) {
    c->AC = c->Y;
    update_ZN(c, c->AC);
}

// unofficial opcodes
static void OP_LAX(C6502 *const c, const Op *) {
    c->AC = c->X = read(c, c->addr);
    update_ZN(c, c->AC);
}

static void OP_SAX(C6502 *const c, const Op *) {
    write(c, c->addr, c->AC & c->X);
}

static void OP_DCP(C6502 *const c, const Op *) {
    const uint8_t m = read(c, c->addr) - 1;
    write(c, c->addr, m);
    update_ZN(c, c->AC - m);
}

static void OP_ISC(C6502 *const c, const Op *) {
    const uint8_t val = read(c, c->addr) + 1;
    write(c, c->addr, val);
    sub(c, val);
}

static void OP_SLO(C6502 *const c, const Op *) {
    const uint8_t val = shift_left(c, read(c, c->addr));
    write(c, c->addr, val);
    c->AC |= val;
    update_ZN(c, c->AC);
}

static void OP_RLA(C6502 *const c, const Op *) {
    const uint8_t val = rotate_left(c, read(c, c->addr));
    write(c, c->addr, val);
    c->AC &= val;
    update_ZN(c, c->AC);
}

static void OP_SRE(C6502 *const c, const Op *) {
    const uint8_t val = shift_right(c, read(c, c->addr));
    write(c, c->addr, val);
    c->AC ^= val;
    update_ZN(c, c->AC);
}

static void OP_RRA(C6502 *const c, const Op *) {
    const uint8_t val = rotate_right(c, read(c, c->addr));
    write(c, c->addr, val);
    add(c, val);
}

static const Op optable[0x100] = {
    [0x00] = {OP_BRK, AM_IMP, 7},
    [0x10] = {OP_BPL, AM_REL, 2},  //''
    [0x20] = {OP_JSR, AM_ABS, 6},
    [0x30] = {OP_BMI, AM_REL, 2},  //''
    [0x40] = {OP_RTI, AM_IMP, 6},
    [0x50] = {OP_BVC, AM_REL, 2},  //''
    [0x60] = {OP_RTS, AM_IMP, 6},
    [0x70] = {OP_BVS, AM_REL, 2},  //''
    [0x80] = {OP_NOP, AM_IMM, 2},
    [0x90] = {OP_BCC, AM_REL, 2},  //''
    [0xA0] = {OP_LDY, AM_IMM, 2},
    [0xB0] = {OP_BCS, AM_REL, 2},  //''
    [0xC0] = {OP_CPY, AM_IMM, 2},
    [0xD0] = {OP_BNE, AM_REL, 2},  //''
    [0xE0] = {OP_CPX, AM_IMM, 2},
    [0xF0] = {OP_BEQ, AM_REL, 2},  //''

    [0x01] = {OP_ORA, AM_INX, 6},
    [0x11] = {OP_ORA, AM_INY, 5, .page_break_extra_cycle = true},
    [0x21] = {OP_AND, AM_INX, 6},
    [0x31] = {OP_AND, AM_INY, 5, .page_break_extra_cycle = true},
    [0x41] = {OP_EOR, AM_INX, 6},
    [0x51] = {OP_EOR, AM_INY, 5, .page_break_extra_cycle = true},
    [0x61] = {OP_ADC, AM_INX, 6},
    [0x71] = {OP_ADC, AM_INY, 5, .page_break_extra_cycle = true},
    [0x81] = {OP_STA, AM_INX, 6},
    [0x91] = {OP_STA, AM_INY, 6},
    [0xA1] = {OP_LDA, AM_INX, 6},
    [0xB1] = {OP_LDA, AM_INY, 5, .page_break_extra_cycle = true},
    [0xC1] = {OP_CMP, AM_INX, 6},
    [0xD1] = {OP_CMP, AM_INY, 5, .page_break_extra_cycle = true},
    [0xE1] = {OP_SBC, AM_INX, 6},
    [0xF1] = {OP_SBC, AM_INY, 5, .page_break_extra_cycle = true},

    // ...
    [0x82] = {OP_NOP, AM_IMM, 2},
    //
    [0xA2] = {OP_LDX, AM_IMM, 2},
    //
    [0xC2] = {OP_NOP, AM_IMM, 2},
    //
    [0xE2] = {OP_NOP, AM_IMM, 2},
    //

    [0x03] = {OP_SLO, AM_INX, 8},
    [0x13] = {OP_SLO, AM_INY, 8},
    [0x23] = {OP_RLA, AM_INX, 8},
    [0x33] = {OP_RLA, AM_INY, 8},
    [0x43] = {OP_SRE, AM_INX, 8},
    [0x53] = {OP_SRE, AM_INY, 8},
    [0x63] = {OP_RRA, AM_INX, 8},
    [0x73] = {OP_RRA, AM_INY, 8},
    [0x83] = {OP_SAX, AM_INX, 6},
    //
    [0xA3] = {OP_LAX, AM_INX, 6},
    [0xB3] = {OP_LAX, AM_INY, 5, .page_break_extra_cycle = true},
    [0xC3] = {OP_DCP, AM_INX, 8},
    [0xD3] = {OP_DCP, AM_INY, 8},
    [0xE3] = {OP_ISC, AM_INX, 8},
    [0xF3] = {OP_ISC, AM_INY, 8},

    [0x04] = {OP_NOP, AM_ZP, 3},
    [0x14] = {OP_NOP, AM_ZPX, 4},
    [0x24] = {OP_BIT, AM_ZP, 3},
    [0x34] = {OP_NOP, AM_ZPX, 4},
    [0x44] = {OP_NOP, AM_ZP, 3},
    [0x54] = {OP_NOP, AM_ZPX, 4},
    [0x64] = {OP_NOP, AM_ZP, 3},
    [0x74] = {OP_NOP, AM_ZPX, 4},
    [0x84] = {OP_STY, AM_ZP, 3},
    [0x94] = {OP_STY, AM_ZPX, 4},
    [0xA4] = {OP_LDY, AM_ZP, 3},
    [0xB4] = {OP_LDY, AM_ZPX, 4},
    [0xC4] = {OP_CPY, AM_ZP, 3},
    [0xD4] = {OP_NOP, AM_ZPX, 4},
    [0xE4] = {OP_CPX, AM_ZP, 3},
    [0xF4] = {OP_NOP, AM_ZPX, 4},

    [0x05] = {OP_ORA, AM_ZP, 3},
    [0x15] = {OP_ORA, AM_ZPX, 4},
    [0x25] = {OP_AND, AM_ZP, 3},
    [0x35] = {OP_AND, AM_ZPX, 4},
    [0x45] = {OP_EOR, AM_ZP, 3},
    [0x55] = {OP_EOR, AM_ZPX, 4},
    [0x65] = {OP_ADC, AM_ZP, 3},
    [0x75] = {OP_ADC, AM_ZPX, 4},
    [0x85] = {OP_STA, AM_ZP, 3},
    [0x95] = {OP_STA, AM_ZPX, 4},
    [0xA5] = {OP_LDA, AM_ZP, 3},
    [0xB5] = {OP_LDA, AM_ZPX, 4},
    [0xC5] = {OP_CMP, AM_ZP, 3},
    [0xD5] = {OP_CMP, AM_ZPX, 4},
    [0xE5] = {OP_SBC, AM_ZP, 3},
    [0xF5] = {OP_SBC, AM_ZPX, 4},

    [0x06] = {OP_ASL, AM_ZP, 5},
    [0x16] = {OP_ASL, AM_ZPX, 6},
    [0x26] = {OP_ROL, AM_ZP, 5},
    [0x36] = {OP_ROL, AM_ZPX, 6},
    [0x46] = {OP_LSR, AM_ZP, 5},
    [0x56] = {OP_LSR, AM_ZPX, 6},
    [0x66] = {OP_ROR, AM_ZP, 5},
    [0x76] = {OP_ROR, AM_ZPX, 6},
    [0x86] = {OP_STX, AM_ZP, 3},
    [0x96] = {OP_STX, AM_ZPY, 4},
    [0xA6] = {OP_LDX, AM_ZP, 3},
    [0xB6] = {OP_LDX, AM_ZPY, 4},
    [0xC6] = {OP_DEC, AM_ZP, 5},
    [0xD6] = {OP_DEC, AM_ZPX, 6},
    [0xE6] = {OP_INC, AM_ZP, 5},
    [0xF6] = {OP_INC, AM_ZPX, 6},

    [0x07] = {OP_SLO, AM_ZP, 5},
    [0x17] = {OP_SLO, AM_ZPX, 6},
    [0x27] = {OP_RLA, AM_ZP, 5},
    [0x37] = {OP_RLA, AM_ZPX, 6},
    [0x47] = {OP_SRE, AM_ZP, 5},
    [0x57] = {OP_SRE, AM_ZPX, 6},
    [0x67] = {OP_RRA, AM_ZP, 5},
    [0x77] = {OP_RRA, AM_ZPX, 6},
    [0x87] = {OP_SAX, AM_ZP, 3},
    [0x97] = {OP_SAX, AM_ZPY, 4},
    [0xA7] = {OP_LAX, AM_ZP, 3},
    [0xB7] = {OP_LAX, AM_ZPY, 4},
    [0xC7] = {OP_DCP, AM_ZP, 5},
    [0xD7] = {OP_DCP, AM_ZPX, 6},
    [0xE7] = {OP_ISC, AM_ZP, 5},
    [0xF7] = {OP_ISC, AM_ZPX, 6},

    [0x08] = {OP_PHP, AM_IMP, 3},
    [0x18] = {OP_CLC, AM_IMP, 2},
    [0x28] = {OP_PLP, AM_IMP, 4},
    [0x38] = {OP_SEC, AM_IMP, 2},
    [0x48] = {OP_PHA, AM_IMP, 3},
    [0x58] = {OP_CLI, AM_IMP, 2},
    [0x68] = {OP_PLA, AM_IMP, 4},
    [0x78] = {OP_SEI, AM_IMP, 2},
    [0x88] = {OP_DEY, AM_IMP, 2},
    [0x98] = {OP_TYA, AM_IMP, 2},
    [0xA8] = {OP_TAY, AM_IMP, 2},
    [0xB8] = {OP_CLV, AM_IMP, 2},
    [0xC8] = {OP_INY, AM_IMP, 2},
    [0xD8] = {OP_CLD, AM_IMP, 2},
    [0xE8] = {OP_INX, AM_IMP, 2},
    [0xF8] = {OP_SED, AM_IMP, 2},

    [0x09] = {OP_ORA, AM_IMM, 2},
    [0x19] = {OP_ORA, AM_ABY, 4, .page_break_extra_cycle = true},
    [0x29] = {OP_AND, AM_IMM, 2},
    [0x39] = {OP_AND, AM_ABY, 4, .page_break_extra_cycle = true},
    [0x49] = {OP_EOR, AM_IMM, 2},
    [0x59] = {OP_EOR, AM_ABY, 4, .page_break_extra_cycle = true},
    [0x69] = {OP_ADC, AM_IMM, 2},
    [0x79] = {OP_ADC, AM_ABY, 4, .page_break_extra_cycle = true},
    [0x89] = {OP_NOP, AM_IMM, 2},
    [0x99] = {OP_STA, AM_ABY, 5},
    [0xA9] = {OP_LDA, AM_IMM, 2},
    [0xB9] = {OP_LDA, AM_ABY, 4, .page_break_extra_cycle = true},
    [0xC9] = {OP_CMP, AM_IMM, 2},
    [0xD9] = {OP_CMP, AM_ABY, 4, .page_break_extra_cycle = true},
    [0xE9] = {OP_SBC, AM_IMM, 2},
    [0xF9] = {OP_SBC, AM_ABY, 4, .page_break_extra_cycle = true},

    [0x0A] = {OP_ASL, AM_ACC, 2},
    [0x1A] = {OP_NOP, AM_IMP, 2},
    [0x2A] = {OP_ROL, AM_ACC, 2},
    [0x3A] = {OP_NOP, AM_IMP, 2},
    [0x4A] = {OP_LSR, AM_ACC, 2},
    [0x5A] = {OP_NOP, AM_IMP, 2},
    [0x6A] = {OP_ROR, AM_ACC, 2},
    [0x7A] = {OP_NOP, AM_IMP, 2},
    [0x8A] = {OP_TXA, AM_IMP, 2},
    [0x9A] = {OP_TXS, AM_IMP, 2},
    [0xAA] = {OP_TAX, AM_IMP, 2},
    [0xBA] = {OP_TSX, AM_IMP, 2},
    [0xCA] = {OP_DEX, AM_IMP, 2},
    [0xDA] = {OP_NOP, AM_IMP, 2},
    [0xEA] = {OP_NOP, AM_IMP, 2},
    [0xFA] = {OP_NOP, AM_IMP, 2},

    // xB
    [0x1B] = {OP_SLO, AM_ABY, 7},
    [0x3B] = {OP_RLA, AM_ABY, 7},
    [0x5B] = {OP_SRE, AM_ABY, 7},
    //
    [0x7B] = {OP_RRA, AM_ABY, 7},
    //
    //
    [0xAB] = {OP_LAX, AM_IMM, 2},
    //
    //
    [0xDB] = {OP_DCP, AM_ABY, 7},
    [0xEB] = {OP_SBC, AM_IMM, 2},
    [0xFB] = {OP_ISC, AM_ABY, 7},

    [0x0C] = {OP_NOP, AM_ABS, 4},
    [0x1C] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x2C] = {OP_BIT, AM_ABS, 4},
    [0x3C] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x4C] = {OP_JMP, AM_ABS, 3},
    [0x5C] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x6C] = {OP_JMP, AM_IND, 5},
    [0x7C] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x8C] = {OP_STY, AM_ABS, 4},
    // [0x9C] = {},
    [0xAC] = {OP_LDY, AM_ABS, 4},
    [0xBC] = {OP_LDY, AM_ABX, 4, .page_break_extra_cycle = true},
    [0xCC] = {OP_CPY, AM_ABS, 4},
    [0xDC] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0xEC] = {OP_CPX, AM_ABS, 4},
    [0xFC] = {OP_NOP, AM_ABX, 4, .page_break_extra_cycle = true},

    [0x0D] = {OP_ORA, AM_ABS, 4},
    [0x1D] = {OP_ORA, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x2D] = {OP_AND, AM_ABS, 4},
    [0x3D] = {OP_AND, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x4D] = {OP_EOR, AM_ABS, 4},
    [0x5D] = {OP_EOR, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x6D] = {OP_ADC, AM_ABS, 4},
    [0x7D] = {OP_ADC, AM_ABX, 4, .page_break_extra_cycle = true},
    [0x8D] = {OP_STA, AM_ABS, 4},
    [0x9D] = {OP_STA, AM_ABX, 5},
    [0xAD] = {OP_LDA, AM_ABS, 4},
    [0xBD] = {OP_LDA, AM_ABX, 4, .page_break_extra_cycle = true},
    [0xCD] = {OP_CMP, AM_ABS, 4},
    [0xDD] = {OP_CMP, AM_ABX, 4, .page_break_extra_cycle = true},
    [0xED] = {OP_SBC, AM_ABS, 4},
    [0xFD] = {OP_SBC, AM_ABX, 4, .page_break_extra_cycle = true},

    [0x0E] = {OP_ASL, AM_ABS, 6},
    [0x1E] = {OP_ASL, AM_ABX, 7},
    [0x2E] = {OP_ROL, AM_ABS, 6},
    [0x3E] = {OP_ROL, AM_ABX, 7},
    [0x4E] = {OP_LSR, AM_ABS, 6},
    [0x5E] = {OP_LSR, AM_ABX, 7},
    [0x6E] = {OP_ROR, AM_ABS, 6},
    [0x7E] = {OP_ROR, AM_ABX, 7},
    [0x8E] = {OP_STX, AM_ABS, 4},
    // [0x9E] = {},
    [0xAE] = {OP_LDX, AM_ABS, 4},
    [0xBE] = {OP_LDX, AM_ABY, 4, .page_break_extra_cycle = true},
    [0xCE] = {OP_DEC, AM_ABS, 6},
    [0xDE] = {OP_DEC, AM_ABX, 7},
    [0xEE] = {OP_INC, AM_ABS, 6},
    [0xFE] = {OP_INC, AM_ABX, 7},

    [0x0F] = {OP_SLO, AM_ABS, 6},
    [0x1F] = {OP_SLO, AM_ABX, 7},
    [0x2F] = {OP_RLA, AM_ABS, 6},
    [0x3F] = {OP_RLA, AM_ABX, 7},
    [0x4F] = {OP_SRE, AM_ABS, 6},
    [0x5F] = {OP_SRE, AM_ABX, 7},
    [0x6F] = {OP_RRA, AM_ABS, 6},
    [0x7F] = {OP_RRA, AM_ABX, 7},
    [0x8F] = {OP_SAX, AM_ABS, 4},
    //
    [0xAF] = {OP_LAX, AM_ABS, 4},
    [0xBF] = {OP_LAX, AM_ABY, 4, .page_break_extra_cycle = true},
    [0xCF] = {OP_DCP, AM_ABS, 6},
    [0xDF] = {OP_DCP, AM_ABX, 7},
    [0xEF] = {OP_ISC, AM_ABS, 6},
    [0xFF] = {OP_ISC, AM_ABX, 7},

};

bool c6202_cycle(C6502 *const c) {
    if (0 == c->current_op_cycles_remaining) {
        const Op *const op = &optable[read(c, c->PC++)];
        if ((NULL == op->address_mode_handler) || (NULL == op->op_handler)) {
            return false;
        }
        // ToDo: check op->cycles > 0
        c->current_op_cycles_remaining = op->cycles;
        c->addr = op->address_mode_handler(c, op);
        op->op_handler(c, op);
    }
    c->current_op_cycles_remaining--;
    c->total_cycles++;

    return (0 == c->current_op_cycles_remaining);
}

int c6202_run_next_instruction(C6502 *const c) {
    int ret = 0;
    do {
        ret++;
        c6202_cycle(c);
    } while (c->current_op_cycles_remaining);
    return ret;
}

void c6502_reset(C6502 *const c) {
    c->addr = RESET_ADDR;
    c->PC = read_u16(c, c->addr);
    c->SP = RESET_SP;
    c->SR.u8 = 0;
    c->SR._unused = 1;
    c->AC = 0;
    c->X = 0;
    c->Y = 0;
    c->current_op_cycles_remaining = 7;
}

void c6502_irq(C6502 *) {
    // ToDO
}
void c6502_nmi(C6502 *) {
    // ToDo
}
