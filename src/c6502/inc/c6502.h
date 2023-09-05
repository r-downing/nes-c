#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    uint8_t (*read)(void *ctx, uint16_t addr);
    bool (*write)(void *ctx, uint16_t addr, uint8_t val);
} C6502BusInterface;

typedef struct {
    uint8_t AC;                           // accumulator
    uint8_t X;                            // register
    uint8_t Y;                            // register
    uint8_t SP;                           // stack pointer
    uint16_t PC;                          // program counter
    uint16_t addr;                        // current target address on bus
    uint8_t current_op_cycles_remaining;  // cycles left on current op
    uint32_t total_cycles;
    union __attribute__((__packed__)) {
        uint8_t u8;  // status register
        struct __attribute__((__packed__)) {
            uint8_t C : 1;        // carry bit
            uint8_t Z : 1;        // zero
            uint8_t I : 1;        // disable interrupts
            uint8_t D : 1;        // decimal mode ToDo
            uint8_t B : 1;        // break
            uint8_t _unused : 1;  // unused
            uint8_t V : 1;        // overflow
            uint8_t N : 1;        // negative
        };
    } SR;

    const C6502BusInterface *bus;
} C6502;

void c6502_reset(C6502 *);
void c6502_irq(C6502 *);
void c6502_nmi(C6502 *);

bool c6202_cycle(C6502 *);

int c6202_run_next_instruction(C6502 *);
