#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_gamepad.h>
#include <stdlib.h>
#include <string.h>
}

TEST_GROUP(NesGamepadTestGroup) {
    NesGamepad g;

    TEST_SETUP() {}

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(NesGamepadTestGroup, test_gamepad_none_pressed) {
    // none pressed. all 0
    for (int i = 0; i < 8; i++) {
        CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));
    }
    // after shifting through all, should return 1 forever
    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));
    }
}

TEST(NesGamepadTestGroup, test_gamepad_some_pressed) {
    g.buttons.Up = 1;
    g.buttons.Select = 1;
    g.buttons.B = 1;

    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // A
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // B
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Select
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Start
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Up
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Down
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Left
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Right

    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));
    }
}

TEST(NesGamepadTestGroup, test_gamepad_strobe) {
    g.buttons.Start = 1;
    g.buttons.Down = 1;
    g.buttons.Left = 1;
    // strobe enabled - should repeatedly give value of A
    nes_gamepad_write_reg(&g, 0x01);

    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));
    }
    g.buttons.A = 1;
    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));
    }
    g.buttons.A = 0;
    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));
    }

    nes_gamepad_write_reg(&g, 0x00);

    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // A
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // B
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Select
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Start
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Up
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Down
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Left
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Right
    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));
    }

    // toggling strobe should reset to A button
    nes_gamepad_write_reg(&g, 0x01);
    nes_gamepad_write_reg(&g, 0x00);

    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // A
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // B
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Select
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Start
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Up
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Down
    CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));  // Left
    CHECK_EQUAL(0, 1 & nes_gamepad_read_reg(&g));  // Right
    for (int i = 0; i < 1000; i++) {
        CHECK_EQUAL(1, 1 & nes_gamepad_read_reg(&g));
    }
}
