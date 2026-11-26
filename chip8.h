#include <stdint.h>

/* Global defines and enums */
#define CHIP8_PROG_START    0x200
#define CHIP8_MAX_PROG_SIZE (4096 - 0x200)

enum CHIP8_STATUS
{
    CHIP8_OK,
    CHIP8_OUT_OF_BOUNDS,
    CHIP8_STACK_UNDERFLOW,
    CHIP8_STACK_OVERFLOW,
    CHIP8_ILLEGAL_INSTRUCTION,
    CHIP8_NO_KEY,
    CHIP8_NO_FONT_CHAR
};


/* CHIP-8 hardware */
struct CHIP8
{
    // CPU
    uint16_t I;                         // Index register
    uint16_t PC;                        // Program counter
    uint8_t  SP;                        // Stack pointer
    uint8_t  V[16];                     // General purpose "variable" registers (V0-VF)

    // System memory
    uint8_t memory[4096];               // 4096 bytes of RAM
    uint16_t opcode;

    // Call stack storage being an array with 16 entries is a
    // conventional choice, not from the original implementation.
    uint16_t stack[16];                 // 16 16-bit addresses for stack

    // Display
    uint8_t framebuffer[32][64];     // 64 x 32 monochrome pixels

    // Timers
    uint8_t  delay_timer;               // Delay timer
    uint8_t  sound_timer;               // Sound timer

    // Keypad
    uint16_t keypad;                    // Keypad (16 keys)
};

/* Function declarations */
void chip8_init(struct CHIP8* chip8, uint32_t srand_seed);
enum CHIP8_STATUS chip8_run_cycle(struct CHIP8* chip8);
