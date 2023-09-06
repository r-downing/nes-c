#include <assert.h>
#include <c6502.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log

static const int MAX_INSTRUCTION_TIMEOUT = 10000;
static const int TERMINATE_PC = 0x8000;

static uint8_t mem[0x10000] = {0};

static uint8_t bus_read(void *ctx, uint16_t addr) {
    return ((uint8_t *)ctx)[addr];
}

static bool bus_write(void *ctx, uint16_t addr, uint8_t val) {
    ((uint8_t *)ctx)[addr] = val;
    return true;
}

static const C6502BusInterface bus_interface = {.read = bus_read, .write = bus_write};

C6502 c = {.bus_interface = &bus_interface, .bus_ctx = mem};

int main(int, char *[]) {
    FILE *const f = fopen("nestest.nes", "rb");
    assert(f);
    // read past header
    uint8_t header[0x10];
    uint8_t prog[0x4000] = {0};
    assert(fread(header, sizeof(header), 1, f));
    assert(fread(prog, sizeof(prog), 1, f));
    fclose(f);

    memcpy(&mem[0xC000], prog, sizeof(prog));

    c6502_reset(&c);
    mem[0x100 + c.SP + 1] = (TERMINATE_PC - 1) & 0xFF;
    mem[0x100 + c.SP + 2] = (TERMINATE_PC - 1) >> 8;
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
    assert(0 == mem[0x02]);
    assert(0 == mem[0x03]);
    assert(0 == memcmp(&mem[0xC000], prog, sizeof(prog)));
    return 0;
}