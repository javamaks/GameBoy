#include "emulator.h"
#include "cpu.h"
#include "display.h"

int main(int argc, char *args[])
{
	Emulator emulator;

	//string name = "cpu/cpu_instrs";
	//string name = "instr_timing";

	//emulator.memory.load_rom("tests/" + name + ".gb");
	//emulator.memory.load_rom("roms/Dr. Mario.gb");
	//emulator.memory.load_rom("roms/kirby.gb");
	emulator.memory.load_rom("roms/Donkey Kong.gb");
	//emulator.memory.load_rom("roms/Mortal Kombat.gb");
	//emulator.memory.load_rom("roms/Legend of Zelda.gb");
	//emulator.memory.load_rom("roms/Tetris.gb");
	//emulator.memory.load_rom("roms/Spider-Man.gb");
	//emulator.memory.load_rom("roms/main.gb");

	emulator.run();

	return 0;
}
