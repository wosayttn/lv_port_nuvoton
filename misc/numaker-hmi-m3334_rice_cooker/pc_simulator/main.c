/**************************************************************************//**
 * @file     main.c
 * @brief    Rice Cooker PC Simulator (SDL2 + LVGL)
 *
 * Build: mkdir build && cd build && cmake .. -DLVGL_PATH=<path> && cmake --build .
 * Run:   ./rice_cooker_sim
 *
 * Mouse  = touch input
 * Keys:  M = Menu, Up/Down = navigate, Enter = Start/Confirm
 *****************************************************************************/
#include "lvgl.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms)  Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms)  usleep((ms)*1000)
#endif

/*============================================================================
 * SDL event watcher (maps PC keys to rice cooker keys)
 *============================================================================*/

/* LVGL 9.x SDL driver handles mouse + keyboard automatically via
 * lv_sdl_window_create(). We use an SDL event watcher to intercept
 * key presses and defer dispatch to the LVGL timer context to avoid
 * re-entrancy issues. */
#include <SDL.h>

/* Ring buffer for deferred key events */
#define KEY_BUF_SIZE 16
static ui_key_t s_key_buf[KEY_BUF_SIZE];
static volatile int s_key_wr = 0;
static volatile int s_key_rd = 0;

static int sim_sdl_event_watcher(void *userdata, SDL_Event *event)
{
    (void)userdata;
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0)
    {
        ui_key_t key = (ui_key_t)-1;
        switch (event->key.keysym.scancode)
        {
            case SDL_SCANCODE_M:      key = UI_KEY_MENU;  break;
            case SDL_SCANCODE_UP:     key = UI_KEY_UP;    break;
            case SDL_SCANCODE_DOWN:   key = UI_KEY_DOWN;  break;
            case SDL_SCANCODE_RETURN: key = UI_KEY_START; break;
            default: break;
        }
        if ((int)key >= 0)
        {
            int next = (s_key_wr + 1) % KEY_BUF_SIZE;
            if (next != s_key_rd)
            {
                s_key_buf[s_key_wr] = key;
                s_key_wr = next;
            }
        }
    }
    return 1; /* allow event to propagate */
}

static void sim_key_dispatch_timer_cb(lv_timer_t *t)
{
    (void)t;
    while (s_key_rd != s_key_wr)
    {
        ui_key_t key = s_key_buf[s_key_rd];
        s_key_rd = (s_key_rd + 1) % KEY_BUF_SIZE;
        ui_key_pressed(key);
    }
}

/*============================================================================
 * Main
 *============================================================================*/
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("=== Rice Cooker PC Simulator ===\n");
    printf("Mouse click = touch\n");
    printf("Keys: M=Menu, Up/Down=Navigate, Enter=Start\n");
    printf("Press ESC or close window to quit.\n\n");

    /* Init LVGL */
    lv_init();

    /* Create SDL window (display) */
    lv_display_t *disp = lv_sdl_window_create(320, 240);
    lv_display_set_default(disp);

    /* Create mouse pointer input device (touch emulation) */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;

    /* Register SDL event watcher for key capture */
    SDL_AddEventWatch(sim_sdl_event_watcher, NULL);

    /* Timer to dispatch deferred key events safely within LVGL context */
    lv_timer_create(sim_key_dispatch_timer_cb, 20, NULL);

    /* Init our UI */
    ui_init();

    printf("UI initialized. Running...\n");

    /* Main loop */
    while (1)
    {
        uint32_t time_till_next = lv_timer_handler();
        SLEEP_MS(time_till_next < 1 ? 1 : time_till_next);
    }

    return 0;
}
