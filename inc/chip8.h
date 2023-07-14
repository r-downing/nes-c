#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32

typedef enum {
    CHIP8_ERROR_NONE = 0,
    CHIP8_ERROR_UNSPECIFIED,
    CHIP8_ERROR_INVALID_ROM_SIZE,
    CHIP8_ERROR_UNKNOWN_OPCODE,
    CHIP8_ERROR_STACK_UNDERFLOW,
    CHIP8_ERROR_STACK_OVERFLOW,
    CHIP8_ERROR_INVALID_KEY_INDEX,
} CHIP8_ERROR;

typedef struct {
    // working memory
    uint8_t mem[0x1000];

    // program counter
    uint16_t pc;

    // stack pointer
    uint8_t sp;

    // call stack return addresses
    uint16_t stack[16];

    // video memory
    uint8_t video[CHIP8_SCREEN_HEIGHT][CHIP8_SCREEN_WIDTH];

    // key states
    uint8_t key[16];

    // general purpose registers
    uint8_t v[16];

    // address pointer
    uint16_t i;

    uint32_t delay_exp;
    bool delay_active;

} Chip8;

// chip8_font_set.c
extern const uint8_t FONT_SET[];
extern const size_t FONT_SET_SIZE;

/** empties registers and memory and resets the program-counter to the start
 * address. */
void chip8_reset(Chip8 *);

CHIP8_ERROR chip8_load_rom_data(Chip8 *, const uint8_t *rom_data,
                                uint16_t length);

CHIP8_ERROR chip8_load_instructions(Chip8 *, const uint16_t *instructions,
                                    uint16_t num_instructions);

CHIP8_ERROR chip8_cycle(Chip8 *);

size_t chip8_load_file(Chip8 *, FILE *);
