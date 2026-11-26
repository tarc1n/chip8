#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "chip8.h"

/* Global constants & variables */
// Font used in COSMAC VIP, 16 characters, 5 bytes each
const uint8_t font[16 * 5] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x60, 0x20, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0xA0, 0xA0, 0xF0, 0x20, 0x20,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x10, 0x10, 0x10,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xF0, 0x50, 0x70, 0x50, 0xF0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xF0, 0x50, 0x50, 0x50, 0xF0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F

};

/* CHIP-8 initialization */
void chip8_init(struct CHIP8* chip8, uint32_t srand_seed)
{
    // Zero out everything
    memset(chip8, 0, sizeof(*chip8));

    // Copy font to memory starting at address 0x000
    memcpy(&chip8->memory[0x000], font, sizeof(font));

    // The reason we have to start reading the ROM after the first 512 bytes is that
    // historically CHIP-8 interpreter itself and other data lived in this region (0x000 - 0x1FF).
    // All programs assume this and start after first 512 bytes, at 0x200.

    // Set program counter to 0x200 (program start)
    chip8->PC = 0x200;

    // Seed RNG for CXNN instruction
    srand(srand_seed);
}

/* CHIP-8 cycle emulation */
// Fetch the instruction from memory in CHIP8 instance.
// Then decode the instruction and finally execute.
enum CHIP8_STATUS chip8_run_cycle(struct CHIP8* chip8)
{
    // Out-of-bounds protection
    if (chip8->PC >= 4095)
    {
        chip8->PC = 0x200;
        return CHIP8_OUT_OF_BOUNDS;
    }

    /*=== FETCH ===*/
    // CHIP-8 instructions are 16-bits wide. That means we have to combine
    // every 2 bytes in the memory to make a single instruction (opcode).
    chip8->opcode = (chip8->memory[chip8->PC] << 8 | chip8->memory[chip8->PC + 1]);

    // That also means incrementing by 2 per fetched instruction.
    chip8->PC += 2;

    /* DECODE and EXECUTE */
    // Resource below was used here for instruction set implementation.
    // https://github.com/mattmikolay/chip-8/wiki/CHIP-8-Instruction-Set

    // CHIP-8 instruction format is as follows:
    // 0xCXYN

    // C, the most significant nibble, determines the primary opcode class.
    // N, the remaining nibble(s), supply operands or further opcode distinction.
    // X and Y are placeholders for general purpose variable registers (V0 - VF).
    // NOTE: Operands change based on the opcode class (e.g., 0x1NNN, 0x3XNN).

    // We can store the operands like this for convenience:
    uint16_t NNN = chip8->opcode & 0x0FFF;        // Lower 12 bits / 3 nibbles
    uint8_t  NN  = chip8->opcode & 0x00FF;        // Lower byte / 2 nibbles
    uint8_t  N   = chip8->opcode & 0x000F;        // Least significant nibble
    uint8_t  X   = (chip8->opcode & 0x0F00) >> 8; // Least significant nibble of upper byte
    uint8_t  Y   = (chip8->opcode & 0x00F0) >> 4; // Most significant nibble of lower byte

