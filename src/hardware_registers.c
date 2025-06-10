#include "hardware_registers.h" 

//Macro to help fill this table easier
#define HW_REG(reg, r, w, cgb) hw_registers[reg] =\
    (HardwareRegister){.read_mask = (r), .write_mask = (w), .cgb_only = (cgb)}\

HardwareRegister hw_registers[256];

//Function to populate table with hardware register rules.
//Lookup table stores the lower byte, as the upper byte is always 0xFF
void init_hw_registers() {
    /*
    * I debated how I was going to do this, as most of these registers are
    * normal R/W registers on all versions of the console, but a handful are special.
    * I thought about including pointers here instead, allocating to the heap,
    * using a hash map, and some other stuff. But ultimately I decided that this
    * is a table of 256 structs that are 3 bytes and this function gets called once.
    * The overhead for some fancy solution is just not worth saving >1KB of memory
    */

    //Most registers are R/W and work on CGB and DMG
    //This just fills the initial table so I don't have to write so many.
    //Fun fact, this is the first loop I've written for this.
    //This is a very interestiong project...
    for (int i = 0; i < 256; ++i) {
        HW_REG(i, 0xFF, 0xFF, 0);
    }

    //WRITE ONLY BITS: READ MASK REPLACE WITH 0 (not read!!!)
    //READ ONLY BITS: WRITE MASK REPLACE WITH 0 (not writable!!!)

    //Define special ones
    //TODO: Figure out 0xFF02 shenanigans
    HW_REG(0x00, 0xFF, 0xF0, 0); //Lower nibble is read-only
    HW_REG(0x11, 0xC0, 0xFF, 0); //0-5 is write only
    HW_REG(0x13, 0x00, 0xFF, 0); //Write only
    HW_REG(0x14, 0x78, 0xFF, 0); //7th bit and bits 0-2 are write only
    hw_registers[0x16] = hw_registers[0x11]; //2nd sound channel has same behavior as first
    hw_registers[0x17] = hw_registers[0x12];   
    hw_registers[0x18] = hw_registers[0x13];
    hw_registers[0x19] = hw_registers[0x14];
    HW_REG(0x1B, 0x00, 0xFF, 0); //W/O
    HW_REG(0x1D, 0x00, 0xFF, 0); //W/O
    HW_REG(0x1E, 0x78, 0xFF, 0); //Bit 7 and 0-2 W/O
    HW_REG(0x20, 0x00, 0xFF, 0); //W/O
    HW_REG(0x23, 0x7F, 0xFF, 0); //Bit 7 W/O
    HW_REG(0x26, 0xFF, 0x80, 0); //Bits 0-6 R/O
    HW_REG(0x41, 0xFF, 0xFC, 0); //Bit 0-1 R/O
    HW_REG(0x44, 0xFF, 0x00, 0); //R/O
    HW_REG(0x4D, 0xFF, 0xF7, 1); //Bit 7 R/O, CGB only
    HW_REG(0x51, 0x00, 0xFF, 1); //HDMA1-5 write only, CGB only
    HW_REG(0x52, 0x00, 0xFF, 1);
    HW_REG(0x53, 0x00, 0xFF, 1);
    HW_REG(0x54, 0x00, 0xFF, 1);
    HW_REG(0x56, 0xFF, 0xFD, 1); //Bit 1 R/O, CGB only
    HW_REG(0x76, 0xFF, 0x00, 1); //R/O, CGB only
    HW_REG(0x77, 0xFF, 0x00, 1); //R/O, CGB only
}