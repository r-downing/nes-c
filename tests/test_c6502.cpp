#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <c6502.h>
#include <stdlib.h>
#include <string.h>

// #include "../src/chip8/chip8_impl.h"
}

uint8_t bus_read(void *ctx, uint16_t addr) {
    return ((uint8_t *)ctx)[addr];
}

bool bus_write(void *ctx, uint16_t addr, uint8_t val) {
    ((uint8_t *)ctx)[addr] = val;
    return true;
}

TEST_GROUP(C6502TestGroup) {
    C6502 c;
    uint8_t mem[0x10000];

    C6502BusInterface bus = {
        .ctx = mem,
        .read = bus_read,
        .write = bus_write,
    };

    TEST_SETUP() {
        memset(&c, 0, sizeof(c));
        memset(mem, 0, sizeof(mem));
        c.bus = &bus;
        // c6502_reset(&c);
    }

    TEST_TEARDOWN() {
        // mock().checkExpectations();
        // mock().clear();
    }
};

// BRK IMP 7
// TEST(C6502TestGroup, test_0x00) {}  // ToDo

// ORA INDX 6
// TEST(C6502TestGroup, test_0x01) {}  // ToDo

// ORA ZP 3
// TEST(C6502TestGroup, test_0x05) {}  // ToDo

// ASL ZP 5
TEST(C6502TestGroup, test_0x06) {
    c.SR.u8 = 0;
    c.PC = 123;
    mem[123] = 0x06;
    mem[124] = 51;
    mem[51] = 0b01001111;
    CHECK_EQUAL(5, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.SR.C, 0);
    CHECK_EQUAL(mem[51], 0b10011110);
    CHECK_EQUAL(c.SR.N, 1);
    CHECK_EQUAL(c.SR.Z, 0);

    c.SR.u8 = 0;
    c.PC = 123;
    mem[123] = 0x06;
    mem[124] = 51;
    mem[51] = 0b10000000;
    CHECK_EQUAL(5, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.SR.C, 1);
    CHECK_EQUAL(mem[51], 0);
    CHECK_EQUAL(c.SR.N, 0);
    CHECK_EQUAL(c.SR.Z, 1);
}

// PHP IMP 3
// TEST(C6502TestGroup, test_0x08) {}  // ToDo

// ORA IMM 2
// TEST(C6502TestGroup, test_0x09) {}  // ToDo

// ASL ACC 2
TEST(C6502TestGroup, test_0x0A) {
    c.SR.u8 = 0;
    c.PC = 900;
    mem[900] = 0x0A;
    c.AC = 0b10000001;
    CHECK_EQUAL(2, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.AC, 0b00000010);
    CHECK_EQUAL(c.PC, 901);
    CHECK_EQUAL(c.SR.N, 0);
    CHECK_EQUAL(c.SR.C, 1);
    CHECK_EQUAL(c.SR.Z, 0);
}

// ORA ABS 4
// TEST(C6502TestGroup, test_0x0D) {}  // ToDo

// ASL ABS 6
TEST(C6502TestGroup, test_0x0E) {
    c.PC = 2000;
    mem[2000] = 0x0E;
    mem[2001] = 0x34;
    mem[2002] = 0x12;
    mem[0x1234] = 0b01010101;
    CHECK_EQUAL(6, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2003);
    CHECK_EQUAL(mem[0x1234], 0b10101010);
}

// BPL REL 2''
TEST(C6502TestGroup, test_0x10) {
    c.PC = 2000;
    mem[2000] = 0x10;
    mem[2001] = 124;
    c.SR.N = 0;
    CHECK_EQUAL(4, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2126);

    c.PC = 2000;
    mem[2000] = 0x10;
    mem[2001] = -124;
    c.SR.N = 0;
    CHECK_EQUAL(3, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2000 - 122);

    c.PC = 2000;
    mem[2000] = 0x10;
    mem[2001] = -124;
    c.SR.N = 1;
    CHECK_EQUAL(2, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2002);
}  // ToDo

// ORA INY 5'
// TEST(C6502TestGroup, test_0x11) {}  // ToDo

// ORA ZPX 4
// TEST(C6502TestGroup, test_0x15) {}  // ToDo

