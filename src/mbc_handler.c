#include "mbc_handler.h"
#include "logging.h" 
#include <stdlib.h>

//Helper functions for MBC information
MBC_Type getMBCType(uint8_t type);
uint8_t getNumRAMBanks(uint8_t ram);

MBC* mbc_init(uint8_t type_byte, uint8_t rom_byte, uint8_t ram_byte) {
    MBC* mbc = (MBC*) malloc(sizeof(MBC));

    //Check for null ptr
    if (mbc == NULL) {
        printError("Failed to initialize MBC chip");
        return NULL;
    }

    //Storing these values JUST in case.
    mbc->mbc_type_byte = type_byte;
    mbc->mbc_rom_size_byte = rom_byte;
    mbc->mbc_ram_size_byte = ram_byte;
    mbc->has_mode_switch = 0;
    mbc->has_battery = 0;
    mbc->has_rtc = 0;

    mbc->mbc_type = getMBCType(type_byte);

    //Games bigger than 1MiB and MBC1 wire this differently for more ROM space
    if (mbc->mbc_type == MBC_1 && rom_byte < 0x5)
        mbc->has_mode_switch = 1;

    //Battery MBCs
    if (type_byte == 0x3 || type_byte == 0x6 || type_byte == 0xF ||
        type_byte == 0x1B || type_byte == 0x1E)
        mbc->has_battery = 1;

    //RTC MBCs
    if (type_byte == 0xF)
        mbc->has_rtc = 1;

    //ROM and EXRAM banks
    mbc->num_rom_banks = 2 << rom_byte; //2^(rom_byte + 1)
    mbc->num_exram_banks = getNumRAMBanks(ram_byte);

    //Sets default values for registers flags
    mbc->mbc_mode = 0; //MBC1 specific
    //This always starts at 0, but no mbc alwasys has stataic addresses, 
    //and MBC1 always treats 0 as 1... For some reason.
    if (mbc->mbc_type == MBC_NONE || mbc->mbc_type == MBC_1)
        mbc->current_rom_bank = 1;
    else
        mbc->current_rom_bank = 0;
    mbc->current_exram_bank = 0;
    mbc->exram_enabled = 0; //Specifies if this is enabled

    return mbc;
}

//Frees MBC memory
void mbc_destroy(MBC* mbc) {
    if (mbc != NULL)
        free(mbc);
}

//Updates MBC information with address and values written to
void update_current_bank(MBC* mbc, uint16_t address, uint8_t value) {
    //Dispatch function that will handle each case individually. This allows
    //for more scalability if I add more down the line, or if I miss some weird quirks

    //It feels kind of messy to have to just have different functions for each of these, 
    //but these MBC chips are literally just wired differently, so there's no clean solution other than
    //encapsulate each one individually.
    switch(mbc->mbc_type) {
        case MBC_NONE:
            break; //Do nothing here
        case MBC_1:
            update_mbc1_data(mbc, address, value);
            break;
        case MBC_2:
            update_mbc2_data(mbc, address, value);
            break;
        case MBC_3:
            update_mbc3_data(mbc, address, value);
            break;
        case MBC_5:
            update_mbc5_data(mbc, address, value);
            break;
    }
}


//Specific MBC update information...
void update_mbc1_data(MBC* mbc, uint16_t address, uint8_t value) {
    /*
    * Each MBC has a range of addresses that will change the state of the MBC.
    * So the register doesn't get updated from a specific write, but a range.
    */

    //RAM Enable registers
    if (address <= 0x1FFF) {
        //If lower nibble is 0x0A, it gets enabled. Otherwise, disabled
        if ((value & 0x0F) == 0x0A)
            mbc->exram_enabled = 1;
        else
            mbc->exram_enabled = 0;
    }

    //ROM bank registers
    else if(address <= 0x3FFF) {
        //Bottom 5 bits select the lower 5 ROM bank bits
        uint8_t current = mbc->current_rom_bank;
        //Preserves top 3 bits of current, preserves bottom 5 of new value
        //Bitwise or combines them, and the last AND ignores bits that set the ROM bank too high.
        //Soo if there are less than 16, the & 0x0F keep the 5th bit 0.
        //Technically the same as a modulo I THINK, but this is "more hardware accurate"
        mbc->current_rom_bank = ((current & 0x60) | (value & 0x1F)) & (mbc->num_rom_banks - 1);
        if (mbc->current_rom_bank == 0)
            mbc->current_rom_bank = 1; //MBC1 treats 0 as 1 for this purpose
    }

    //RAM bank register
    else if (address <= 0x5FFF) {
        //"More RAM" mode
        if (mbc->mbc_mode == 1)
            mbc->current_exram_bank = value & 0x03; //Preserves bottom 2 bits
        else {
            //This basically will just edit bits 5 and 6 of the current value...
            mbc->current_rom_bank = (mbc->current_rom_bank & 0x9F) | ((value << 5) & 0x60);
            mbc->current_rom_bank &= (mbc->num_rom_banks - 1);
            if (mbc->current_rom_bank == 0)
                mbc->current_rom_bank = 1;
        }
    }

    //Mode register
    else if (address <= 0x7FFF) {
        //If this MBC is switchable
        if (mbc->has_mode_switch) {
            mbc->mbc_mode = value & 0x01; //Lowest bit sets this
        }
    }
}

