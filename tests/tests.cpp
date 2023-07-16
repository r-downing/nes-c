#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <stdlib.h>
#include <string.h>

#include "../src/chip8_impl.h"
#include <chip8.h>
}

int chip8_impl_rand(void) {
    mock().actualCall(__func__);
    return mock().returnIntValueOrDefault(0);
}

uint32_t chip8_impl_ticks(void) {
    mock().actualCall(__func__);
    return mock().returnUnsignedIntValueOrDefault(0);
}

TEST_GROUP(Chip8TestGroup) {
    Chip8 c;
    Chip8 c2;

    TEST_SETUP() {
        // memset(&c, 0, sizeof(c));
        chip8_reset(&c);
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }

    void step_chip8(const CHIP8_ERROR expected) {
        c2 = c;      // make a snapshot of c before stepping
        c2.pc += 2;  // and increment pc as expected
        const CHIP8_ERROR err = chip8_cycle(&c);
        CHECK_EQUAL(expected, err);
    }
    void step_chip8(void) { step_chip8(CHIP8_ERROR_NONE); }
};

TEST(Chip8TestGroup, test_chip8_reset) {
    chip8_reset(&c);

    CHECK_EQUAL(0x200, c.pc);
    CHECK_EQUAL(0, memcmp(&c.mem[0x50], FONT_SET, FONT_SET_SIZE));
    uint8_t v_cmp[sizeof(c.v)] = {0};
    CHECK_EQUAL(0, memcmp(c.v, v_cmp, sizeof(v_cmp)));
}

// test load instructions
TEST(Chip8TestGroup, test_load_instr) {
    const uint16_t instr[(0x1000 - 0x200) / 2] = {0x1234, 0x5678};
    CHECK_EQUAL(
        CHIP8_ERROR_NONE,
        chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0]))));
    CHECK_EQUAL(0x12, c.mem[0x200]);
    CHECK_EQUAL(0x34, c.mem[0x201]);
    CHECK_EQUAL(0x56, c.mem[0x202]);
    CHECK_EQUAL(0x78, c.mem[0x203]);
}

// test load instructions too big
TEST(Chip8TestGroup, test_load_instr_fail) {
    const uint16_t instr[((0x1000 - 0x200) / 2) + 1] = {0};
    CHECK_EQUAL(
        CHIP8_ERROR_INVALID_ROM_SIZE,
        chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0]))));
}

