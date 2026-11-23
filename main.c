#include <stdlib.h>
#include <raylib.h>

#include "chip8.h"

// Increase this value to your liking
#define RENDER_SCALE  1

#define WINDOW_WIDTH  (64 * (10 * RENDER_SCALE))
#define WINDOW_HEIGHT (32 * (10 * RENDER_SCALE))

const KeyboardKey scancode[16] =
{
    KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,
    KEY_Q, KEY_W, KEY_E, KEY_A,
    KEY_S, KEY_D, KEY_Z, KEY_C,
    KEY_FOUR, KEY_R, KEY_F, KEY_V
};

void main_cycle(struct CHIP8* chip8)
{
    for (uint8_t count = 10; count > 0; count--) chip8_run_cycle(chip8);
    ClearBackground(BLACK);
    for (uint8_t y = 0; y < 32; y++)
    {
        for (uint8_t x = 0; x < 64; x++)
        {
            if (chip8->framebuffer[y][x]) DrawRectangle(x * (10 * RENDER_SCALE), y * (10 * RENDER_SCALE), (10 * RENDER_SCALE), (10 * RENDER_SCALE), WHITE);
        }
    }
}

int main (int argc, char *argv[])
{
    struct CHIP8 chip8;
    chip8_init(&chip8);

    // Load ROM from command line
    uint8_t rom_loaded = 0;
    if (argc > 1) { if ((load_rom(argv[1], &chip8))) return EXIT_FAILURE; else rom_loaded = 1; }

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CHIP-8 Interpreter");
    SetTargetFPS(60);

    // Main program loop
    while (!WindowShouldClose())
    {
        for (uint8_t key = 0; key < 16; key++)
        {
            if (IsKeyDown(scancode[key])) chip8.keypad |= (1U << key);
            else chip8.keypad &= ~(1U << key);
        }
        BeginDrawing();

        if (rom_loaded)
        {
            main_cycle(&chip8);
            if (IsFileDropped())
            {
                FilePathList dropped_files = LoadDroppedFiles();
                if (dropped_files.count > 0)
                {
                    chip8_init(&chip8);
                    const char *file_path = dropped_files.paths[0];
                    load_rom(file_path, &chip8);

                }
                UnloadDroppedFiles(dropped_files);
            }
        }
        else
        {
            ClearBackground((Color){192, 192, 192 , 0});
            DrawText("Drop a ROM file here", 0, 0, 20 * RENDER_SCALE, BLACK);
            if (IsFileDropped())
            {
                FilePathList dropped_files = LoadDroppedFiles();
                if (dropped_files.count > 0)
                {
                    chip8_init(&chip8);
                    const char *file_path = dropped_files.paths[0];
                    if (!(load_rom(file_path, &chip8))) rom_loaded = 1;

                }
                UnloadDroppedFiles(dropped_files);
            }
        }
        EndDrawing();

        if (chip8.delay_timer > 0) chip8.delay_timer--;
        if (chip8.sound_timer > 0) chip8.sound_timer--;
    }

    CloseWindow();
    return 0;
}
