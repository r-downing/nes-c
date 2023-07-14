#include "chip8.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "./chip8_impl.h"

#define CHIP8_FONT_SET_START_ADDR 0x50
#define CHIP8_PROGRAM_START_ADDR 0x200

typedef union __attribute__((__packed__)) {
    uint16_t u16;

    // HXYN
    struct __attribute__((__packed__)) {
        uint16_t N : 4;
        uint16_t Y : 4;
        uint16_t X : 4;
        uint16_t H : 4;
    };

    // HXNN
    struct __attribute__((__packed__)) {
        uint16_t NN : 8;
        uint16_t : 8;  // H, X
    };

    // HNNN
    struct __attribute__((__packed__)) {
        uint16_t NNN : 12;
        uint16_t : 4;  // H
    };

} Chip8Opcode;

void chip8_reset(Chip8 *const c) {
    memset(c, 0, sizeof(*c));
    c->pc = CHIP8_PROGRAM_START_ADDR;
    memcpy(&c->mem[CHIP8_FONT_SET_START_ADDR], FONT_SET, FONT_SET_SIZE);
}

CHIP8_ERROR chip8_load_rom_data(Chip8 *const c, const uint8_t *const rom_data,
                                const uint16_t length) {
    if (length > (sizeof(c->mem) - CHIP8_PROGRAM_START_ADDR)) {
        return CHIP8_ERROR_INVALID_ROM_SIZE;
    }
    memcpy(&c->mem[CHIP8_PROGRAM_START_ADDR], rom_data, length);
    return CHIP8_ERROR_NONE;
}

CHIP8_ERROR chip8_load_instructions(Chip8 *const c,
                                    const uint16_t *const instructions,
                                    const uint16_t num_instructions) {
    const size_t length = sizeof(*instructions) * num_instructions;
    if (length > (sizeof(c->mem) - CHIP8_PROGRAM_START_ADDR)) {
        return CHIP8_ERROR_INVALID_ROM_SIZE;
    }
    uint8_t *load_dest = &c->mem[CHIP8_PROGRAM_START_ADDR];
    for (size_t n = 0; n < num_instructions; n++) {
        load_dest[0] = instructions[n] >> 8;
        load_dest[1] = instructions[n] & 0xFF;
        load_dest += sizeof(*instructions);
    }
    return CHIP8_ERROR_NONE;
}

size_t chip8_load_file(Chip8 *const c, FILE *const f) {
    return fread(&c->mem[CHIP8_PROGRAM_START_ADDR], 1,
                 sizeof(c->mem) - CHIP8_PROGRAM_START_ADDR, f);
}

static CHIP8_ERROR op_handler_0(Chip8 *const c, const Chip8Opcode *const op) {
    if (0x00E0 == op->u16) {
        memset(c->video, 0, sizeof(c->video));
        return CHIP8_ERROR_NONE;
    } else if (0x00EE == op->u16) {
        if (0 == c->sp) {
            return CHIP8_ERROR_STACK_UNDERFLOW;
        }
        c->sp--;
        c->pc = c->stack[c->sp];
        return CHIP8_ERROR_NONE;
    }
    return CHIP8_ERROR_UNKNOWN_OPCODE;
}