// clear screen
TEST(Chip8TestGroup, test_00E0) {
    const uint16_t instr[] = {0x00E0};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.video[3][5] = 1;
    step_chip8();
    CHECK_EQUAL(0, c.video[3][5]);
    c2.video[3][5] = 0;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// return
TEST(Chip8TestGroup, test_00EE_ok) {
    const uint16_t instr[] = {0x00EE};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.stack[c.sp++] = 0x300;
    step_chip8();
    CHECK_EQUAL(0, c.sp);
    CHECK_EQUAL(0x300, c.pc);
    c2.sp = 0;
    c2.pc = 0x300;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// return w/ empty stack
TEST(Chip8TestGroup, test_00EE_error) {
    const uint16_t instr[] = {0x00EE};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8(CHIP8_ERROR_STACK_UNDERFLOW);
}

// goto
TEST(Chip8TestGroup, test_1NNN) {
    const uint16_t instr[] = {0x1234};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8();
    CHECK_EQUAL(0x234, c.pc);
    c2.pc = 0x234;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// call
TEST(Chip8TestGroup, test_2NNN) {
    const uint16_t instr[] = {0x2234};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8();
    CHECK_EQUAL(0x234, c.pc);
    CHECK_EQUAL(1, c.sp);  // should add to stack
    CHECK_EQUAL(0x202, c.stack[0]);
    c2.pc = 0x234;
    c2.sp = 1;
    c2.stack[0] = 0x202;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// call stack overflow
TEST(Chip8TestGroup, test_2NNN_error) {
    const uint16_t instr[] = {0x2234};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.sp = 16;
    step_chip8(CHIP8_ERROR_STACK_OVERFLOW);
}

// skip if Vx == NN, true
TEST(Chip8TestGroup, test_3XNN_true) {
    const uint16_t instr[] = {0x3712};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x204, c.pc);
    c2.pc = 0x204;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// skip if Vx == NN, false
TEST(Chip8TestGroup, test_3XNN_false) {
    const uint16_t instr[] = {0x3712};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x14;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    c2.pc = 0x202;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// skip if Vx != NN, true
TEST(Chip8TestGroup, test_4XNN_true) {
    const uint16_t instr[] = {0x4712};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x14;
    step_chip8();
    CHECK_EQUAL(0x204, c.pc);
    c2.pc = 0x204;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// skip if Vx != NN, false
TEST(Chip8TestGroup, test_4XNN_false) {
    const uint16_t instr[] = {0x4712};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    c2.pc = 0x202;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// skip if Vx == Vy, true
TEST(Chip8TestGroup, test_5XY0_true) {
    const uint16_t instr[] = {0x5790};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x12;
    c.v[9] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x204, c.pc);
    c2.pc = 0x204;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// skip if Vx == Vy, false
TEST(Chip8TestGroup, test_5XY0_false) {
    const uint16_t instr[] = {0x5790};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x14;
    c.v[9] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    c2.pc = 0x202;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 5XYR, R 1-F undefined
TEST(Chip8TestGroup, test_5XYR) {
    for (uint16_t i = 0x5791; i <= 0x579F; i++) {
        chip8_reset(&c);
        const uint16_t instr[] = {i};
        chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
        step_chip8(CHIP8_ERROR_UNKNOWN_OPCODE);
    }
}

// 6XNN, set Vx to NN
TEST(Chip8TestGroup, test_6XNN) {
    const uint16_t instr[] = {0x6581};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    CHECK_EQUAL(0x00, c.v[5]);
    step_chip8();
    CHECK_EQUAL(0x81, c.v[5]);
    c2.v[5] = 0x81;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 7XNN, Vx += NN, no carry
TEST(Chip8TestGroup, test_7XNN) {
    const uint16_t instr[] = {0x7986};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[9] = 0x85;
    step_chip8();
    CHECK_EQUAL((uint8_t)(0x85 + 0x86), c.v[9]);
    CHECK_EQUAL(0, c.v[0xF]);
    c2.v[9] = (uint8_t)(0x85 + 0x86);
    c2.v[0xF] = 0;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY0, Vx = Vy
TEST(Chip8TestGroup, test_8XY0) {
    const uint16_t instr[] = {0x8520};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x85;
    step_chip8();
    CHECK_EQUAL(0x85, c.v[5]);
    c2.v[5] = 0x85;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY1, Vx |= Vy
TEST(Chip8TestGroup, test_8XY1) {
    const uint16_t instr[] = {0x8521};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x10;
    c.v[5] = 0x14;
    step_chip8();
    CHECK_EQUAL(0x10 | 0x14, c.v[5]);
    c2.v[5] = c.v[5];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY2, Vx &= Vy
TEST(Chip8TestGroup, test_8XY2) {
    const uint16_t instr[] = {0x8522};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x13;
    c.v[5] = 0x15;
    step_chip8();
    CHECK_EQUAL(0x13 & 0x15, c.v[5]);
    c2.v[5] = c.v[5];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY3, Vx ^= Vy
TEST(Chip8TestGroup, test_8XY3) {
    const uint16_t instr[] = {0x8523};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x13;
    c.v[5] = 0x15;
    step_chip8();
    CHECK_EQUAL(0x13 ^ 0x15, c.v[5]);
    c2.v[5] = c.v[5];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY4, Vx += Vy, VF = 1 if carry
TEST(Chip8TestGroup, test_8XY4) {
    const uint16_t instr[] = {0x8524, 0x8524};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x71;
    c.v[5] = 0x35;
    c.v[0xF] = 1;
    step_chip8();
    CHECK_EQUAL(0x71 + 0x35, c.v[5]);
    CHECK_EQUAL(0, c.v[0xF]);
    c2.v[5] = c.v[5];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
    step_chip8();
    CHECK_EQUAL((uint8_t)(0x71 + 0x71 + 0x35), c.v[5]);
    CHECK_EQUAL(1, c.v[0xF]);
    c2.v[5] = c.v[5];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY5, Vx -= Vy, VF = 1 if no borrow
TEST(Chip8TestGroup, test_8XY5) {
    const uint16_t instr[] = {0x8525, 0x8525};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x42;
    c.v[5] = 0x73;
    c.v[0xF] = 0;
    step_chip8();
    CHECK_EQUAL(0x73 - 0x42, c.v[5]);
    CHECK_EQUAL(1, c.v[0xF]);
    c2.v[5] = c.v[5];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
    step_chip8();
    CHECK_EQUAL((uint8_t)(0x73 - 0x42 - 0x42), c.v[5]);
    CHECK_EQUAL(0, c.v[0xF]);
    c2.v[5] = c.v[5];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY6, Vx >>= 1, VF = LSB
TEST(Chip8TestGroup, test_8XY6) {
    const uint16_t instr[] = {0x8316, 0x8316};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[0xF] = 1;
    c.v[3] = 0x82;
    step_chip8();
    CHECK_EQUAL(0, c.v[0xF]);
    CHECK_EQUAL(0x41, c.v[3]);
    c2.v[3] = c.v[3];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
    step_chip8();
    CHECK_EQUAL(1, c.v[0xF]);
    CHECK_EQUAL(0x20, c.v[3]);
    c2.v[3] = c.v[3];
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// 8XY7, Vx = Vy - Vx, VF 1 if no borrow
TEST(Chip8TestGroup, test_8XY7) {
    const uint16_t instr[] = {0x8527, 0x8587};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[2] = 0x42;
    c.v[5] = 0x73;
    c.v[8] = 0xF0;
    c.v[0xF] = 1;
    step_chip8();
    CHECK_EQUAL(0x42, c.v[2]);
    uint8_t v5cmp = (0x42 - 0x73);
    CHECK_EQUAL(v5cmp, c.v[5]);
    CHECK_EQUAL(0, c.v[0xF]);
    step_chip8();
    CHECK_EQUAL(0x42, c.v[2]);
    CHECK_EQUAL(0xF0, c.v[8]);
    v5cmp = 0xF0 - v5cmp;
    CHECK_EQUAL(v5cmp, c.v[5]);
    CHECK_EQUAL(1, c.v[0xF]);
}

// 8XYE, Vx <<= 1, VF = MSB
TEST(Chip8TestGroup, test_8XYE) {
    const uint16_t instr[] = {0x831E, 0x831E};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[0xF] = 1;
    c.v[3] = 0x42;
    step_chip8();
    CHECK_EQUAL(0, c.v[0xF]);
    CHECK_EQUAL(0x84, c.v[3]);
    step_chip8();
    CHECK_EQUAL(1, c.v[0xF]);
    CHECK_EQUAL(0x08, c.v[3]);
}

// 8XYR, R [8-D, F] - invalid
TEST(Chip8TestGroup, test_8XYR) {
    for (uint16_t i = 0x8008; i <= 0x800D; i++) {
        chip8_reset(&c);
        const uint16_t instr[] = {i};
        chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
        step_chip8(CHIP8_ERROR_UNKNOWN_OPCODE);
    }
}

// skip if Vx != Vy, true
TEST(Chip8TestGroup, test_9XY0_true) {
    const uint16_t instr[] = {0x9790};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x13;
    c.v[9] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x204, c.pc);
}

// skip if Vx != Vy, false
TEST(Chip8TestGroup, test_9XY0_false) {
    const uint16_t instr[] = {0x9790};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[7] = 0x12;
    c.v[9] = 0x12;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
}

// 9XYR, R 1-F undefined
TEST(Chip8TestGroup, test_9XYR) {
    for (uint16_t i = 0x9791; i <= 0x979F; i++) {
        chip8_reset(&c);
        const uint16_t instr[] = {i};
        chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
        step_chip8(CHIP8_ERROR_UNKNOWN_OPCODE);
    }
}

// ANNN, I = NNN
TEST(Chip8TestGroup, test_ANNN) {
    const uint16_t instr[] = {0xA790};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8();
    CHECK_EQUAL(0x790, c.i);
}

// PC = V0 + NNN
TEST(Chip8TestGroup, test_BNNN) {
    const uint16_t instr[] = {0xB750};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[0] = 0x46;
    step_chip8();
    CHECK_EQUAL(0x796, c.pc);
}

// Vx = rand() & NN
TEST(Chip8TestGroup, test_CXNN) {
    const uint16_t instr[] = {0xC30E};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    mock().expectOneCall("chip8_impl_rand").andReturnValue(0x1C);
    step_chip8();
    CHECK_EQUAL((0x0E & 0x1C), c.v[3]);
}

// draw 8xN sprite at Vx, Vy, VF = 1 if collision
TEST(Chip8TestGroup, test_DXYN) {
    const uint16_t instr[] = {0xD572, 0xD571};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.i = 0x700;
    c.mem[0x700] = 0b10100000;
    c.mem[0x701] = 0b00000001;
    c.v[5] = 2;
    c.v[7] = 3;
    step_chip8();
    CHECK_EQUAL(1, c.video[3][2]);
    CHECK_EQUAL(1, c.video[3][4]);
    CHECK_EQUAL(1, c.video[4][9]);
    c2.video[3][2] = c.video[3][2];
    c2.video[3][4] = c.video[3][4];
    c2.video[4][9] = c.video[4][9];
    CHECK_EQUAL(0, c.v[0xF]);
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
    step_chip8();

    CHECK_EQUAL(0, c.video[3][2]);
    CHECK_EQUAL(0, c.video[3][4]);
    CHECK_EQUAL(1, c.video[4][9]);
    c2.video[3][2] = c.video[3][2];
    c2.video[3][4] = c.video[3][4];
    c2.video[4][9] = c.video[4][9];
    CHECK_EQUAL(1, c.v[0xF]);
    c2.v[0xF] = c.v[0xF];
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// if key[Vx], skip next
TEST(Chip8TestGroup, test_EX9E) {
    const uint16_t instr[] = {0xE39E, 0xE39E, 0x0000, 0xE59E};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[3] = 6;
    c.v[5] = 100;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    c.key[6] = 1;
    step_chip8();
    CHECK_EQUAL(0x206, c.pc);
    step_chip8(CHIP8_ERROR_INVALID_KEY_INDEX);
}

// if not key[Vx], skip next
TEST(Chip8TestGroup, test_EXA1) {
    const uint16_t instr[] = {0xE3A1, 0xE3A1, 0x0000, 0xE5A1};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[3] = 6;
    c.v[5] = 100;
    c.key[6] = 1;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    c.key[6] = 0;
    step_chip8();
    CHECK_EQUAL(0x206, c.pc);
    step_chip8(CHIP8_ERROR_INVALID_KEY_INDEX);
}

TEST(Chip8TestGroup, test_EXRR) {
    const uint16_t instr[] = {0xE3A2};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8(CHIP8_ERROR_UNKNOWN_OPCODE);
}

// Vx = delay()
TEST(Chip8TestGroup, test_FX07) {
    const uint16_t instr[] = {0xFA07, 0xFA07};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));

    c.delay_active = true;
    c.delay_exp = 0x3050;

    mock().expectOneCall("chip8_impl_ticks").andReturnValue(0x3010);
    step_chip8();
    CHECK_TRUE(c.delay_active);
    CHECK_EQUAL(0x40, c.v[0xA]);

    mock().expectOneCall("chip8_impl_ticks").andReturnValue(0x3051);
    step_chip8();
    CHECK_FALSE(c.delay_active);
    CHECK_EQUAL(0, c.v[0xA]);
    c2.delay_active = false;
    c2.v[0xA] = 0;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// Vx = get_key(), blocking
TEST(Chip8TestGroup, test_FX0A) {
    const uint16_t instr[] = {0xF20A};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8();
    CHECK_EQUAL(0x200, c.pc);
    c.key[7] = 1;
    step_chip8();
    CHECK_EQUAL(0x202, c.pc);
    CHECK_EQUAL(7, c.v[2]);
}

// delay = Vx
TEST(Chip8TestGroup, test_FX15) {
    const uint16_t instr[] = {0xF915};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[9] = 0x50;
    mock().expectOneCall("chip8_impl_ticks").andReturnValue(0x2000);
    step_chip8();
    CHECK_TRUE(c.delay_active);
    CHECK_EQUAL(0x2050, c.delay_exp);
}

// sound = Vx
TEST(Chip8TestGroup, test_FX18) {
    // CHECK_TRUE(0); // ToDo
}

// I += Vx
TEST(Chip8TestGroup, test_FX1E) {
    const uint16_t instr[] = {0xF51E};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[5] = 0x46;
    c.i = 0x120;
    step_chip8();
    CHECK_EQUAL(0x166, c.i);
}

// I = font[Vx]
TEST(Chip8TestGroup, test_FX29) {
    const uint16_t instr[] = {0xFA29};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[0xA] = 0x6;
    step_chip8();
    CHECK_EQUAL((6 * 5) + 0x50, c.i);
    c2.i = c.i;
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// I[0:3] = BCD(Vx)
TEST(Chip8TestGroup, test_FX33) {
    const uint16_t instr[] = {0xFA33};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    c.v[0xA] = 139;
    c.i = 0x700;
    step_chip8();
    CHECK_EQUAL(0x700, c.i);
    CHECK_EQUAL(c.mem[0x700], 1);
    CHECK_EQUAL(c.mem[0x701], 3);
    CHECK_EQUAL(c.mem[0x702], 9);
}

// I[0:X] = V0:Vx, inclusive
TEST(Chip8TestGroup, test_FX55) {
    const uint16_t instr[] = {0xF455};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    const uint8_t data[] = {0x31, 0x52, 0x11, 0x25, 0x53};
    memcpy(c.v, data, sizeof(data));
    c.i = 0x700;
    step_chip8();
    CHECK_EQUAL(0, memcmp(&c.mem[0x700], data, sizeof(data)));
    memcpy(&c2.mem[0x700], data, sizeof(data));
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// V0:Vx = I[0:X]
TEST(Chip8TestGroup, test_FX65) {
    const uint16_t instr[] = {0xF465};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    const uint8_t data[] = {0x31, 0x52, 0x11, 0x25, 0x53};
    memcpy(&c.mem[0x700], data, sizeof(data));
    c.i = 0x700;
    step_chip8();
    // CHECK_EQUAL(0x700, c.i);
    CHECK_EQUAL(0, memcmp(c.v, data, sizeof(data)));
    memcpy(c2.v, data, sizeof(data));
    CHECK_EQUAL(0, memcmp(&c, &c2, sizeof(c)));
}

// undefined
TEST(Chip8TestGroup, test_FXRR) {
    const uint16_t instr[] = {0xF366};
    chip8_load_instructions(&c, instr, (sizeof(instr) / sizeof(instr[0])));
    step_chip8(CHIP8_ERROR_UNKNOWN_OPCODE);
}

#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char **argv) { return RUN_ALL_TESTS(argc, argv); }
