#include "instructions.h"
#include "interrupt_handler.h"

//Complements accumulator register
int cpl(CPU* cpu, Instruction* instruction) {
    //Gets current accumulator value
    uint8_t val = getRegisterValue8(cpu, REG_A);

    //Sets complement
    setRegisterValue(cpu, REG_A, ~val);

    //Sets flags
    setFlag(cpu, HALFCARRY);
    setFlag(cpu, SUB);

    //4 t-cycles
    return 4;
}

//Complements carry flag
int ccf(CPU* cpu, Instruction* instruction) {
    //Update flags
    updateFlag(cpu, CARRY, !flagIsSet(cpu, CARRY)); //Sets flag if carry is clear, clears if its set
    clearFlag(cpu, SUB);
    clearFlag(cpu, HALFCARRY);

    //4 t-cycles
    return 4;
}

//Sets carry flag
int scf(CPU* cpu, Instruction* instruction) {
    //Update flags
    setFlag(cpu, CARRY);
    clearFlag(cpu, SUB);
    clearFlag(cpu, HALFCARRY);

    //4 t-cycles
    return 4;
}

//Decimal adjust accumulator
int daa(CPU* cpu, Instruction* instruction) {
    /*
    * This instruction is WEIRD and is the only one to use the half-carry and sub flags
    * Basically it treats hex operations as if they are decimal, so instead of
    * 0x43 + 0x49 = 0x8C, you'd want 0x43 + 0x49 = 0x92.
    * So the gist is to check if there is a carry/half carry and wrap it around
    * at 9 instead of F, or from 0 to 9 instead of 0 to F in the case of subtraction.
    */
    //Get accumulator value
    uint8_t val = getRegisterValue8(cpu, REG_A);
    uint8_t correction = val;
    
    //Adjust the value
    if (!flagIsSet(cpu, SUB)) {
        //If addition, check half carry and lower nibble > 0x9
        if ((val & 0xF) > 0x9 || flagIsSet(cpu, HALFCARRY))
            correction += 0x06;

        //Check if upper nibble is over 9 now and adjust if so
        if (val > 0x99 || flagIsSet(cpu, CARRY)) {
            correction += 0x60;
            setFlag(cpu, CARRY);
        }
        else {
            clearFlag(cpu, CARRY);
        }
    }
    else {
        //If last operation was subtraction, adjust based on half-carry
        if (flagIsSet(cpu, HALFCARRY))
            correction -= 0x06;

        if (flagIsSet(cpu, CARRY))
            correction -= 0x60;
    }

    //Store value
    setRegisterValue(cpu, REG_A, correction);

    //Update flags
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(correction));

    //4 t-cycles
    return 4;
}

//Intended to stop the clock and put the system into a more efficient power saving mode
int stop(CPU* cpu, Instruction* instruction) {
    /*
    * This instruction is really weird and has a lot of strange edge cases.
    * On CGB, this instruction enables the double-speed mode. On GB, 
    * it SOMETIEMS does what it's intended to do and sometimes just... doesn't.
    * No commercial GB games use this instruction, so for the time being I will implement it
    * as essentially a 2-byte "NOP", except in CBG mode where I will implement the double speed mode.
    * This will make the emulator slightly less accurate, but unless you're doing weird glitch stuff, 
    * it shouldn't matter at all, which is sufficient for now.
    *
    * More info can be found here: https://gbdev.io/pandocs/Reducing_Power_Consumption.html
    */    

    //TODO: Implement CGB double-speed mode
    cpu->registers.pc++; //Instruction is 2-bytes. It simply skips one byte.
    
    //4 t-cycles
    return 4;
}

//Halts the CPU but does not stop the clock
int halt(CPU* cpu, Instruction* instruction) {
    /*
    * This is also a very strange instruction.
    * Depending on the different interrupt flags, it either halts, does not halt,
    * fetches the next instruction twice, or just continues like normal.
    */

    //Check if interrupt is pending
    uint8_t interrupt_pending = anyInterruptPending(cpu);
    uint8_t ime = cpu->state.IME;

    //If IME is set, HALT is normal
    if (ime)
        setFlag(cpu, IS_HALTED);
    else {
        //If IME is 0 and there are no interrupts, HALT is normal.
        //This means that before next instruction, HALT gets cleared
        if (!interrupt_pending)
            setFlag(cpu, IS_HALTED);        
        //BUT!! If IME is 0 and there ARE interrupts, HALT bug is triggered
        else
            setFlag(cpu, HALT_BUG);
    }

    //4 t-cycles
    return 4;
}

//No-operation. Simply acts as a dummy instruction
int nop(CPU* cpu, Instruction* instruction) {
    //4 t-cycles
    return 4;
}

//Disables interrupts immediately by clearing IME
int di(CPU* cpu, Instruction* instruction) {
    //Clears IME flag
    clearFlag(cpu, IME);
    //Since EI has a 1 cycle delay to setting the IME, a second flag is used to track that.
    //If an EI and DI get called together, IME is never set, so this accoutns for that weird edge-case
    clearFlag(cpu, ENABLE_IME);

    //4 t-cycles
    return 4;
}

//Enables interrupts with a 1-cycle delay by setting IME
int ei(CPU* cpu, Instruction* instruction) {
    //Set enable IME flag, which accounts for the 1-cycle delay
    setFlag(cpu, ENABLE_IME);

    //4 t-cycles
    return 4;
}