// ASL ZPX 6
TEST(C6502TestGroup, test_0x16) {
    c.PC = 1000;
    mem[1000] = 0x16;
    mem[1001] = 0x85;
    c.X = 0x82;
    mem[7] = 0b00001111;
    CHECK_EQUAL(6, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 1002);
    CHECK_EQUAL(mem[7], 0b00011110);
}

// CLC IMP 2
TEST(C6502TestGroup, test_0x18) {
    c.PC = 500;
    mem[500] = 0x18;
    c.SR.u8 = ~0;
    c.SR.C = 1;

    CHECK_EQUAL(2, c6202_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 501);
    CHECK_EQUAL(c.SR.C, 0);
    CHECK_EQUAL(c.SR.u8, 0b11111110);
}

// ORA ABS Y 4'
// TEST(C6502TestGroup, test_0x19) {}  // ToDo

// ORA ABS X 4'
// TEST(C6502TestGroup, test_0x1D) {}  // ToDo

// ASL ABS X 7
// TEST(C6502TestGroup, test_0x1E) {}  // ToDo

// JSR ABS 6
// TEST(C6502TestGroup, test_0x20) {}  // ToDo

// AND INX 6
// TEST(C6502TestGroup, test_0x21) {}  // ToDo

// BIT ZP 3
// TEST(C6502TestGroup, test_0x24) {}  // ToDo

// AND ZP 3
// TEST(C6502TestGroup, test_0x25) {}  // ToDo

// ROL ZP 5
// TEST(C6502TestGroup, test_0x26) {}  // ToDo

// PLP IMP 4
// TEST(C6502TestGroup, test_0x28) {}  // ToDo

// AND IMM 2
// TEST(C6502TestGroup, test_0x29) {}  // ToDo

// ROL ACC 2
// TEST(C6502TestGroup, test_0x2A) {}  // ToDo

// BIT ABS 4
// TEST(C6502TestGroup, test_0x2C) {}  // ToDo

// AND ABS 4
// TEST(C6502TestGroup, test_0x2D) {}  // ToDo

// ROL ABS 6
// TEST(C6502TestGroup, test_0x2E) {}  // ToDo

// BMI REL 2''
// TEST(C6502TestGroup, test_0x30) {}  // ToDo

// AND INY 5'
// TEST(C6502TestGroup, test_0x31) {}  // ToDo

// AND ZPX 4
// TEST(C6502TestGroup, test_0x35) {}  // ToDo

// ROL ZPX 6
// TEST(C6502TestGroup, test_0x36) {}  // ToDo

// SEC IMP 2
// TEST(C6502TestGroup, test_0x38) {}  // ToDo

// AND ABY 4'
// TEST(C6502TestGroup, test_0x39) {}  // ToDo
// TEST(C6502TestGroup, test_0x3A) {}  // ToDo

// TEST(C6502TestGroup, test_0x3C) {}  // ToDo
// TEST(C6502TestGroup, test_0x3D) {}  // ToDo
// TEST(C6502TestGroup, test_0x3E) {}  // ToDo

// TEST(C6502TestGroup, test_0x40) {}  // ToDo
// TEST(C6502TestGroup, test_0x41) {}  // ToDo

// TEST(C6502TestGroup, test_0x44) {}  // ToDo
// TEST(C6502TestGroup, test_0x45) {}  // ToDo
// TEST(C6502TestGroup, test_0x46) {}  // ToDo

// TEST(C6502TestGroup, test_0x48) {}  // ToDo
// TEST(C6502TestGroup, test_0x49) {}  // ToDo
// TEST(C6502TestGroup, test_0x4A) {}  // ToDo

// TEST(C6502TestGroup, test_0x4C) {}  // ToDo
// TEST(C6502TestGroup, test_0x4D) {}  // ToDo
// TEST(C6502TestGroup, test_0x4E) {}  // ToDo

// TEST(C6502TestGroup, test_0x50) {}  // ToDo
// TEST(C6502TestGroup, test_0x51) {}  // ToDo

// TEST(C6502TestGroup, test_0x54) {}  // ToDo
// TEST(C6502TestGroup, test_0x55) {}  // ToDo
// TEST(C6502TestGroup, test_0x56) {}  // ToDo

// TEST(C6502TestGroup, test_0x58) {}  // ToDo
// TEST(C6502TestGroup, test_0x59) {}  // ToDo
// TEST(C6502TestGroup, test_0x5A) {}  // ToDo

