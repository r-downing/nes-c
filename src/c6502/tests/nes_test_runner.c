#include <assert.h>
#include <c6502.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Missing nestest.nes rom path arg");
        return 1;
    }
    if (argc < 3) {
        printf("Missing step count");
        return 2;
    }

    const int step_count = atoi(argv[2]);

    FILE *const f = fopen(argv[1], "rb");
    assert(f);
    assert(fread(rom, 0x10, 1, f));
    assert(fread(rom, sizeof(rom), 1, f));
    fclose(f);

    c6502_reset(&c);
    c.PC = 0xC000;  // manually set PC for headless
    c.SR.I = 1;     // ToDo - investigate
    int tc = 0;     // ToDo - fix offset, add to c6502 struct
    volatile int bp = 0;
    for (int i = 0; i < step_count; i++) {
        tc += c6202_run_next_instruction(&c);
        printf("%04X    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", c.PC, c.AC, c.X, c.Y, c.SR.u8, c.SP, tc);
    }
    return 0;
}