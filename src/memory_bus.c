#include "memory_bus.h" 
#include "logging.h"
#include "hardware_def.h"
#include "hardware_registers.h"

#include <stdlib.h>

MemoryBus* memory_bus_init(Memory* mem, GlobalSystemState* system_state) {
	if (mem == NULL || system_state == NULL) {
		printError("Error intializing memory bus");
		return NULL;
	}

	MemoryBus* bus = (MemoryBus*)malloc(sizeof(MemoryBus));

	if (bus == NULL) {
		printError("Error intializing memory bus");
		return NULL;
	}

	bus->memory = mem;
	bus->system_state = system_state;

	return bus;
}

void memory_bus_destroy(MemoryBus* bus) {
	if (bus != NULL)
		free(bus);
}

//Reads memory or returns 0xFF as a default value if location is inaccessible
uint8_t mem_read(MemoryBus* bus, uint16_t address, Accessor accessor) {
	//Get memory information from memory module
	MemoryValue mem_value = get_memory_value(bus->memory, address); //Gets memory information
	
	//If memory area is inaccessible, then reutrn 0xFF as a default
	if (!mem_accessible(bus, mem_value.range, accessor) || mem_value.mem_ptr == NULL) {
		printError("Read at inaccessible address");
		return 0xFF;
	}

	uint8_t result = *(mem_value.mem_ptr);

	//Edge case where MBC2 only returns the lower nibble for EXRAM reads.
	if (mem_value.range == RANGE_EXRAM && bus->memory->mbc_chip->mbc_type == MBC_2)
		result |= 0xF0;

	//Hardware registers have some special properties
	if (mem_value.range == RANGE_IO) {
		//Address 0xFF00 returns current input value. This changes depending on selector bit.
		//For now, this will just poll SDL events and get the byte based on that
		if (address == 0xFF00) { result = get_input_byte(bus->memory, result); }

		//Most hardware registers are a combination of read/write only, so this masks the output
		result = mask_hw_reg_read(result, address);
	}

	return result;
}

//Writes to memory and updates emulator state if necessary
//Returns 0 if no write occurs, 1 if a write does occur
uint8_t mem_write(MemoryBus* bus, uint16_t address, uint8_t new_val, Accessor accessor) {
	//Get memory information from memory module
	MemoryValue mem_value = get_memory_value(bus->memory, address); //Gets memory information
	uint8_t success = 1; //Defaults to no write

	//If area is accessible and not read only, then do the write stuff
	if (mem_accessible(bus, mem_value.range, accessor) && mem_value.range != RANGE_ROM && mem_value.mem_ptr != NULL) {
		uint8_t* mem_ptr = mem_value.mem_ptr;

		//Edge case where MBC2 exram only contains the lower nibble
		if (mem_value.range == RANGE_EXRAM && bus->memory->mbc_chip->mbc_type == MBC_2)
			new_val &= 0x0F;

		//Handle IO Range shenanigans
		if (mem_value.range == RANGE_IO) {
			new_val = mask_hw_reg_write(new_val, *mem_ptr, address);
			update_global_state(bus, address, new_val); //Updates various hardware register related states
			
			//If APU is disabled, then APU hardware registers become read only
			if (address >= 0xFF10 && address <= 0xFF25 && bus->system_state->apu_state->apu_enable == 0)
				new_val = *mem_ptr; //Set new val to old val, so no writes are done
		}

		//Update memory address with new value
		//I miiight be missing edge cases?
		*mem_ptr = new_val;
		success = 0;
	}

	//Updates memory bank info. This happens regardless if a write happens or not
	//(although these registers are typically in ROM so it won't do the write anyway but still)
	update_current_bank(bus->memory->mbc_chip, address, new_val);
	return success;
}

