# CHIP-8 Interpreter
## Usage
Run the program directly to be greeted with a window. From there, drop the ROM file onto the program.

Or you can run the program from command line and supply the ROM file path as an argument like this:
```
./chip8 pong.ch8
```
## Building
### Linux
#### Required Dependencies
- raylib
#### Building
Simply use the compiler of your choice and compile `main.c` and `chip8.c` with `-lraylib` flag
```
clang main.c chip8.c -o chip8 -lraylib
```
### Windows
You can install [Windows Subsystem for Linux](https://learn.microsoft.com/en-us/windows/wsl/install) and follow Linux build instructions.
## Resources Used
- https://github.com/mattmikolay/chip-8/wiki/CHIP-8-Instruction-Set
- https://chip8kb.gulrak.net/reference/variants/classic-chip-8
- http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
## AI Disclosure
No code in this project is AI generated, but I have used AI tools strictly as a learning aid to help me understand the underlying concepts as a beginner.
