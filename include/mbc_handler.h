#ifndef MBC_HANDLER_H
#define MBC_HANDLER_H 

#include "hardware_def.h"
#include <stdint.h>

/*
* Different cartridges have different layouts and rules for their banked memory, so
* this will help keep track of that and adjust accordingly. The cartridge
* will tell the emulator what the MBC is, and then this will be what handles the
* setup and all that. Annoyingly this means I need ANOTHER lookup table..
*/

//Stores information regarding memory banking
typedef struct {
    //Metadata info from cartridge header
    MBC_Type mbc_type;
    uint8_t mbc_type_byte;
    uint8_t mbc_rom_size_byte;
    uint8_t mbc_ram_size_byte;
    uint8_t has_mode_switch; //MBC1 specific
    uint8_t has_battery;
    uint8_t has_rtc; //Whether or not this has a real time clock

    //Information needed for bank switching
    uint8_t mbc_mode; //MBC1 specific
    uint16_t current_rom_bank;
    uint16_t num_rom_banks;
    uint8_t current_exram_bank;
    uint8_t num_exram_banks;
    uint8_t exram_enabled; //Specifies if this is enabled
} MBC;

//Sets up MBC chip information
MBC* mbc_init(uint8_t, uint8_t, uint8_t);
void mbc_destroy(MBC*);

//Determines current bank
//For MBC1, it will also handle switching modes
void update_current_bank(MBC*, uint16_t, uint8_t);
void update_mbc1_data(MBC*, uint16_t, uint8_t);
void update_mbc2_data(MBC*, uint16_t, uint8_t);
void update_mbc3_data(MBC*, uint16_t, uint8_t);
void update_mbc5_data(MBC*, uint16_t, uint8_t);

#endif