void update_mbc2_data(MBC* mbc, uint16_t address, uint8_t value) {
    if (address <= 0x3FFF) {
        //Different things depending on bit 8 of the address
        if (((address >> 8) & 0x01) == 1) {
            //If bit 8 is set...
            //Bottom 4 bits select ROM bank
            mbc->current_rom_bank = (value & 0x0F) & (mbc->num_rom_banks - 1);
            //Sets ROM bank to 1 if its 0 on MBC2
            if (mbc->current_rom_bank == 0)
                mbc->current_rom_bank = 1;
        }
        else {
            //If bit 8 is clear...
            //Enable exram access if lower nibble is 0x0A
            mbc->exram_enabled = (value & 0x0F) == 0x0A;
        }
    }
}

//TODO: Implement this
//The logic with the RTC is so weirdly complex that I would like to just come
//back to this later. I will need more stupid structs and logic but whatever man
//that's just how it works with hardware.
void update_mbc3_data(MBC* mbc, uint16_t address, uint8_t value) {

}

void update_mbc5_data(MBC* mbc, uint16_t address, uint8_t value) {
    //Genuinely the most straightforward one

    //RAM Enable register
    if (address <= 0x1FFF) {
        mbc->exram_enabled = (value & 0x0F) == 0x0A; 
    }

    //ROM bank select
    else if (address <= 0x2FFF) {
        //Only lowest 8 bits of the current ROM bank
        mbc->current_rom_bank = (mbc->current_rom_bank & 0x0100) | value;
        mbc->current_rom_bank &= (mbc->num_rom_banks - 1);
    }

    //ROM bank select
    else if (address <= 0x3FFF) {
        //Selects 9th bit of current ROM bank
        mbc->current_rom_bank = (mbc->current_rom_bank & 0x00FF) | ((uint16_t)(value & 0x01) << 9);
        mbc->current_rom_bank &= mbc->num_rom_banks-1;
    }

    //RAM bank select
    else if (address <= 0x5FFF) {
        mbc->current_exram_bank = value & 0x0F;
    }
}


//Gets the specific MBC chip type
MBC_Type getMBCType(uint8_t type) {
    //Note that there are quite a few of these, but most of them are rare.
    //The ones I've implemented are the commonly used once as far as I know.
    //So, not every game is compatible... 

    //TODO: Implement the rest

    if (type == 0x00)
        return MBC_NONE;

    if (type <= 0x03)
        return MBC_1;

    if (type <= 0x05)
        return MBC_2;

    if (type == 0x0F || type == 0x11)
        //Other MBC3 values only apply to Japanese Pokemon Crystal, so for now they will not be included
        return MBC_3;

    if (type >= 0x19 && type <= 0x1E)
        return MBC_5;

    //Anything else is incompatible
    printError("Unsupported MBC Type! ROM incompatible");
    return MBC_NONE;
}

//Returns number of RAM banks
uint8_t getNumRAMBanks(uint8_t ram) {
    //Technically, the value specified in the type bit can ALSO
    //indicate that there is no RAM, however, under any NORMAL circumstance,
    //if a MBC doesn't have an RAM, this byte specifies that.

    if (ram == 0x0)
        return 0;

    if (ram == 0x2)
        return 1;

    if (ram == 0x3)
        return 4;

    if (ram == 0x4)
        return 16;

    if (ram == 0x5)
        return 8;

    return 0; //Placeholder for now.
}