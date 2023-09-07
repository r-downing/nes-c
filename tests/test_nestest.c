#include <assert.h>
#include <c6502.h>
#include <nes_bus.h>
#include <nes_cart.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log

static const int MAX_CYCLE_TIMEOUT = 100000;
static const int TERMINATE_PC = 0x8000;

NesBus bus;

#define c (bus.cpu)
#define p (bus.ppu)

int main(int, char *[]) {
    nes_cart_init(&bus.cart, "nestest.nes");
    nes_bus_init(&bus);

    uint8_t *const mem = bus.ram;
    mem[0x100 + c.SP + 1] = (TERMINATE_PC - 1) & 0xFF;
    mem[0x100 + c.SP + 2] = (TERMINATE_PC - 1) >> 8;
    c.PC = 0xC000;  // manually set PC for headless
    c.SR.I = 1;     // ToDo - investigate
    int i;
    for (i = 0; i < MAX_CYCLE_TIMEOUT; i++) {
        nes_bus_cycle(&bus);
        if (bus.cpu.current_op_cycles_remaining) continue;
        // c6202_run_next_instruction(&bus.cpu);
        if (bus.cpu_subcycle_count != 0) continue;
        if (TERMINATE_PC == c.PC) {
            break;
        }
        printf("%04X    A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%d,%d CYC:%d\n", c.PC, c.AC, c.X, c.Y, c.SR.u8, c.SP,
               p.scanlines, p.dots, c.total_cycles);
    }
    assert(MAX_CYCLE_TIMEOUT != i);
    assert(0 == mem[0x02]);
    assert(0 == mem[0x03]);
    return 0;
}