#ifndef PPU_H
#define PPU_H 

#include <stdint.h>
#include "memory.h"
#include "hardware_def.h"
#include "game_display.h"

//This is the Pixel Processing Unit, which is the GameBoy's screen basically.

//State flags for PPU
typedef struct {
    uint8_t lcd_on; //Whether LCD is on or not TODO: Might be redundant?

    uint16_t window_ly; //Window has an internal scanline counter 
    uint8_t window_ly_increment; //Flag of whether or not window's internal scanline counter has been incremented on this scanline

    uint8_t prev_stat; //Whenther stat was previous triggered or not

    int current_obj_index; //Tracks the current index of the scanline's sprite array
    int pixel_obj_index; //Tracks object that is drawn on current pixel to avoid looping multiple times
} PPUState;

//Struct for Object attributes, which will help drawing a lot
typedef struct {
    int16_t y_pos;
    int16_t x_pos;
    uint8_t tile_index;
    uint8_t flags;
} OAM_Entry;

typedef struct {
    Memory* memory; //Memory

    //Scanline OAM entires
    OAM_Entry scanline_obj[10]; //Each scanline can have at max 10 sprites visible

    //Frame buffer and current palette data for drawing...
    uint32_t* framebuffer; //Screen frame buffer
    uint32_t dmg_palette[4]; //DMG palette consistes of 4 colors. Place holder for now.

    //PPU state flags
    PPUState state;
} PPU;

PPU* ppu_init(Memory* mem);
void ppu_destroy(PPU* ppu);
void ppu_oam_scan(PPU* ppu, uint8_t mode_2_time);
void ppu_write_lcd(PPU* ppu, uint16_t mode_3_time);
void draw(PPU* ppu);

//PPU Mode switch functions
void switch_mode_0_1(PPU* ppu);
void switch_mode_1_2(PPU* ppu);
void switch_mode_2_3(PPU* ppu);
void switch_mode_3_0(PPU* ppu);
void switch_mode_0_2(PPU* ppu);

uint8_t get_bg_color_index(PPU* ppu, uint16_t mode_3_time); //Helper function to get background color
uint8_t get_win_color_index(PPU* ppu, uint16_t mode_3_time); //Helper function to get window color
uint8_t get_obj_color_index(PPU* ppu, uint16_t mode_3_time, int object_index); //Helper function to get object color
uint8_t get_color_index(PPU* ppu, uint8_t tilemap_pixel_x, uint8_t tilemap_pixel_y, uint8_t tile_index, uint8_t tile_area); //Gets Color index for background and window tiles

void store_object(PPU* ppu, uint16_t object_ba);
uint8_t win_is_visible(PPU* ppu, uint16_t mode_3_time);
int visible_obj_index(PPU* ppu, uint16_t mode_3_time);
uint8_t get_pixel_color_id(PPU* ppu, uint16_t mode_3_time);
uint8_t obj_color_id_from_index(PPU* ppu, uint8_t color_index, uint8_t palette);

#endif 