#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include "logging.h"

//Initializes CPU and its components.
//There should only ever be one of these
CPU* cpu_init() {
    RegisterFile rf = {0};
    CPU* cpu = (CPU*)malloc(sizeof(CPU));

    //If memory isn't allocated, return an error
    if (cpu == NULL) {
        printError("Error: Unable to initialize CPU");
        return NULL;
    }

    cpu->registers = rf;

    return cpu;
}

//Function to destroy CPU to prevent memory leaks.
void cpu_destroy(CPU* cpu) {
    if (cpu != NULL)
        free(cpu);
}

//Returns the value stored at the register.
//This will only return values from 16 bit registers!
//Returns 0 if register is invalid.
uint16_t getRegisterValue16(CPU* cpu, Registers reg) {
    //Returns 0 if CPU is null
    if (cpu == NULL) {
        printError("Error: CPU is a null pointer.");
        return 0; 
    }

    uint16_t val = 0;

    switch (reg) {
        //I don't think A and F are ever treated as the same like this,
        //but keeping this here for now
        case REG_AF:
            val = ((uint16_t)cpu->registers.A) << 8;
            val |= cpu->registers.F;
            break;
        case REG_BC:
            val = ((uint16_t)cpu->registers.B) << 8;
            val |= cpu->registers.C;
            break;
        case REG_DE:
            val = ((uint16_t)cpu->registers.D) << 8;
            val |= cpu->registers.E;
            break;
        case REG_HL:
            val = ((uint16_t)cpu->registers.H) << 8;
            val |= cpu->registers.L;
            break;
        case REG_sp:
            val = cpu->registers.sp;
            break;
        case REG_pc:
            val = cpu->registers.pc;
            break;
        default:
            printError("Error: Invalid register! Did you mean to return an 8 bit register?");
            break;
    }

    //Returns 0 for invalid register
    return val;
}

//Returns the value stored at the register.
//This only will return values from 8 bit registers!
//Returns 0 if register is invalid.
uint8_t getRegisterValue8(CPU* cpu, Registers reg) {
    //Returns 0 if CPU is null
    if (cpu == NULL) {
        printError("Error: CPU is a null pointer.");
        return 0; 
    }

    uint8_t val = 0;

    //Just like below, this switch statement is icky gross,
    //but it SHOULD be ideal to handle all of the annoying details
    switch (reg) {
        case REG_A:
            val = cpu->registers.A;
            break;
        case REG_B:
            val = cpu->registers.B;
            break;
        case REG_C:
            val = cpu->registers.C;
            break;
        case REG_D:
            val = cpu->registers.D;
            break;
        case REG_E:
            val = cpu->registers.E;
            break;
        case REG_F:
            val = cpu->registers.F;
            break;
        case REG_H:
            val = cpu->registers.H;
            break;
        case REG_L:
            val = cpu->registers.L;
            break;
        default:
            printError("Error: Invalid register. Did you mean to return a 16 bit register?");
            break;
    }

    //Returns 0 if invalid register.
    return val;
}