// TEST(C6502TestGroup, test_0x5C) {}  // ToDo
// TEST(C6502TestGroup, test_0x5D) {}  // ToDo
// TEST(C6502TestGroup, test_0x5E) {}  // ToDo

// TEST(C6502TestGroup, test_0x60) {}  // ToDo
// TEST(C6502TestGroup, test_0x61) {}  // ToDo

// TEST(C6502TestGroup, test_0x64) {}  // ToDo
// TEST(C6502TestGroup, test_0x65) {}  // ToDo
// TEST(C6502TestGroup, test_0x66) {}  // ToDo

// TEST(C6502TestGroup, test_0x68) {}  // ToDo
// TEST(C6502TestGroup, test_0x69) {}  // ToDo
// TEST(C6502TestGroup, test_0x6A) {}  // ToDo

// TEST(C6502TestGroup, test_0x6C) {}  // ToDo
// TEST(C6502TestGroup, test_0x6D) {}  // ToDo
// TEST(C6502TestGroup, test_0x6E) {}  // ToDo

// TEST(C6502TestGroup, test_0x70) {}  // ToDo
// TEST(C6502TestGroup, test_0x71) {}  // ToDo

// TEST(C6502TestGroup, test_0x74) {}  // ToDo
// TEST(C6502TestGroup, test_0x75) {}  // ToDo
// TEST(C6502TestGroup, test_0x76) {}  // ToDo

// TEST(C6502TestGroup, test_0x78) {}  // ToDo
// TEST(C6502TestGroup, test_0x79) {}  // ToDo
// TEST(C6502TestGroup, test_0x7A) {}  // ToDo

// TEST(C6502TestGroup, test_0x7C) {}  // ToDo
// TEST(C6502TestGroup, test_0x7D) {}  // ToDo
// TEST(C6502TestGroup, test_0x7E) {}  // ToDo

// TEST(C6502TestGroup, test_0x81) {}  // ToDo

// TEST(C6502TestGroup, test_0x84) {}  // ToDo
// TEST(C6502TestGroup, test_0x85) {}  // ToDo
// TEST(C6502TestGroup, test_0x86) {}  // ToDo

// TEST(C6502TestGroup, test_0x88) {}  // ToDo
// TEST(C6502TestGroup, test_0x89) {}  // ToDo
// TEST(C6502TestGroup, test_0x8A) {}  // ToDo

// TEST(C6502TestGroup, test_0x8C) {}  // ToDo
// TEST(C6502TestGroup, test_0x8D) {}  // ToDo
// TEST(C6502TestGroup, test_0x8E) {}  // ToDo

// TEST(C6502TestGroup, test_0x90) {}  // ToDo
// TEST(C6502TestGroup, test_0x91) {}  // ToDo

// TEST(C6502TestGroup, test_0x94) {}  // ToDo
// TEST(C6502TestGroup, test_0x95) {}  // ToDo
// TEST(C6502TestGroup, test_0x96) {}  // ToDo

// TEST(C6502TestGroup, test_0x98) {}  // ToDo
// TEST(C6502TestGroup, test_0x99) {}  // ToDo
// TEST(C6502TestGroup, test_0x9A) {}  // ToDo

// TEST(C6502TestGroup, test_0x9C) {}  // ToDo
// TEST(C6502TestGroup, test_0x9D) {}  // ToDo
// TEST(C6502TestGroup, test_0x9E) {}  // ToDo

// TEST(C6502TestGroup, test_0xA0) {}  // ToDo
// TEST(C6502TestGroup, test_0xA1) {}  // ToDo
// TEST(C6502TestGroup, test_0xA2) {}  // ToDo

// TEST(C6502TestGroup, test_0xA4) {}  // ToDo
// TEST(C6502TestGroup, test_0xA5) {}  // ToDo
// TEST(C6502TestGroup, test_0xA6) {}  // ToDo

// TEST(C6502TestGroup, test_0xA8) {}  // ToDo
// TEST(C6502TestGroup, test_0xA9) {}  // ToDo
// TEST(C6502TestGroup, test_0xAA) {}  // ToDo

