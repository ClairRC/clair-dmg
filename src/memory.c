#include <stdlib.h>
#include "memory.h"
#include "logging.h"

//TODO: Fix this function, particularly to be able to read memory before knowing how many memory banks there are for startup.
Memory* memory_init(size_t rom_banks, size_t exram_banks, size_t wram_banks) {
    if (rom_banks < 1 || wram_banks < 1) {
        printError("Error: There must be at least 1 ROM bank and 1 WRAM bank.");
        return NULL;
    }

    Memory* mem = (Memory*)malloc(sizeof(Memory));

    //Check for error with initialization
    if (mem == NULL) {
        printError("Error: Unable to initialize memory.");
        return NULL;
    }

    //Initialize dynamic values
    mem->num_rom_banks = rom_banks;
    mem->num_exram_banks = exram_banks;
    mem->num_wram_banks = wram_banks;

    mem->current_rom_bank = 0;
    mem->current_exram_bank = 0;
    mem->current_wram_bank = 0;

    //Since rom and wram banks 0 are already included, if there is only 1, then
    //we don't need these pointers to include anything. 
    //Similarly, if there is 0 exram banks, that array doesnt exist, so we keep it NULL.
    
    //TODO: Add error for failing to initialize any of these.
    if (rom_banks < 2)
        mem->rom_x = NULL;
    else
        //Subtract 1 from rom_banks because bank 0 is already included
        mem->rom_x = (uint8_t*)malloc((rom_banks-1) * 0x4000); //rom area is 0x4000 bytes

    if (exram_banks < 1)
        mem->exram_x = NULL;
    else
        mem->exram_x = (uint8_t*)malloc(exram_banks * 0x2000); //exram area is 0x2000 bytes

    if (wram_banks < 2)
        mem->wram_x = NULL;
    else
        //Subtract 1 from wram_banks because bank 0 is already included
        mem->wram_x = (uint8_t*)malloc((wram_banks-1) * 0x1000); //wram area is 0x1000 bytes
}

void memory_destroy(Memory* mem) {
    //If pointer is null, we're done
    if (mem == NULL)
        return;

    if (mem->rom_x != NULL)
        free(mem->rom_x);

    if (mem->exram_x != NULL)
        free(mem->exram_x);

    if (mem->wram_x != NULL)
        free(mem->wram_x);

    free(mem);
}

//TODO: Implement read/write functions!!!
//Placeholders for now
uint8_t mem_read(Memory* mem, uint16_t address, Accessor accessor) {
    return 0;
}

int mem_write(Memory* mem, uint16_t address, uint8_t value, Accessor access) {
    return 0;
}