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
    if (mbc->mbc_type == MBC_1 && rom_byte >= 0x5)
        mbc->has_mode_switch = 1;

    //Battery MBCs
    if (type_byte == 0x3 || type_byte == 0x6 || type_byte == 0xF ||
        type_byte == 0x1B || type_byte == 0x1E)
        mbc->has_battery = 1;

    //RTC MBCs
    if (type_byte == 0xF)
        mbc->has_rtc = 1;

    //ROM and EXRAM banks
    mbc->num_rom_banks = 2 << rom_byte; //2^(rom_byte - 1)
    mbc->num_exram_banks = getNumRAMBanks(ram_byte);

    //Sets default values for registers flags
    mbc->mbc_mode = 0; //MBC1 specific
    mbc->current_rom_bank = 0;
    mbc->current_exram_bank = 0;
    mbc->exram_enabled = 0; //Specifies if this is enabled
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
        //Other MBC3 values only apply to Japanese Pokemon Crystal, sooo oopss
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

    else 
        return 0; //Placeholder for now.
}