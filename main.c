#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <time.h>

#include "chip8.h"

// GOAL: Improve portability by handling ROM loading and RNG seed generation on front-end, simplify UI state

/* Preprocessor directives */
// Increase this value to your liking (default: 640x320)
#define RENDER_SCALE  10

#define WINDOW_WIDTH  (64 * RENDER_SCALE)
#define WINDOW_HEIGHT (32 * RENDER_SCALE)

/* Global constants & variables */
bool rom_loaded = false;
const KeyboardKey chip8_layout[16] =
{
    KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,
    KEY_Q, KEY_W, KEY_E, KEY_A,
    KEY_S, KEY_D, KEY_Z, KEY_C,
    KEY_FOUR, KEY_R, KEY_F, KEY_V
};

/* Function declarations */
int load_rom(const char* filename, struct CHIP8* chip8);
void main_cycle(struct CHIP8* chip8);

/* Main program */
int main (int argc, char *argv[])
{
    // Create CHIP-8 instance
    struct CHIP8 chip8;

    // Pass CHIP-8 instance and RNG seed to CHIP-8 init function
    chip8_init(&chip8, (uint32_t)time(NULL));

    // Load ROM from command line if argument is present
    if ((argc > 1) && (load_rom(argv[1], &chip8) == 0)) rom_loaded = true;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CHIP-8 Interpreter");
    SetTargetFPS(60);

    /* Main program loop */
    while (!WindowShouldClose())
    {
        if (IsFileDropped())
        {
            FilePathList dropped_files = LoadDroppedFiles();
            if (dropped_files.count > 0)
            {
                chip8_init(&chip8, (uint32_t)time(NULL));
                const char *file_path = dropped_files.paths[0];
                rom_loaded = (load_rom(file_path, &chip8) == 0);
            }
            UnloadDroppedFiles(dropped_files);
        }

        BeginDrawing();
        if (rom_loaded)
        {
            // Check for key presses
            for (uint8_t key = 0; key < 16; key++)
            {
                if (IsKeyDown(chip8_layout[key])) chip8.keypad |= (1U << key);
                else chip8.keypad &= ~(1U << key);
            }

            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) chip8.sound_timer--;

            main_cycle(&chip8);
        }
        else
        {
            ClearBackground((Color){192, 192, 192 , 255});
            DrawText("Drop a ROM file here", 0, 0, 2 * RENDER_SCALE, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

/* Loading the ROM */
int load_rom(const char* filename, struct CHIP8* chip8)
{
    // Try to open ROM
    printf("INFO: Loading ROM file \"%s\"...\n", filename);
    FILE* file;                      // Create a pointer to a FILE instance
    file = fopen(filename, "rb");    // Open the binary file
    if (file == NULL)
    {
        printf("ERROR: Could not load file \"%s\"!\n", filename);
        return -1;
    }

    // Find ROM size
    fseek(file, 0, SEEK_END);        // Move stream position indicator to end of file
    long file_size = ftell(file);     // Save current position (so we know the file size)
    fseek(file, 0, SEEK_SET);        // Move stream position indicator back to beginning of file

    if (file_size > 0 && file_size <= CHIP8_MAX_PROG_SIZE)
    {
        // Write ROM file into CHIP-8 memory
        fread(&chip8->memory[0x200], 1, file_size, file);
        printf("INFO: ROM file \"%s\" has been loaded into memory\n", filename);
        fclose(file);
        return 0;
    }
    else
    {
        printf("ERROR: ROM file \"%s\" is bigger than %d bytes!\n", filename, CHIP8_MAX_PROG_SIZE);
        fclose(file);
        return -1;
    }
}

/* Function definitions */
void main_cycle(struct CHIP8* chip8)
{
    // Run 10 CPU cycles and handle errors
    for (uint8_t count = 10; count > 0; count--)
    {
        enum CHIP8_STATUS status;
        switch (status = chip8_run_cycle(chip8))
        {
            case CHIP8_OK: break;
            case CHIP8_OUT_OF_BOUNDS:
                printf("WARNING: Program tried go out of bounds! Program counter set to program start (0x200)\n");
                break;
            case CHIP8_STACK_UNDERFLOW:
                printf("WARNING: Program tried to return from a non-existent subroutine!\n");
                printf("WARNING: Stack underflow: instruction 0x%04X at address 0x%04X\n", chip8->opcode, chip8->PC - 2);
                break;
            case CHIP8_STACK_OVERFLOW:
                printf("WARNING: Program tried to exceed hard-coded call stack storage limit of 16!\n");
                printf("WARNING: Stack overflow: instruction 0x%04X at address 0x%04X\n", chip8->opcode, chip8->PC - 2);
                break;
            case CHIP8_ILLEGAL_INSTRUCTION:
                printf("WARNING: Illegal instruction 0x%X at address 0x%X\n", chip8->opcode, chip8->PC - 2);
                break;
            case CHIP8_NO_KEY:
                printf("WARNING: Program tried to fetch a non-existent key! \n");
                printf("WARNING: Instruction 0x%X at address 0x%X\n", chip8->opcode, chip8->PC - 2);
                break;
            case CHIP8_NO_FONT_CHAR:
                printf("WARNING: Program tried to fetch a non-existent font character! \n");
                printf("WARNING: Instruction 0x%X at address 0x%X\n", chip8->opcode, chip8->PC - 2);
                break;
        }
    }

    ClearBackground(BLACK);
    for (uint8_t y = 0; y < 32; y++)
    {
        for (uint8_t x = 0; x < 64; x++)
        {
            if (chip8->framebuffer[y][x])
            {
                DrawRectangle(x * RENDER_SCALE, y * RENDER_SCALE, RENDER_SCALE, RENDER_SCALE, WHITE);
            }
        }
    }
}
