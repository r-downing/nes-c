#include <assert.h>
#include <c6502.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log

static const int MAX_INSTRUCTION_TIMEOUT = 10000;
static const int TERMINATE_PC = 0x8000;

static uint8_t ram[0x8000] = {0};
static uint8_t rom[0x4000] = {0};

static uint8_t bus_read(void *, uint16_t addr) {
    if (addr < sizeof(ram)) {
        return ram[addr];
    }
    return rom[addr & 0x3FFF];  // cartridge rom mirroring
}

static bool bus_write(void *, uint16_t addr, uint8_t val) {
    assert(addr < sizeof(ram));
    ram[addr] = val;
    return true;
}

static const C6502BusInterface bus_interface = {.read = bus_read, .write = bus_write};

C6502 c = {.bus = &bus_interface};

int main(int, char *[]) {
    FILE *const f = fopen("nestest.nes", "rb");
    assert(f);
    assert(fread(rom, 0x10, 1, f));
    assert(fread(rom, sizeof(rom), 1, f));
    fclose(f);

    c6502_reset(&c);
    ram[0x100 + c.SP + 1] = (TERMINATE_PC - 1) & 0xFF;
    ram[0x100 + c.SP + 2] = (TERMINATE_PC - 1) >> 8;
    c.PC = 0xC000;  // manually set PC for headless
    c.SR.I = 1;     // ToDo - investigate
    int i;
    for (i = 0; i < MAX_INSTRUCTION_TIMEOUT; i++) {
        c6202_run_next_instruction(&c);
        if (TERMINATE_PC == c.PC) {
            break;
        }
        printf("%04X    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", c.PC, c.AC, c.X, c.Y, c.SR.u8, c.SP,
               c.total_cycles);
    }
    assert(MAX_INSTRUCTION_TIMEOUT != i);
    assert(0 == ram[0x02]);
    assert(0 == ram[0x03]);
    return 0;
}