    // First, we mask out the most significant nibble to determine the opcode,
    // then we check if this opcode has additional operands.
    switch (chip8->opcode & 0xF000)
    {
        case 0x0000:
            // 0NNN has 3 distinct opcodes
            switch (NNN)
            {
                case 0x0E0:
                    // 0x00E0: Clear the screen
                    memset(chip8->framebuffer, 0, sizeof(chip8->framebuffer));
                    break;

                case 0x0EE:
                    // 0x00EE: Return from a subroutine
                    if (chip8->SP > 0)
                    {
                        chip8->SP--;
                        chip8->PC = chip8->stack[chip8->SP];
                    }
                    else return CHIP8_STACK_UNDERFLOW;
                    break;

                default:
                    // 0NNN: Execute machine language subroutine at address NNN

                    // Upon a bit of investigation, this instruction was apparently used to execute
                    // native machine code on the RCA 1802 processor that the original CHIP-8
                    // ran on. Since we are not emulating that, we can simply Do Nothing(tm).
                    break;
            }
            break;

        case 0x1000:
            // 1NNN: Jump to address NNN
            chip8->PC = NNN;
            break;

        case 0x2000:
            // 2NNN: Execute subroutine starting at address NNN
            if (chip8->SP < 16)
            {
                chip8->stack[chip8->SP] = chip8->PC;
                chip8->SP++;
                chip8->PC = NNN;
            }
            else return CHIP8_STACK_OVERFLOW;
            break;

        case 0x3000:
            // 3XNN: Skip the following instruction
            //if the value of register VX equals NN
            if (chip8->V[X] == NN) chip8->PC += 2;
            break;

        case 0x4000:
            // 4XNN: Skip the following instruction
            // if the value of register VX is not equal to NN
            if (chip8->V[X] != NN) chip8->PC += 2;
            break;

        case 0x5000:
            // 5XY0: Skip the following instruction
            // if the value of register VX is equal to the value of register VY
            if (N == 0x0)
            {
                if (chip8->V[X] == chip8->V[Y]) chip8->PC += 2;
            }
            else
            {
                break;
            }
            break;

        case 0x6000:
            // 6XNN: Store number NN in register VX
            chip8->V[X] = NN;
            break;

        case 0x7000:
            // 7XNN: Add the value NN to register VX
            chip8->V[X] += NN;
            break;

        case 0x8000:
            // 8NNN has 9 distinct opcodes
            switch (N)
            {
                case 0x0:
                    // 8XY0: Store the value of register VY in register VX
                    chip8->V[X] = chip8->V[Y];
                    break;

                case 0x1:
                    // 8XY1: Set VX to VX OR VY
                    chip8->V[X] = chip8->V[X] | chip8->V[Y];
                    break;

                case 0x2:
                    // 8XY2: Set VX to VX AND VY
                    chip8->V[X] = chip8->V[X] & chip8->V[Y];
                    break;

                case 0x3:
                    // 8XY3: Set VX to VX XOR VY
                    chip8->V[X] = chip8->V[X] ^ chip8->V[Y];
                    break;

                case 0x4:
                {
                    // 8XY4: Add the value of register VY to register VX
                    // Set VF to 01 if a carry occurs,
                    // Set VF to 00 if a carry does not occur
                    uint8_t carry = (chip8->V[X] + chip8->V[Y] > 255) ? 1 : 0;
                    chip8->V[X] += chip8->V[Y];
                    chip8->V[15] = carry;
                    break;
                }

                case 0x5:
                {
                    // 8XY5: Subtract the value of register VY from register VX
                    // Set VF to 00 if a borrow occurs
                    // Set VF to 01 if a borrow does not occur
                    uint8_t no_borrow = (chip8->V[X] >= chip8->V[Y]) ? 1 : 0;
                    chip8->V[X] -= chip8->V[Y];
                    chip8->V[15] = no_borrow;
                    break;
                }

                case 0x6:
                {
                    // 8XY6: Store the value of register VY shifted right one bit in register VX
                    // Set register VF to the least significant bit prior to the shift
                    // VY is unchanged
                    uint8_t nonshifted = (chip8->V[Y] & 0x01);
                    chip8->V[X] = (chip8->V[Y] >> 1);
                    chip8->V[15] = nonshifted;

                    break;
                }
                case 0x7:
                {
                    // 8XY7: Set register VX to the value of VY minus VX
                    // Set VF to 00 if a borrow occurs
                    // Set VF to 01 if a borrow does not occur
                    uint8_t no_borrow = (chip8->V[Y] >= chip8->V[X]) ? 1 : 0;
                    chip8->V[X] = chip8->V[Y] - chip8->V[X];
                    chip8->V[15] = no_borrow;
                    break;
                }

                case 0xE:
                {
                    // 8XYE: Store the value of register VY shifted left one bit in register VX
                    // Set register VF to the most significant bit prior to the shift
                    // VY is unchanged
                    uint8_t nonshifted = (chip8->V[Y] & 0x80) >> 7;
                    chip8->V[X] = (chip8->V[Y] << 1);
                    chip8->V[15] = nonshifted;
                    break;
                }

                default:
                    return CHIP8_ILLEGAL_INSTRUCTION;
                    break;
            }
            break;

        case 0x9000:
            // 9XY0: Skip the following instruction
            // if the value of register VX is not equal to the value of register VY
            if (N == 0x0)
            {
                if (chip8->V[X] != chip8->V[Y]) chip8->PC += 2;
            }
            else return CHIP8_ILLEGAL_INSTRUCTION;
            break;

        case 0xA000:
            // ANNN: Store memory address NNN in register I
            chip8->I = NNN;
            break;

        case 0xB000:
            // BNNN: Jump to address NNN + V0
            chip8->PC = NNN + chip8->V[0];
            break;

        case 0xC000:
        {
            // CXNN: Set VX to a random number with a mask of NN
            uint8_t random_number = rand();
            chip8->V[X] = random_number & NN;
            break;
        }
        case 0xD000:
        {
            // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
            // Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
            uint8_t pos_x = chip8->V[X] % 64;
            uint8_t pos_y = chip8->V[Y] % 32;
            chip8->V[15] = 0;
            for (uint8_t row = 0; row < N; row++)
            {
                if((pos_y + row > 31)) break;
                uint8_t sprite_data = chip8->memory[chip8->I + row];
                for (uint8_t column = 0; column < 8; column++)
                {
                    if (sprite_data & (0x80 >> column))
                    {
                        if((pos_x + column > 63)) break;
                        if (chip8->framebuffer[pos_y + row][pos_x + column]) chip8->V[15] = 1;
                        chip8->framebuffer[pos_y + row][pos_x + column] ^= 1;
                    }
                }
            }

            break;
        }
        case 0xE000:
            // ENNN has 2 distinct opcodes
            switch (NN)
            {
                case 0x9E:
                {
                    // EX9E: Skip the following instruction
                    // if the key corresponding to the hex value currently stored in register VX is pressed
                    if (chip8->V[X] > 0x0F) return CHIP8_NO_KEY;
                    uint16_t mask = 1U << (chip8->V[X] & 0x0F);
                    if (chip8->keypad & mask) chip8->PC += 2;
                    break;
                }
                case 0xA1:
                {
                    // EXA1: Skip the following instruction
                    // if the key corresponding to the hex value currently stored in register VX is not pressed
                    if (chip8->V[X] > 0x0F) return CHIP8_NO_KEY;
                    uint16_t mask = 1U << (chip8->V[X] & 0x0F);
                    if (!(chip8->keypad & mask)) chip8->PC += 2;
                    break;
                }
                default:
                    return CHIP8_ILLEGAL_INSTRUCTION;
                    break;
            }
            break;

        case 0xF000:
            // FNNN has 9 distinct opcodes
            switch (NN)
            {
                case 0x07:
                    // FX07: Store the current value of the delay timer in register VX
                    chip8->V[X] = chip8->delay_timer;
                    break;

                case 0x0A:
                {
                    // FX0A: Wait for a keypress and store the result in register VX
                    uint8_t key_press = 0;
                    for (uint8_t i = 0; i < 16; i++)
                    {
                        if (chip8->keypad & (1U << i))
                        {
                            chip8->V[X] = i;
                            key_press = 1;
                            break;
                        }
                    }
                    if (!key_press) { chip8->PC -= 2; }
                    break;
                }
                case 0x15:
                    // FX15: Set the delay timer to the value of register VX
                    chip8->delay_timer = chip8->V[X];
                    break;

                case 0x18:
                    // FX18: Set the sound timer to the value of register VX
                    chip8->sound_timer = chip8->V[X];
                    break;

                case 0x1E:
                    // FX1E: Add the value stored in register VX to register I
                    chip8->I += chip8->V[X];
                    break;

                case 0x29:
                    // FX29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    if (chip8->V[X] > 0x0F) return CHIP8_NO_FONT_CHAR;
                    chip8->I = (chip8->V[X] & 0x0F) * 5;
                    break;

                case 0x33:
                {
                    // FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    uint8_t hundreds = chip8->V[X] / 100;
                    uint8_t tens = (chip8->V[X] / 10) - (hundreds * 10);
                    uint8_t ones = chip8->V[X] - ((hundreds * 100) + (tens * 10));

                    // Remember, big endian
                    chip8->memory[chip8->I]     = hundreds;
                    chip8->memory[chip8->I + 1] = tens;
                    chip8->memory[chip8->I + 2] = ones;
                    break;
                }
                case 0x55:
                {
                    // FX55: Store the values of registers V0 to VX inclusive in memory starting at address I
                    // I is set to I + X + 1 after operation
                    for (uint8_t i = 0; i < X + 1; i++)
                    {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    chip8->I = chip8->I + X + 1;
                    break;
                }
                case 0x65:
                {
                    // FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I
                    // I is set to I + X + 1 after operation
                    for (uint8_t i = 0; i < X + 1; i++)
                    {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }
                    chip8->I = chip8->I + X + 1;
                    break;
                }
                default:
                {
                    return CHIP8_ILLEGAL_INSTRUCTION;
                    break;
                }
            }
            break;
    }

    return CHIP8_OK;
}