//Sets register value
//Handles setting register values of 8 bit or 16 bit to make programming
//instruction set a little bit easier.
int setRegisterValue(CPU* cpu, Registers reg, uint16_t value) {
    //Returns 1 if CPU is null
    if (cpu == NULL) {
        printError("Error: CPU is a null pointer.");
        return 1; 
    }

    //Lots of switch statements here, but ideally the abstraction
    //will make coding the instructions easier, because there is a LOT of them.
    //The goal is to keep all the tedious stuff here so that actually writing
    //and executing the instructions is easy, since that's ultimately the goal

    switch (reg) {
        case REG_A:
            cpu->registers.A = (uint8_t)value;
            break;
        case REG_B:
            cpu->registers.B = (uint8_t)value;
            break;
        case REG_C:
            cpu->registers.C = (uint8_t)value;
            break;
        case REG_D:
            cpu->registers.D = (uint8_t)value;
            break;
        case REG_E:
            cpu->registers.E = (uint8_t)value;
            break;
        case REG_F:
            cpu->registers.F = (uint8_t)value;
            break;
        case REG_H:
            cpu->registers.H = (uint8_t)value;
            break;
        case REG_L:
            cpu->registers.L = (uint8_t)value;
            break;
        case REG_sp:
            cpu->registers.sp = (uint16_t)value;
            break;
        case REG_pc:
            cpu->registers.pc = (uint16_t)value;
            break;

        //Special registers
        //Some registers can be merged together to store 16 bit values
        //I don't THINK AF is ever treated this way, but I'm keeping this just in case
        case REG_AF:
            cpu->registers.A = (uint8_t)(value>>8);
            cpu->registers.F = (uint8_t)value;
            break;
        case REG_BC:
            cpu->registers.B = (uint8_t)(value>>8);
            cpu->registers.C = (uint8_t)value;
            break;
        case REG_DE:
            cpu->registers.D = (uint8_t)(value>>8);
            cpu->registers.E = (uint8_t)value;
            break;
        case REG_HL:
            cpu->registers.H = (uint8_t)(value>>8);
            cpu->registers.L = (uint8_t)value;
            break;
        default:
            //Return 1 if register value is wrong
            printError("Error: Invalid register value!");
            return 1;
        }

    //Return 0 for success
    return 0;
}

//Sets flag values stored in F register
int setFlag(CPU* cpu, Flags flag) {
    //Returns 1 if CPU is null
    if (cpu == NULL) {
        printError("Error: CPU is a null pointer.");
        return 1; 
    }

    switch (flag) {
        case ZERO:
            cpu->registers.F |= FLAG_ZERO;
            break;
        case SUB:
            cpu->registers.F |= FLAG_SUB;
            break;
        case HALFCARRY:
            cpu->registers.F |= FLAG_HALFCARRY;
            break;
        case CARRY:
            cpu->registers.F |= FLAG_CARRY;
            break;
        default:
            printError("Error: Invalid flag!");
            return 1; //Return 1 for failure
    }

    return 0;
}

//Clears flag stored in F register
int clearFlag(CPU* cpu, Flags flag) {
    //Returns 1 if CPU is null
    if (cpu == NULL) {
        printError("Error: CPU is a null pointer.");
        return 1; 
    }

    switch (flag) {
        case ZERO:
            cpu->registers.F &= ~FLAG_ZERO;
            break;
        case SUB:
            cpu->registers.F &= ~FLAG_SUB;
            break;
        case HALFCARRY:
            cpu->registers.F &= ~FLAG_HALFCARRY;
            break;
        case CARRY:
            cpu->registers.F &= ~FLAG_CARRY;
            break;
        default:
            printError("Error: Invalid flag!");
            return 1; //Failure
    }

    return 0;
}

//Updates flag based on boolean input
int updateFlag(CPU* cpu, Flags flag, int status) {
    //Expression is FALSE -- Clear flag
    if (status == 0)
        clearFlag(cpu, flag);
    //Expression is TRUE -- Set flag
    else
        setFlag(cpu, flag);

    return 0;
}

//Returns whether a flag is set or not. 1 for set, 0 for clear
int flagIsSet(CPU* cpu, Flags flag) {
    //Gets flag value itself
    uint8_t flagVal = getRegisterValue8(cpu,REG_F);

    //Whether flag is set or not
    // 0 - Clear; 1 - Set
    int flagState = 1;

    //Determins if flag is set based on flag input. Again, more switch statements.
    //I'm kinda relying on them a lot, but again I'm hoping it makes usage easier later.
    switch(flag) {
        //Bitwise & to determine if value is 0 and flip flag state
        case ZERO:
            if ((flagVal & FLAG_ZERO) == 0)
                flagState = 0;
            break;
        case SUB:
            if ((flagVal & FLAG_SUB) == 0)
                flagState = 0;
            break;
        case HALFCARRY:
            if ((flagVal & FLAG_HALFCARRY) == 0)
                flagState = 0;
            break;
        case CARRY:
            if ((flagVal & FLAG_CARRY) == 0)
                flagState = 0;
            break;
        default:
            flagState = 0;
            break;
    }

    return flagState;
}