//Updates subsystem state depending on hardware register writes
void update_global_state(MemoryBus* bus, uint16_t address, uint8_t new_val) {
	//Writes here disable boot rom
	if (address == 0xFF50 && bus->memory->local_state.boot_rom_mapped == 1)
		disable_bootrom(bus->memory);

	//This register begins DMA transfer if its not active already
	else if (address == 0xFF46) {
		if (!bus->system_state->dma_state->active) {
			bus->system_state->dma_state->active = 1;
			bus->system_state->dma_state->source = new_val;
		}
	}

	//Writes to DIV reset system clock
	else if (address == 0xFF04)
		bus->system_state->timer_state->system_time = 0;

	//Bit 7 of LCDC controls whether PPU is on or not
	else if (address == 0xFF40) {
		if (((new_val >> 7) & 0x1) == 1) {
			//If LCD was previously off, reset frame time and turn it on
			if (bus->system_state->ppu_state->lcd_on == 0) {
				bus->system_state->ppu_state->lcd_on = 1;
				bus->system_state->ppu_state->frame_time = 4;
			}
		}
		else
			bus->system_state->ppu_state->lcd_on = 0;
	}

	//Bit 7 of NR52 turns on/off APU
	else if (address == 0xFF26) {
		if (((new_val >> 7) & 0x1) == 1)
			bus->system_state->apu_state->apu_enable = 1; //Turn APU on
		else
			//Set the flag for APU to clear registers
			bus->system_state->apu_state->turn_off_apu = 1;
	}

	//Bit 7 of NRx4 triggers channel x
	//Bit 6 for channels 1-3 determine if the note has length timer
	else if (address == 0xFF14) {
		if (new_val & 0x80)
			bus->system_state->apu_state->trigger_ch1 = 1;

		if (new_val & 0x40)
			bus->system_state->apu_state->ch1_length_enable = 1;
		else
			bus->system_state->apu_state->ch1_length_enable = 0;
	}

	else if (address == 0xFF19) {
		if (new_val & 0x80)
			bus->system_state->apu_state->trigger_ch2 = 1;

		if (new_val & 0x40)
			bus->system_state->apu_state->ch2_length_enable = 1;
		else
			bus->system_state->apu_state->ch2_length_enable = 0;
	}

	else if (address == 0xFF1E) {
		if (new_val & 0x80)
			bus->system_state->apu_state->trigger_ch3 = 1;

		if (new_val & 0x40)
			bus->system_state->apu_state->ch3_length_enable = 1;
		else
			bus->system_state->apu_state->ch3_length_enable = 0;
	}

	else if (address == 0xFF23) {
		if (new_val & 0x80)
			bus->system_state->apu_state->trigger_ch4 = 1;

		if (new_val & 0x40)
			bus->system_state->apu_state->ch4_length_enable = 1;
		else
			bus->system_state->apu_state->ch4_length_enable = 0;
	}
}

//Handles input register
uint8_t get_input_byte(Memory* mem, uint8_t val) {
	//Flags for whether buttons or d-pad is selected
	uint8_t read_dpad = !(mem->io[0x0] & 1 << 4);
	uint8_t read_buttons = !(mem->io[0x0] & 1 << 5);

	uint8_t joypad_state = 0xF; //Default joypad state

	//Read buttons
	if (read_dpad)
		joypad_state &= mem->local_state.dpad_state;

	if (read_buttons)
		joypad_state &= mem->local_state.button_state;

	//Keep top nibble, replace bottom nibble with button inputs
	val = (val & 0xF0) | (joypad_state & 0x0F);

	return val;
}

//Handles edge cases for hardware register reads
uint8_t mask_hw_reg_read(uint8_t val, uint16_t address) {
	HardwareRegister hw_reg = hw_registers[(uint8_t)address]; //Gets hardware register from LSB

	//Return default value if register is CGB only and current mode is DMG
	if (hw_reg.cgb_only)
		return 0xFF;

	//Bits with a 0 are write-only, so this forces those bits to be set as 1, which is correct behavior for this
	val |= ~hw_reg.read_mask;

	return val;
}

//Masks bits for writing
uint8_t mask_hw_reg_write(uint8_t new_val, uint8_t old_val, uint16_t address) {
	HardwareRegister hw_reg = hw_registers[(uint8_t)address]; //Gets hardware register from LSB

	//Bits with a 0 are read-only, so this forces those bits to not be updated, which is correct behavior for this
	new_val = (new_val & hw_reg.write_mask) | (old_val & ~hw_reg.write_mask);

	return new_val;
}

//Checks whether memory location is accessible based on memory range and who is accessing
//Returns 0 for inaccesible, 1 for accessible
uint8_t mem_accessible(MemoryBus* bus, MemoryRange range, Accessor accessor) {
	uint8_t dma_active = bus->system_state->dma_state->active;
	PPU_Mode current_ppu_mode = bus->system_state->ppu_state->current_mode;
	uint8_t accessible = 1; //Default accessible

	switch (range) {
		//ROM is inaccessible by the CPU when DMA is active
		case RANGE_ROM:
			if (dma_active && accessor == CPU_ACCESS)
				accessible = 0;
			break;

		//VRAM is inaccessible by CPU during PPU Mode 3 and DMA transfer
		case RANGE_VRAM:
			if (accessor == CPU_ACCESS) {
				if (current_ppu_mode == PPU_MODE_3 || dma_active)
					accessible = 0;
			}
			break;

		//EXRAM is inaccessibly by CPU during DMA or if MBC has it disabled
		case RANGE_EXRAM:
			if (bus->memory->exram_x == NULL)
				accessible = 0;

			if (accessor == CPU_ACCESS) {
				if (!(bus->memory->mbc_chip->exram_enabled) || dma_active)
					accessible = 0;
			}
			break;

		//WRAM is inaccessible by CPU is DMA transfer is active
		case RANGE_WRAM:
			if (accessor == CPU_ACCESS && dma_active)
				accessible = 0;
			break;

		//OAM is inaccessible by CPU during DMA transfer and PPU modes 2 and 3
		case RANGE_OAM:
			if (accessor == CPU_ACCESS) {
				if (current_ppu_mode == PPU_MODE_2 || current_ppu_mode == PPU_MODE_3 || dma_active)
					accessible = 0;
			}
			break;

		//Self explanatory
		case RANGE_PROHIBITED:
			accessible = 0;
			break;

		//All other regions always accessible 
		default:
			break;
	}

	return accessible;
}