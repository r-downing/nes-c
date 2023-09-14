// #define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <assert.h>
#include <nes_bus.h>
#include <nes_cart.h>
#include <stdbool.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT (240 + 1)

static uint32_t scr[SCREEN_HEIGHT][SCREEN_WIDTH] = {{~0}};

static struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit_flag;
} s_ctx = {NULL};

NesBus bus = {0};

static typeof(s_ctx) *const s = &s_ctx;  // Todo - make ctx ptr a param

#define SCALE 4

void spg_init(void) {
    assert(0 == SDL_Init(SDL_INIT_VIDEO));
    // Todo: params
    s->window = SDL_CreateWindow("WindowTitle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE,
                                 SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    assert(s->window);

    s->renderer = SDL_CreateRenderer(s->window, -1, SDL_RENDERER_ACCELERATED);
    assert(s->renderer);
    SDL_RenderSetLogicalSize(s->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    s->texture = SDL_CreateTexture(s->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH,
                                   SCREEN_HEIGHT);
}

void spg_handle_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (SDL_QUIT == e.type) {
            s->quit_flag = true;
        } else if ((SDL_KEYDOWN == e.type) || (SDL_KEYUP == e.type)) {
            const int pressed = (SDL_KEYDOWN == e.type) ? 1 : 0;
            switch (e.key.keysym.sym) {
                case SDLK_UP: {
                    bus.gamepad.buttons.Up = pressed;
                    break;
                }
                case SDLK_DOWN: {
                    bus.gamepad.buttons.Down = pressed;
                    break;
                }
                case SDLK_LEFT: {
                    bus.gamepad.buttons.Left = pressed;
                    break;
                }
                case SDLK_RIGHT: {
                    bus.gamepad.buttons.Right = pressed;
                    break;
                }
                case SDLK_a: {
                    bus.gamepad.buttons.A = pressed;
                    break;
                }
                case SDLK_s: {
                    bus.gamepad.buttons.B = pressed;
                    break;
                }
                case SDLK_RETURN: {
                    bus.gamepad.buttons.Start = pressed;
                    break;
                }
                case SDLK_RSHIFT: {
                    bus.gamepad.buttons.Select = pressed;
                    break;
                }
            }
        }
    }
}

void spg_draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    scr[y][x] = 0xFF000000 | (r) | (g << 8) | (b << 16);
    // SDL_SetRenderDrawColor(s->renderer, r, g, b, SDL_ALPHA_OPAQUE);
    // SDL_RenderDrawPoint(s->renderer, x, y);
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

void render(void *, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    spg_draw_pixel(x, y, r, g, b);
}

int main(int argc, char **argv) {
    (void)argc;
    nes_cart_init(&bus.cart, argv[1]);
    nes_bus_init(&bus);
    bus.ppu.draw_pixel = render;
    spg_init();

#if 0
    for (int bank = 0; bank < 2; bank++) {
        for (int sx = 0; sx < 32; sx++) {
            for (int sy = 0; sy < 8; sy++) {
                nes_chr_tile mtile = {0};
                for (size_t sss = 0; sss < sizeof(mtile); sss++) {
                    mtile.u8_arr[sss] =
                        nes_bus_ppu_read(&bus, sss + (0x1000 * bank) + ((32 * sy + sx) * sizeof(mtile)));
                }

                const nes_chr_tile *const tile = &mtile;
                // const nes_chr_tile *const tile =
                //     (nes_chr_tile *)(&bus.cart.chr_rom[(0x1000 * bank) + ((32 * sy + sx) * sizeof(nes_chr_tile))]);

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
#endif

    // for (int i = 0; i < 21477272 / 4; i++) {
    //     nes_bus_cycle(&bus);
    // }
    while (!s->quit_flag) {
        const uint32_t loop_start_ms = SDL_GetTicks();
        while (bus.ppu.status.vblank) {
            nes_bus_cycle(&bus);
        }
        while (!bus.ppu.status.vblank) {
            nes_bus_cycle(&bus);
        }

        SDL_UpdateTexture(s->texture, NULL, scr, sizeof(scr[0]));
        SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);

        SDL_RenderPresent(s->renderer);
        spg_handle_events();

        const uint32_t elapsed_ms = SDL_GetTicks() - loop_start_ms;
        if (elapsed_ms < 17) {
            SDL_Delay(17 - elapsed_ms);  // Todo - make this better
        }
    }

    SDL_Quit();
    return 0;
}