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

    //MBC chip metadata (mostly for debugging but not necessarily needed)
    mbc->mbc_type_byte = type_byte;
    mbc->mbc_rom_size_byte = rom_byte;
    mbc->mbc_ram_size_byte = ram_byte;

    //Get MBC chip type
    mbc->mbc_type = getMBCType(type_byte);

    /*
    * Default MBC Values
    */

    mbc->current_rom_bank = 1; //Starts at 1 by default TODO: confirm
    mbc->mbc_mode = 1; //MBC1 specific but keeping here for clarity
    mbc->current_exram_bank = 0; //Starts at 0 by default
    mbc->exram_enabled = 0; //Specifies if this is enabled. Starts disabled
    mbc->has_mode_switch = 0; //MBC1 has a mode switch for "more ram" or "more rom"
    mbc->has_battery = 0;
    mbc->has_rtc = 0;

    /*
     * Get general MBC Information
     */
    
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

    /*
     * MBC Specific values
     */

    switch (mbc->mbc_type) {
        case MBC_NONE:
            //Since no MBC has no bank switching, there is always just bank 0 and 1, and the area for the switchable bank is just static
            //TODO: Confirm that there is always at least 2 ROM banks even with no MBC
            mbc->current_rom_bank = 1;
            if (mbc->num_exram_banks > 0) { mbc->exram_enabled = 1; } //SOME no mbc cartridges have RAM, in which case it is just static addresses
            break;

        case MBC_1:
            //MBC1 cartridges with less than 1MiB of memory allow for a "more ROM" or "more RAM" mode
            if (rom_byte < 0x5) { mbc->has_mode_switch = 1; }
            mbc->mbc_mode = 0; //MBC1 specific. Starts at 0 by default
            //TODO: Confirm if the thing below is true for all MBCs? Not super sure..
            mbc->current_rom_bank = 1; //MBC1 treats bank 0 as 1, so this starts at 1
            break;

        default:
            break;
    }

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
    //BANK1
    else if(address <= 0x3FFF) {
        //Bottom 5 bits select the lower 5 ROM bank bits
        uint8_t current = mbc->current_rom_bank;

       
        //This is probably the issue..
        //Preserves top 3 bits of the current bank and replaces the bottom 5
        current &= 0x60; //Preserves bits 6 and 7 (BANK2)
        current |= value & 0x1F; //Sets bottom 5 bits (BANK1)
        current %= mbc->num_rom_banks; //Masks the value based on number of rom banks

        if ((value & 0x1F) == 0x0)
            current = 1; //BANK 1 corrects 0 value to 1

        mbc->current_rom_bank = current;

    }

    //RAM bank register
    else if (address <= 0x5FFF) {
        //"More RAM" mode (Mode 1)
        if (mbc->mbc_mode == 1) {
            mbc->current_exram_bank = value & (mbc->num_exram_banks - 1);
        }

        //"More ROM" mode (Mode 0)
        //BANK2
        else {
            //If this can be swtiched, then mode 0 only allwos reads from RAM bank 0
            //Not sure exaaactly why?
            //TODO: Confirm that and learn More
            if (mbc->has_mode_switch) {
                mbc->current_exram_bank = 0;
            }

            //Changes top 2 bits of the ROM bank
            uint8_t current = mbc->current_rom_bank;
            current &= 0x1F; //Preserve lower bits (BANK1)
            current |= (value & 0x03) << 5; //Sets upper 3 bits (BANK2)
            current %= mbc->num_rom_banks;

            //Only sets the bank if the cartridge has enough banks
            //TODO: This MIGHT be false, it might wrap around?
            mbc->current_rom_bank = current;
        }
    }

    //Mode register
    else if (address <= 0x7FFF) {
        //Switches mode if this MBC is switchable
        if (mbc->has_mode_switch) {
            mbc->mbc_mode = value & 0x01; //Lowest bit sets this
        }
    }
}

void update_mbc2_data(MBC* mbc, uint16_t address, uint8_t value) {
    if (address <= 0x3FFF) {
        //Depending on bit 8 of the address, this is either the RAM enable register,
        //or the ROM bank register

        //If bit 8 of the address is clear, this is the RAM enable register
        if (((address >> 8) & 0x1) == 0) {
            //If lower nibble of value is 0x0A, enable register. Otherwise, disable
            mbc->exram_enabled = (value & 0x0F) == 0x0A;
        }

        //If bit 8 is set, this selects the ROM bank
        else {
            //Only sets the bank if the cartridge has enough banks
            //TODO: This MIGHT be false, it might wrap around?
            if (value < mbc->num_rom_banks)
                mbc->current_rom_bank = value;

            //treat bank 0 as bank 1
            //TODO: This might only be an MBC1 thing? Or not? I'm not super sure
            if (mbc->current_rom_bank == 0)
                mbc->current_rom_bank = 1;
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

    if (type == 0x01 ||
        type == 0x02 ||
        type == 0x03)
        return MBC_1;

    if (type == 0x05 ||
        type == 0x06)
        return MBC_2;

    if (type == 0x0F ||
        type == 0x10 ||
        type == 0x11 ||
        type == 0x12 ||
        type == 0x13)
        return MBC_3;

    if (type >= 0x19 && type <= 0x1E)
        //This range is all MBC5
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