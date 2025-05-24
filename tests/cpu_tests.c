#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "cpu.h"

void testSetRegisterValue(CPU* cpu) {
    printf("Testing setRegister...\n");
    
    assert(setRegisterValue(cpu, REG_B, 163) == 0); // B = 163
    assert(setRegisterValue(cpu, REG_C, 13) == 0);  // C = 13
    assert(setRegisterValue(cpu, REG_sp, 12) == 0); // sp = 12
    assert(setRegisterValue(cpu, REG_D, -1) == 0);  // D = 255
    assert(setRegisterValue(cpu, 53, 12) == 1); // Error - 53 isnt a register

    printf("Tests Passed!\n\n");
}

int main() {
    CPU* cpu = cpu_init();

    testSetRegisterValue(cpu);

    cpu_destroy(cpu);
    return 0;
}