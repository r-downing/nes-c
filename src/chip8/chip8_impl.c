
#include "./chip8_impl.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

int chip8_impl_rand(void) { return rand(); }

uint32_t chip8_impl_ticks(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 60u) + (spec.tv_nsec / (1000000000ul / 60));
}
