#include "hardware.h" 


//Updates system time and other hardware states based on time passed
void update_hardware(EmulatorSystem* system, emu_time delta_time) {
    //I think that having this here is good because it makes it so that 
    //updating time passed ALWAYS accounts for specific hardware timing, which is
    //how it works in reality. Plus, it means the instruciton loop should only ever
    //update hardware atomically, which is also accurate to real behavior

    //TODO: Implement
}

//Takes emulator system pointer
void update_hardware_registers(EmulatorSystem* system) {
    //Start time since last update
    emu_time start_time = system->elapsed_time - system->delta_time;
    
    //Allows hardware update per tick
    for (int i = 0; i < system->delta_time; ++i) {
        
    }
}