#include <SDL2/SDL.h>
#include <emscripten.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "chip8.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static Chip8 c8 = {0};

bool running = false;
bool frozen = true;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320

bool init(void) {
    chip8_reset(&c8);

    FILE *const rom_file = fopen("assets/tetris_reassembled.ch8", "rb");
    if (!rom_file) {
        printf("Couldn't open rom file\n");
        return false;
    }

    const size_t loaded = chip8_load_file(&c8, rom_file);
    printf("loaded %lu bytes rom\n", loaded);

    fclose(rom_file);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initiliazed. SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("emscripten chip8", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_Window could not be initialized. SDL_Error: %s\n",
               SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    running = true;
    return true;
    ;
}

void quit_game(void);

void draw_square(int x, int y, int fill) {
    const SDL_Rect rect = {.x = x * 10, .y = y * 10, .w = 9, .h = 9};

    fill = fill ? 255 : 0;

    SDL_SetRenderDrawColor(renderer, 0, fill, 0, 255);
    SDL_RenderFillRect(renderer, &rect);
}

/*
CHIP8:
1 2 3 C
4 5 6 D
7 8 9 E
A 0 C F

QWERTY:
1 2 3 4
Q W E R
A S D F
Z X C V
*/
const SDL_Keycode chip8_keys[] = {
    [0] = SDLK_x,   [1] = SDLK_1,   [2] = SDLK_2,   [3] = SDLK_3,
    [4] = SDLK_q,   [5] = SDLK_w,   [6] = SDLK_e,   [7] = SDLK_a,
    [8] = SDLK_s,   [9] = SDLK_d,   [0xa] = SDLK_z, [0xb] = SDLK_c,
    [0xc] = SDLK_4, [0xd] = SDLK_r, [0xe] = SDLK_f, [0xf] = SDLK_v,
};

void handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit_game();
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            printf("key %u\n", e.key.keysym.scancode);
            for (size_t i = 0; i < sizeof(chip8_keys) / sizeof(chip8_keys[0]);
                 i++) {
                if (e.key.keysym.sym == chip8_keys[i]) {
                    c8.key[i] = (e.type == SDL_KEYDOWN) ? 1 : 0;
                    break;
                }
            }
        }
    }
}

void quit_game(void) {
    SDL_DestroyWindow(window);
    window = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_Quit();

#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
}

EMSCRIPTEN_KEEPALIVE
void pause(void) { frozen = !frozen; }

void main_loop(void) {
    // printf("asdf\n");
    handle_events();

    // if (frozen)
    //     return;

    for (int i = 0; i < 100; i++) {
        chip8_cycle(&c8);
    }

    SDL_SetRenderDrawColor(renderer, 18, 1, 54, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 32; y++) {
            draw_square(x, y, c8.video[y][x]);
        }
    }

    // SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    SDL_RenderPresent(renderer);

    // SDL_Delay(70);
}

EMSCRIPTEN_KEEPALIVE
void jscallback(uint8_t *data, size_t length) {
    printf("len %lu\n", length);
    printf("data[0] %u\n", data[0]);
    // chip8_load_rom_data(&c8, data, length);
    frozen = false;
}

EMSCRIPTEN_KEEPALIVE
int main(void) {
    printf("built %s @ %s\n", __DATE__, __TIME__);

    if (!init()) return -1;

    emscripten_set_main_loop(main_loop, 0, 1);

    quit_game();
    return 0;
}
