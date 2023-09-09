// #define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <assert.h>
#include <nes_cart.h>
#include <stdbool.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240

static struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool quit_flag;
} s_ctx = {NULL};

static typeof(s_ctx) *const s = &s_ctx;  // Todo - make ctx ptr a param

void spg_init(void) {
    assert(0 == SDL_Init(SDL_INIT_VIDEO));
    // Todo: params
    s->window = SDL_CreateWindow("WindowTitle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * 3,
                                 SCREEN_HEIGHT * 3, SDL_WINDOW_SHOWN);
    assert(s->window);

    s->renderer = SDL_CreateRenderer(s->window, -1, SDL_RENDERER_ACCELERATED);
    assert(s->renderer);
    SDL_RenderSetLogicalSize(s->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void spg_handle_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (SDL_QUIT == e.type) {
            s->quit_flag = true;
        }
    }
}

void spg_draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(s->renderer, r, g, b, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(s->renderer, x, y);
}

void spg_fill(uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(s->renderer, r, g, b, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(s->renderer);
}

#define BIT(x, bit) (((x) >> (bit)) & 1)
typedef union __attribute__((__packed__)) {
    uint8_t u8_arr[16];
    struct __attribute__((__packed__)) {
        uint8_t hi_plane[8];
        uint8_t lo_plane[8];
    };
} nes_chr_tile;

int main(int argc, char **argv) {
    NesCart cart = {0};
    (void)argc;
    nes_cart_init(&cart, argv[1]);
    spg_init();

    for (int bank = 0; bank < 2; bank++) {
        for (int sx = 0; sx < 32; sx++) {
            for (int sy = 0; sy < 8; sy++) {
                const nes_chr_tile *const tile =
                    (nes_chr_tile *)(&cart.chr_rom[(0x1000 * bank) + ((32 * sy + sx) * sizeof(nes_chr_tile))]);

                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        const uint8_t c = (BIT(tile->hi_plane[y], 7 - x) << 1) | BIT(tile->lo_plane[y], 7 - x);
                        spg_draw_pixel(x + (8 * sx), (bank * 64) + y + (8 * sy), (c & 1) ? 255 : 0, (c & 2) ? 255 : 0,
                                       0);
                    }
                }
            }
        }
    }
    while (!s->quit_flag) {
        SDL_RenderPresent(s->renderer);
        spg_handle_events();
    }

    SDL_Quit();
    return 0;
}