static CHIP8_ERROR op_handler_1(Chip8 *const c, const Chip8Opcode *const op) {
    c->pc = op->NNN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_2(Chip8 *const c, const Chip8Opcode *const op) {
    if (sizeof(c->stack) / sizeof(c->stack[0]) <= c->sp) {
        return CHIP8_ERROR_STACK_OVERFLOW;
    }
    c->stack[c->sp] = c->pc;  // save return addr
    c->sp++;
    c->pc = op->NNN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_3(Chip8 *const c, const Chip8Opcode *const op) {
    if (c->v[op->X] == op->NN) {
        c->pc += sizeof(op->u16);
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_4(Chip8 *const c, const Chip8Opcode *const op) {
    if (c->v[op->X] != op->NN) {
        c->pc += sizeof(op->u16);
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_5(Chip8 *const c, const Chip8Opcode *const op) {
    if (op->N != 0) {
        return CHIP8_ERROR_UNKNOWN_OPCODE;
    }
    if (c->v[op->X] == c->v[op->Y]) {
        c->pc += sizeof(op->u16);
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_6(Chip8 *const c, const Chip8Opcode *const op) {
    c->v[op->X] = op->NN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_7(Chip8 *const c, const Chip8Opcode *const op) {
    c->v[op->X] += op->NN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_8(Chip8 *const c, const Chip8Opcode *const op) {
    switch (op->N) {
        case 0: {
            c->v[op->X] = c->v[op->Y];
            return CHIP8_ERROR_NONE;
        }
        case 1: {
            c->v[op->X] |= c->v[op->Y];
            return CHIP8_ERROR_NONE;
        }
        case 2: {
            c->v[op->X] &= c->v[op->Y];
            return CHIP8_ERROR_NONE;
        }
        case 3: {
            c->v[op->X] ^= c->v[op->Y];
            return CHIP8_ERROR_NONE;
        }
        case 4: {
            const uint16_t add = c->v[op->X] + c->v[op->Y];
            c->v[op->X] = (uint8_t)add;
            c->v[0xF] = add >> 8;
            return CHIP8_ERROR_NONE;
        }
        case 5: {
            const uint16_t sub = 0x100 + c->v[op->X] - c->v[op->Y];
            c->v[op->X] = (uint8_t)sub;
            c->v[0xF] = sub >> 8;
            return CHIP8_ERROR_NONE;
        }
        case 6: {
            c->v[0xF] = c->v[op->X] & 1;
            c->v[op->X] >>= 1;
            return CHIP8_ERROR_NONE;
        }
        case 7: {
            const uint16_t sub = 0x100 + c->v[op->Y] - c->v[op->X];
            c->v[op->X] = (uint8_t)sub;
            c->v[0xF] = sub >> 8;
            return CHIP8_ERROR_NONE;
        }
        case 0xE: {
            c->v[0xF] = c->v[op->X] >> 7;
            c->v[op->X] <<= 1;
            return CHIP8_ERROR_NONE;
        }
        default: {
            return CHIP8_ERROR_UNKNOWN_OPCODE;
        }
    }
}

static CHIP8_ERROR op_handler_9(Chip8 *const c, const Chip8Opcode *const op) {
    if (op->N != 0) {
        return CHIP8_ERROR_UNKNOWN_OPCODE;
    }
    if (c->v[op->X] != c->v[op->Y]) {
        c->pc += sizeof(op->u16);
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_A(Chip8 *const c, const Chip8Opcode *const op) {
    c->i = op->NNN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_B(Chip8 *const c, const Chip8Opcode *const op) {
    c->pc = c->v[0] + op->NNN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_C(Chip8 *const c, const Chip8Opcode *const op) {
    c->v[op->X] = chip8_impl_rand() & op->NN;
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_D(Chip8 *const c, const Chip8Opcode *const op) {
    c->v[0xF] = 0;
    for (size_t row = 0; row < op->N; row++) {
        const uint8_t sprite_row = c->mem[c->i + row];
        const size_t yp = (c->v[op->Y] + row) % CHIP8_SCREEN_HEIGHT;
        for (size_t col = 0; col < 8; col++) {
            const size_t xp = (c->v[op->X] + col) % CHIP8_SCREEN_WIDTH;
            uint8_t *const dest = &c->video[yp][xp];
            const bool pix = sprite_row & (0x80 >> col);
            c->v[0xF] |= pix & (*dest);
            *dest ^= pix;
        }
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_E(Chip8 *const c, const Chip8Opcode *const op) {
    const uint8_t k = c->v[op->X];
    if (0x9E == op->NN) {
        if (k >= (sizeof(c->key) / sizeof(c->key[0]))) {
            return CHIP8_ERROR_INVALID_KEY_INDEX;
        }
        if (c->key[k]) {
            c->pc += sizeof(op->u16);
        }
    } else if (0xA1 == op->NN) {
        if (k >= (sizeof(c->key) / sizeof(c->key[0]))) {
            return CHIP8_ERROR_INVALID_KEY_INDEX;
        }
        if (!c->key[k]) {
            c->pc += sizeof(op->u16);
        }
    } else {
        return CHIP8_ERROR_UNKNOWN_OPCODE;
    }
    return CHIP8_ERROR_NONE;
}

static CHIP8_ERROR op_handler_F(Chip8 *const c, const Chip8Opcode *const op) {
    switch (op->NN) {
        case 0x07: {
            c->v[op->X] = 0;
            if (c->delay_active) {
                const int32_t remaining = c->delay_exp - chip8_impl_ticks();
                if (remaining > 0) {
                    c->v[op->X] = remaining;
                } else {
                    c->delay_active = false;
                }
            }
            return CHIP8_ERROR_NONE;
        }
        case 0x0A: {
            for (size_t k = 0; k < (sizeof(c->key) / sizeof(c->key[0])); k++) {
                if (c->key[k]) {
                    c->v[op->X] = k;
                    return CHIP8_ERROR_NONE;
                }
            }
            c->pc -= sizeof(op->u16);
            return CHIP8_ERROR_NONE;
        }
        case 0x15: {
            c->delay_exp = chip8_impl_ticks() + c->v[op->X];
            c->delay_active = (0 != c->v[op->X]);
            return CHIP8_ERROR_NONE;
        }
        case 0x18: {
            // ToDo sound = vx
            return CHIP8_ERROR_NONE;
        }
        case 0x1E: {
            c->i += c->v[op->X];
            return CHIP8_ERROR_NONE;
        }
        case 0x29: {
            c->i = CHIP8_FONT_SET_START_ADDR + 5 * c->v[op->X];
            return CHIP8_ERROR_NONE;
        }
        case 0x33: {
            c->mem[c->i] = c->v[op->X] / 100;
            c->mem[c->i + 1] = (c->v[op->X] / 10) % 10;
            c->mem[c->i + 2] = c->v[op->X] % 10;
            return CHIP8_ERROR_NONE;
        }
        case 0x55: {
            memcpy(&c->mem[c->i], c->v, op->X + 1);
            return CHIP8_ERROR_NONE;
        }
        case 0x65: {
            memcpy(c->v, &c->mem[c->i], op->X + 1);
            return CHIP8_ERROR_NONE;
        }
        default: {
            return CHIP8_ERROR_UNKNOWN_OPCODE;
        }
    }
}

CHIP8_ERROR chip8_cycle(Chip8 *const c) {
    // ToDo: assert(c->pc <=  (sizeof(c->mem) - 2))
    const Chip8Opcode op = {.u16 = (c->mem[c->pc]) << 8 | (c->mem[c->pc + 1])};
    c->pc += sizeof(op.u16);

    switch (op.H) {
        case 0x0:
            return op_handler_0(c, &op);
        case 0x1:
            return op_handler_1(c, &op);
        case 0x2:
            return op_handler_2(c, &op);
        case 0x3:
            return op_handler_3(c, &op);
        case 0x4:
            return op_handler_4(c, &op);
        case 0x5:
            return op_handler_5(c, &op);
        case 0x6:
            return op_handler_6(c, &op);
        case 0x7:
            return op_handler_7(c, &op);
        case 0x8:
            return op_handler_8(c, &op);
        case 0x9:
            return op_handler_9(c, &op);
        case 0xA:
            return op_handler_A(c, &op);
        case 0xB:
            return op_handler_B(c, &op);
        case 0xC:
            return op_handler_C(c, &op);
        case 0xD:
            return op_handler_D(c, &op);
        case 0xE:
            return op_handler_E(c, &op);
        case 0xF:
            return op_handler_F(c, &op);
        default:
            return CHIP8_ERROR_UNSPECIFIED;
    }
}
