#include <stdint.h>
#include <stdlib.h>
#include "hardware_def.h"

//Instruction struct

/*
* The way I am going to do this is by giving each instruction 2 operands
* and 2 operand type. This will cut down the number of functions i need to implement,
* and hopefully just make things much less confusing. 
*/
typedef struct Instruction Instruction; //Forward declaration
struct Instruction {
    int (*funct_ptr)(Instruction*); //Pointer to function
    
    //Operands. Always 16 bit int for consistency
    uint16_t dest_operand;
    uint16_t src_operand;

    //Operand types to decode what to do
    OperandType dest_operand_type;    
    OperandType src_operand_type;

    size_t num_bytes; //Size of instruction in bytes
    size_t num_t_cycles; //Number of cycles the instruction takes for timing
};

//Lookup tables for CPU instructions.
