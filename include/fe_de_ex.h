#ifndef FE_DE_EX_H
#define FE_DE_EX_H
#include "system.h" 
#include "game_display.h"
#include "instructions.h"

//This is the fetch, decode, execute loop file that will handle the main instruction loop
int fe_de_ex(EmulatorSystem* system);

void handle_interrupt(EmulatorSystem* system); //Checks for and handles interrupts
uint8_t fetch_instruction_opcode(EmulatorSystem* system);
Instruction* decode_instruction(EmulatorSystem* system, uint8_t opcode);
void execute_instruction(EmulatorSystem* system, Instruction* instr);

#endif