/*#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "cpu.h"

//Tests setting register values
void testSetRegisterValue(CPU* cpu) {
    printf("Testing setRegister...\n");
    
    assert(setRegisterValue(cpu, REG_B, 163) == 0); // B = 163
    assert(setRegisterValue(cpu, REG_C, 13) == 0);  // C = 13
    assert(setRegisterValue(cpu, REG_sp, 12) == 0); // sp = 12
    assert(setRegisterValue(cpu, REG_D, -1) == 0);  // D = 255
    assert(setRegisterValue(cpu, 53, 12) == 1); // Error - 53 isnt a register

    printf("Tests Passed!\n\n");
}

//Tests getting 16 bit register values
void testGet16BitRegisterValue(CPU* cpu) {
    printf("Testing getRegisterValue16...\n");

    assert(getRegisterValue16(cpu, REG_sp) == 12); //Value stored by set register test
    assert(getRegisterValue16(cpu, REG_BC) == ((163 << 8) + 13));
    assert(getRegisterValue16(cpu, REG_B) == 0); //Invalid register

    printf("Tests Passed!\n\n");
}

//Tests getting 8 bit register values
void testGet8BitRegisterValue(CPU* cpu) {
    printf("Testing getRegisterValue8...\n");

    assert(getRegisterValue8(cpu, REG_B) == 163); //Value stored by set register test
    assert(getRegisterValue8(cpu, REG_D) == 255);
    assert(getRegisterValue8(cpu, REG_C) == 13);
    assert(getRegisterValue8(cpu, REG_sp) == 0); //Invalid register

    printf("Tests passed!\n\n");
}

//Tests setting flag
void testSetFlag(CPU* cpu) {
    printf("Testing setFlag...\n");

    assert(setFlag(cpu, HALFCARRY) == 0);
    assert(setFlag(cpu, CARRY) == 0);
    assert(setFlag(cpu, 55) == 1); //Invalid flag

    printf("Tests passed!\n\n");
}

//Tests clearing flag
void testClearFlag(CPU* cpu) {
    printf("Testing clearFlag...\n");

    assert(clearFlag(cpu, SUB) == 0);
    assert(clearFlag(cpu, ZERO) == 0);
    assert(clearFlag(cpu, 56) == 1); //Invalid flag

    printf("Tests passed!\n\n");
}

//Tests checking if flag is clear or set
void testFlagIsSet(CPU* cpu) {
    printf("Testing flagIsSet...\n");

    assert(flagIsSet(cpu, CARRY) == 1); //Carry flag is set
    assert(flagIsSet(cpu, HALFCARRY) == 1); //Half carry flag is set
    assert(flagIsSet(cpu, SUB) == 0); //Sub flag is not set
    assert(flagIsSet(cpu, ZERO) == 0); //Zero flag is not set

    printf("Tests passed!\n\n");
}

int main() {
    CPU* cpu = cpu_init();

    testSetRegisterValue(cpu);
    testGet16BitRegisterValue(cpu);
    testGet8BitRegisterValue(cpu);
    testSetFlag(cpu);
    testClearFlag(cpu);
    testFlagIsSet(cpu);

    cpu_destroy(cpu);

    printf("All tests passed.\n");

    return 0;
}*/

int main() {
    return 0;
}