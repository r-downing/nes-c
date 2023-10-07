#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <c6502.h>
#include <stdlib.h>
#include <string.h>
}

static uint8_t mock_bus_read(void *, uint16_t addr) {
    return mock().actualCall(__func__).withParameter("addr", addr).returnUnsignedIntValueOrDefault(0);
}

static bool mock_bus_write(void *, uint16_t addr, uint8_t val) {
    return mock()
        .actualCall(__func__)
        .withParameter("addr", addr)
        .withParameter("val", val)
        .returnBoolValueOrDefault(false);
}

static C6502BusInterface bus = {
    .read = mock_bus_read,
    .write = mock_bus_write,
};

namespace mock_bus {
void expect_read(uint16_t addr, uint8_t ret) {
    mock().expectOneCall("mock_bus_read").withParameter("addr", addr).andReturnValue(ret);
}
void expect_write(uint16_t addr, uint8_t val, bool ret) {
    mock().expectOneCall("mock_bus_write").withParameter("addr", addr).withParameter("val", val).andReturnValue(ret);
}
}  // namespace mock_bus

TEST_GROUP(C6502TestGroup) {
    C6502 c;

    TEST_SETUP() {
        memset(&c, 0, sizeof(c));
        c.bus_interface = &bus;
        // c6502_reset(&c);
        // c.cycles_remaining = 0;
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(C6502TestGroup, test_reset) {
    mock_bus::expect_read(0xFFFC, 0x12);
    mock_bus::expect_read(0xFFFD, 0x13);
    c6502_reset(&c);
    CHECK_EQUAL(0x1312, c.PC);

    // Todo - check rest of reset stuff
}

// ASL ZP 5
TEST(C6502TestGroup, test_0x06) {
    c.SR.u8 = 0;
    c.PC = 123;
    mock_bus::expect_read(123, 0x06);
    mock_bus::expect_read(124, 51);
    mock_bus::expect_read(51, 0b01001111);
    mock_bus::expect_write(51, 0b01001111, true);
    mock_bus::expect_write(51, 0b10011110, true);
    CHECK_EQUAL(5, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.SR.C, 0);
    CHECK_EQUAL(c.SR.N, 1);
    CHECK_EQUAL(c.SR.Z, 0);

    c.SR.u8 = 0;
    c.PC = 123;
    mock_bus::expect_read(123, 0x06);
    mock_bus::expect_read(124, 51);
    mock_bus::expect_read(51, 0b10000000);
    mock_bus::expect_write(51, 0b10000000, true);
    mock_bus::expect_write(51, 0, true);
    CHECK_EQUAL(5, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.SR.C, 1);
    CHECK_EQUAL(c.SR.N, 0);
    CHECK_EQUAL(c.SR.Z, 1);
}

// ASL ACC 2
TEST(C6502TestGroup, test_0x0A) {
    c.SR.u8 = 0;
    c.PC = 900;
    mock_bus::expect_read(900, 0x0A);
    c.AC = 0b10000001;
    CHECK_EQUAL(2, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.AC, 0b00000010);
    CHECK_EQUAL(c.PC, 901);
    CHECK_EQUAL(c.SR.N, 0);
    CHECK_EQUAL(c.SR.C, 1);
    CHECK_EQUAL(c.SR.Z, 0);
}

// ASL ABS 6
TEST(C6502TestGroup, test_0x0E) {
    c.PC = 2000;
    mock_bus::expect_read(2000, 0x0E);
    mock_bus::expect_read(2001, 0x34);
    mock_bus::expect_read(2002, 0x12);
    mock_bus::expect_read(0x1234, 0b01010101);
    mock_bus::expect_write(0x1234, 0b01010101, true);  // double-write for read-modify-write
    mock_bus::expect_write(0x1234, 0b10101010, true);
    CHECK_EQUAL(6, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2003);
}

// BPL REL 2''
TEST(C6502TestGroup, test_0x10) {
    c.PC = 2000;
    mock_bus::expect_read(2000, 0x10);
    mock_bus::expect_read(2001, 124);

    c.SR.N = 0;
    CHECK_EQUAL(4, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2126);

    c.PC = 2000;
    mock_bus::expect_read(2000, 0x10);
    mock_bus::expect_read(2001, -124);
    c.SR.N = 0;
    CHECK_EQUAL(3, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2000 - 122);

    c.PC = 2000;
    mock_bus::expect_read(2000, 0x10);
    mock_bus::expect_read(2001, -124);
    c.SR.N = 1;
    CHECK_EQUAL(2, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 2002);
}

// ORA INY 5'
TEST(C6502TestGroup, test_0x11) {
    c.PC = 500;
    c.Y = 0x22;
    mock_bus::expect_read(500, 0x11);
    mock_bus::expect_read(501, 34);
    mock_bus::expect_read(34, 0x12);
    mock_bus::expect_read(35, 0x34);
    mock_bus::expect_read(0x3434, 0b11001100);

    c.AC = 0b00000001;
    CHECK_EQUAL(5, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.AC, 0b11001101);
    CHECK_EQUAL(c.SR.Z, 0);
    CHECK_EQUAL(c.SR.N, 1);

    c.PC = 500;
    c.Y = 0xF0;
    mock_bus::expect_read(500, 0x11);
    mock_bus::expect_read(501, 34);
    mock_bus::expect_read(34, 0x12);
    mock_bus::expect_read(35, 0x34);
    mock_bus::expect_read(0x3502, 0b10);
    c.AC = 0b1;
    CHECK_EQUAL(6, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.AC, 0b11);
    CHECK_EQUAL(c.SR.Z, 0);
    CHECK_EQUAL(c.SR.N, 0);
}

// ORA ZPX 4
TEST(C6502TestGroup, test_0x15) {
    c.PC = 800;
    mock_bus::expect_read(800, 0x15);
    mock_bus::expect_read(801, 0x30);
    mock_bus::expect_read(0x30, 0b1010);
    c.AC = 0b1001;
    CHECK_EQUAL(4, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.AC, 0b1011);
}

// ASL ZPX 6
TEST(C6502TestGroup, test_0x16) {
    c.PC = 1000;
    mock_bus::expect_read(1000, 0x16);
    mock_bus::expect_read(1001, 0x85);
    c.X = 0x82;
    mock_bus::expect_read(7, 0b00001111);
    mock_bus::expect_write(7, 0b00001111, true);
    mock_bus::expect_write(7, 0b00011110, true);
    CHECK_EQUAL(6, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 1002);
}

// CLC IMP 2
TEST(C6502TestGroup, test_0x18) {
    c.PC = 500;
    mock_bus::expect_read(500, 0x18);

    c.SR.u8 = ~0;
    c.SR.C = 1;

    CHECK_EQUAL(2, c6502_run_next_instruction(&c));
    CHECK_EQUAL(c.PC, 501);
    CHECK_EQUAL(c.SR.C, 0);
    CHECK_EQUAL(c.SR.u8, 0b11111110);
}

// Todo - remaining cpu tests
// BRK IMP 7
// TEST(C6502TestGroup, test_0x00) {}

// ORA INDX 6
// TEST(C6502TestGroup, test_0x01) {}

// ORA ZP 3
// TEST(C6502TestGroup, test_0x05) {}

// PHP IMP 3
// TEST(C6502TestGroup, test_0x08) {}

// ORA IMM 2
// TEST(C6502TestGroup, test_0x09) {}

// ORA ABS 4
// TEST(C6502TestGroup, test_0x0D) {}

// ORA ABS Y 4'
// TEST(C6502TestGroup, test_0x19) {}

// ORA ABS X 4'
// TEST(C6502TestGroup, test_0x1D) {}

// ASL ABS X 7
// TEST(C6502TestGroup, test_0x1E) {}

// JSR ABS 6
// TEST(C6502TestGroup, test_0x20) {}

// AND INX 6
// TEST(C6502TestGroup, test_0x21) {}

// BIT ZP 3
// TEST(C6502TestGroup, test_0x24) {}

// AND ZP 3
// TEST(C6502TestGroup, test_0x25) {}

// ROL ZP 5
// TEST(C6502TestGroup, test_0x26) {}

// PLP IMP 4
// TEST(C6502TestGroup, test_0x28) {}

// AND IMM 2
// TEST(C6502TestGroup, test_0x29) {}

// ROL ACC 2
// TEST(C6502TestGroup, test_0x2A) {}

// BIT ABS 4
// TEST(C6502TestGroup, test_0x2C) {}

// AND ABS 4
// TEST(C6502TestGroup, test_0x2D) {}

// ROL ABS 6
// TEST(C6502TestGroup, test_0x2E) {}

// BMI REL 2''
// TEST(C6502TestGroup, test_0x30) {}

// AND INY 5'
// TEST(C6502TestGroup, test_0x31) {}

// AND ZPX 4
// TEST(C6502TestGroup, test_0x35) {}

// ROL ZPX 6
// TEST(C6502TestGroup, test_0x36) {}

// SEC IMP 2
// TEST(C6502TestGroup, test_0x38) {}

// AND ABY 4'
// TEST(C6502TestGroup, test_0x39) {}
// TEST(C6502TestGroup, test_0x3A) {}

// TEST(C6502TestGroup, test_0x3C) {}
// TEST(C6502TestGroup, test_0x3D) {}
// TEST(C6502TestGroup, test_0x3E) {}

// TEST(C6502TestGroup, test_0x40) {}
// TEST(C6502TestGroup, test_0x41) {}

// TEST(C6502TestGroup, test_0x44) {}
// TEST(C6502TestGroup, test_0x45) {}
// TEST(C6502TestGroup, test_0x46) {}

// TEST(C6502TestGroup, test_0x48) {}
// TEST(C6502TestGroup, test_0x49) {}
// TEST(C6502TestGroup, test_0x4A) {}

// TEST(C6502TestGroup, test_0x4C) {}
// TEST(C6502TestGroup, test_0x4D) {}
// TEST(C6502TestGroup, test_0x4E) {}

// TEST(C6502TestGroup, test_0x50) {}
// TEST(C6502TestGroup, test_0x51) {}

// TEST(C6502TestGroup, test_0x54) {}
// TEST(C6502TestGroup, test_0x55) {}
// TEST(C6502TestGroup, test_0x56) {}

// TEST(C6502TestGroup, test_0x58) {}
// TEST(C6502TestGroup, test_0x59) {}
// TEST(C6502TestGroup, test_0x5A) {}

// TEST(C6502TestGroup, test_0x5C) {}
// TEST(C6502TestGroup, test_0x5D) {}
// TEST(C6502TestGroup, test_0x5E) {}

// TEST(C6502TestGroup, test_0x60) {}
// TEST(C6502TestGroup, test_0x61) {}

// TEST(C6502TestGroup, test_0x64) {}
// TEST(C6502TestGroup, test_0x65) {}
// TEST(C6502TestGroup, test_0x66) {}

// TEST(C6502TestGroup, test_0x68) {}
// TEST(C6502TestGroup, test_0x69) {}
// TEST(C6502TestGroup, test_0x6A) {}

// TEST(C6502TestGroup, test_0x6C) {}
// TEST(C6502TestGroup, test_0x6D) {}
// TEST(C6502TestGroup, test_0x6E) {}

// TEST(C6502TestGroup, test_0x70) {}
// TEST(C6502TestGroup, test_0x71) {}

// TEST(C6502TestGroup, test_0x74) {}
// TEST(C6502TestGroup, test_0x75) {}
// TEST(C6502TestGroup, test_0x76) {}

// TEST(C6502TestGroup, test_0x78) {}
// TEST(C6502TestGroup, test_0x79) {}
// TEST(C6502TestGroup, test_0x7A) {}

// TEST(C6502TestGroup, test_0x7C) {}
// TEST(C6502TestGroup, test_0x7D) {}
// TEST(C6502TestGroup, test_0x7E) {}

// TEST(C6502TestGroup, test_0x81) {}

// TEST(C6502TestGroup, test_0x84) {}
// TEST(C6502TestGroup, test_0x85) {}
// TEST(C6502TestGroup, test_0x86) {}

// TEST(C6502TestGroup, test_0x88) {}
// TEST(C6502TestGroup, test_0x89) {}
// TEST(C6502TestGroup, test_0x8A) {}

// TEST(C6502TestGroup, test_0x8C) {}
// TEST(C6502TestGroup, test_0x8D) {}
// TEST(C6502TestGroup, test_0x8E) {}

// TEST(C6502TestGroup, test_0x90) {}
// TEST(C6502TestGroup, test_0x91) {}

// TEST(C6502TestGroup, test_0x94) {}
// TEST(C6502TestGroup, test_0x95) {}
// TEST(C6502TestGroup, test_0x96) {}

// TEST(C6502TestGroup, test_0x98) {}
// TEST(C6502TestGroup, test_0x99) {}
// TEST(C6502TestGroup, test_0x9A) {}

// TEST(C6502TestGroup, test_0x9C) {}
// TEST(C6502TestGroup, test_0x9D) {}
// TEST(C6502TestGroup, test_0x9E) {}

// TEST(C6502TestGroup, test_0xA0) {}
// TEST(C6502TestGroup, test_0xA1) {}
// TEST(C6502TestGroup, test_0xA2) {}

// TEST(C6502TestGroup, test_0xA4) {}
// TEST(C6502TestGroup, test_0xA5) {}
// TEST(C6502TestGroup, test_0xA6) {}

// TEST(C6502TestGroup, test_0xA8) {}
// TEST(C6502TestGroup, test_0xA9) {}
// TEST(C6502TestGroup, test_0xAA) {}

// TEST(C6502TestGroup, test_0xAC) {}
// TEST(C6502TestGroup, test_0xAD) {}
// TEST(C6502TestGroup, test_0xAE) {}

// TEST(C6502TestGroup, test_0xB0) {}
// TEST(C6502TestGroup, test_0xB1) {}

// TEST(C6502TestGroup, test_0xB4) {}
// TEST(C6502TestGroup, test_0xB5) {}
// TEST(C6502TestGroup, test_0xB6) {}

// TEST(C6502TestGroup, test_0xB8) {}
// TEST(C6502TestGroup, test_0xB9) {}
// TEST(C6502TestGroup, test_0xBA) {}

// TEST(C6502TestGroup, test_0xBC) {}
// TEST(C6502TestGroup, test_0xBD) {}
// TEST(C6502TestGroup, test_0xBE) {}

// TEST(C6502TestGroup, test_0xC0) {}
// TEST(C6502TestGroup, test_0xC1) {}

// TEST(C6502TestGroup, test_0xC4) {}
// TEST(C6502TestGroup, test_0xC5) {}
// TEST(C6502TestGroup, test_0xC6) {}

// TEST(C6502TestGroup, test_0xC8) {}
// TEST(C6502TestGroup, test_0xC9) {}
// TEST(C6502TestGroup, test_0xCA) {}

// TEST(C6502TestGroup, test_0xCC) {}
// TEST(C6502TestGroup, test_0xCD) {}
// TEST(C6502TestGroup, test_0xCE) {}

// TEST(C6502TestGroup, test_0xD0) {}
// TEST(C6502TestGroup, test_0xD1) {}

// TEST(C6502TestGroup, test_0xD4) {}
// TEST(C6502TestGroup, test_0xD5) {}
// TEST(C6502TestGroup, test_0xD6) {}

// TEST(C6502TestGroup, test_0xD8) {}
// TEST(C6502TestGroup, test_0xD9) {}
// TEST(C6502TestGroup, test_0xDA) {}

// TEST(C6502TestGroup, test_0xDC) {}
// TEST(C6502TestGroup, test_0xDD) {}
// TEST(C6502TestGroup, test_0xDE) {}

// TEST(C6502TestGroup, test_0xE0) {}
// TEST(C6502TestGroup, test_0xE1) {}

// TEST(C6502TestGroup, test_0xE4) {}
// TEST(C6502TestGroup, test_0xE5) {}
// TEST(C6502TestGroup, test_0xE6) {}

// TEST(C6502TestGroup, test_0xE8) {}
// TEST(C6502TestGroup, test_0xE9) {}
// TEST(C6502TestGroup, test_0xEA) {}

// TEST(C6502TestGroup, test_0xEC) {}
// TEST(C6502TestGroup, test_0xED) {}
// TEST(C6502TestGroup, test_0xEE) {}

// TEST(C6502TestGroup, test_0xF0) {}
// TEST(C6502TestGroup, test_0xF1) {}

// TEST(C6502TestGroup, test_0xF4) {}
// TEST(C6502TestGroup, test_0xF5) {}
// TEST(C6502TestGroup, test_0xF6) {}

// TEST(C6502TestGroup, test_0xF8) {}
// TEST(C6502TestGroup, test_0xF9) {}
// TEST(C6502TestGroup, test_0xFA) {}

// TEST(C6502TestGroup, test_0xFC) {}
// TEST(C6502TestGroup, test_0xFD) {}
// TEST(C6502TestGroup, test_0xFE) {}