// TEST(C6502TestGroup, test_0xAC) {}  // ToDo
// TEST(C6502TestGroup, test_0xAD) {}  // ToDo
// TEST(C6502TestGroup, test_0xAE) {}  // ToDo

// TEST(C6502TestGroup, test_0xB0) {}  // ToDo
// TEST(C6502TestGroup, test_0xB1) {}  // ToDo

// TEST(C6502TestGroup, test_0xB4) {}  // ToDo
// TEST(C6502TestGroup, test_0xB5) {}  // ToDo
// TEST(C6502TestGroup, test_0xB6) {}  // ToDo

// TEST(C6502TestGroup, test_0xB8) {}  // ToDo
// TEST(C6502TestGroup, test_0xB9) {}  // ToDo
// TEST(C6502TestGroup, test_0xBA) {}  // ToDo

// TEST(C6502TestGroup, test_0xBC) {}  // ToDo
// TEST(C6502TestGroup, test_0xBD) {}  // ToDo
// TEST(C6502TestGroup, test_0xBE) {}  // ToDo

// TEST(C6502TestGroup, test_0xC0) {}  // ToDo
// TEST(C6502TestGroup, test_0xC1) {}  // ToDo

// TEST(C6502TestGroup, test_0xC4) {}  // ToDo
// TEST(C6502TestGroup, test_0xC5) {}  // ToDo
// TEST(C6502TestGroup, test_0xC6) {}  // ToDo

// TEST(C6502TestGroup, test_0xC8) {}  // ToDo
// TEST(C6502TestGroup, test_0xC9) {}  // ToDo
// TEST(C6502TestGroup, test_0xCA) {}  // ToDo

// TEST(C6502TestGroup, test_0xCC) {}  // ToDo
// TEST(C6502TestGroup, test_0xCD) {}  // ToDo
// TEST(C6502TestGroup, test_0xCE) {}  // ToDo

// TEST(C6502TestGroup, test_0xD0) {}  // ToDo
// TEST(C6502TestGroup, test_0xD1) {}  // ToDo

// TEST(C6502TestGroup, test_0xD4) {}  // ToDo
// TEST(C6502TestGroup, test_0xD5) {}  // ToDo
// TEST(C6502TestGroup, test_0xD6) {}  // ToDo

// TEST(C6502TestGroup, test_0xD8) {}  // ToDo
// TEST(C6502TestGroup, test_0xD9) {}  // ToDo
// TEST(C6502TestGroup, test_0xDA) {}  // ToDo

// TEST(C6502TestGroup, test_0xDC) {}  // ToDo
// TEST(C6502TestGroup, test_0xDD) {}  // ToDo
// TEST(C6502TestGroup, test_0xDE) {}  // ToDo

// TEST(C6502TestGroup, test_0xE0) {}  // ToDo
// TEST(C6502TestGroup, test_0xE1) {}  // ToDo

// TEST(C6502TestGroup, test_0xE4) {}  // ToDo
// TEST(C6502TestGroup, test_0xE5) {}  // ToDo
// TEST(C6502TestGroup, test_0xE6) {}  // ToDo

// TEST(C6502TestGroup, test_0xE8) {}  // ToDo
// TEST(C6502TestGroup, test_0xE9) {}  // ToDo
// TEST(C6502TestGroup, test_0xEA) {}  // ToDo

// TEST(C6502TestGroup, test_0xEC) {}  // ToDo
// TEST(C6502TestGroup, test_0xED) {}  // ToDo
// TEST(C6502TestGroup, test_0xEE) {}  // ToDo

// TEST(C6502TestGroup, test_0xF0) {}  // ToDo
// TEST(C6502TestGroup, test_0xF1) {}  // ToDo

// TEST(C6502TestGroup, test_0xF4) {}  // ToDo
// TEST(C6502TestGroup, test_0xF5) {}  // ToDo
// TEST(C6502TestGroup, test_0xF6) {}  // ToDo

// TEST(C6502TestGroup, test_0xF8) {}  // ToDo
// TEST(C6502TestGroup, test_0xF9) {}  // ToDo
// TEST(C6502TestGroup, test_0xFA) {}  // ToDo

// TEST(C6502TestGroup, test_0xFC) {}  // ToDo
// TEST(C6502TestGroup, test_0xFD) {}  // ToDo
// TEST(C6502TestGroup, test_0xFE) {}  // ToDo
