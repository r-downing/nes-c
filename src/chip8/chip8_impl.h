#pragma once

#include <stdint.h>

/** random number */
int chip8_impl_rand(void);

/** chip8 60hz realtime tick counter*/
uint32_t chip8_impl_ticks(void);
