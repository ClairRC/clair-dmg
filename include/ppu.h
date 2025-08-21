#ifndef PPU_H
#define PPU_H 

#include <stdint.h>
#include "memory_bus.h"
#include "ppu_state.h"
#include "hardware_def.h"
#include "sdl_data.h"

//PPU Frame definitions
#define VISIBLE_SCANLINES 144
#define SCANLINE_END 456
#define MODE_2_END 80
#define MODE_3_END 240
#define MODE_0_END 0
#define VBLANK_BEGIN (SCANLINE_END * VISIBLE_SCANLINES)
#define MODE_1_END 70224

#define MODE_3_TIME(frame_time) ((frame_time) % SCANLINE_END) - 80 //Current time into mode 3

//LCDC flag bits
#define BG_WIN_ENABLE 1 << 0
#define OBJ_ENABLE 1 << 1
#define OBJ_SIZE 1 << 2
#define BG_TILE_MAP 1 << 3
#define BG_WIN_INDEX_MODE 1 << 4
#define WIN_ENABLE 1 << 5
#define WIN_TILE_MAP 1 << 6

//Object flag bits
#define OBJ_DMG_PALETTE 1 << 4
#define OBJ_X_FLIP 1 << 5
#define OBJ_Y_FLIP 1 << 6
#define OBJ_PRIORITY 1 << 7

//Color Palette Values
#define PALETTE_WHITE 0xFFFFFF
#define PALETTE_LIGHT_GRAY 0xAAAAAA
#define PALETTE_DARK_GRAY 0x555555
#define PALETTE_BLACK 0x000000

/*#define PALETTE_WHITE 0xFFFFFF
#define PALETTE_LIGHT_GRAY 0xAAAAAA
#define PALETTE_DARK_GRAY 0x555555
#define PALETTE_BLACK 0x000000*/

//This is the Pixel Processing Unit, which is the GameBoy's screen basically.

//Flags that are internal to the PPU
typedef struct {
    uint16_t window_ly; //Window has an internal scanline counter 
    uint8_t window_ly_increment; //Flag of whether or not window's internal scanline counter has been incremented on this scanline
    uint8_t prev_stat; //Whether stat was previous triggered or not

    int current_obj_index; //Tracks the current index of the scanline's sprite array
    int pixel_obj_index; //Tracks object that is drawn on current pixel to avoid looping multiple times
} LocalPPUState;

//Struct for Object attributes, which will help drawing a lot
typedef struct {
    int16_t y_pos;
    int16_t x_pos;
    uint8_t tile_index;
    uint8_t flags;
} OAM_Entry;

//Struct to store necessary information for drawing object
typedef struct {
    uint8_t color;
    uint8_t flags;
} ObjColorData;

//Palette data for bg and objects
//Placeholder for now, buuut when CGB implementation happens this will matter a lot
typedef struct {
    uint32_t BG_Palette[4];
    uint32_t OBJ0_Palette[4];
    uint32_t OBJ1_Palette[4];
} PaletteData;

typedef struct {
    MemoryBus* bus; //Memory
    SDL_Display_Data* sdl_data;

    //Scanline OAM entires
    OAM_Entry scanline_obj[10]; //Each scanline can have at max 10 sprites visible

    //Frame buffer and current palette data for drawing...
    uint32_t* framebuffer; //Screen frame buffer
    PaletteData* palette; //DMG palette consistes of 4 colors. Place holder for now.

    //PPU state flags
    LocalPPUState local_state;
    GlobalPPUState* global_state;
} PPU;

//PPU functions
PPU* ppu_init(MemoryBus* bus, GlobalPPUState* global_state, SDL_Display_Data* sdl_data);
void ppu_destroy(PPU* ppu);
void update_ppu(PPU* ppu);
void draw(PPU* ppu);
void ppu_oam_scan(PPU* ppu);
void ppu_write_lcd(PPU* ppu);
void update_stat(PPU* ppu);

//PPU Mode switch functions
void switch_mode_0_1(PPU* ppu);
void switch_mode_1_2(PPU* ppu);
void switch_mode_2_3(PPU* ppu);
void switch_mode_3_0(PPU* ppu);
void switch_mode_0_2(PPU* ppu);

//Helper functions
void update_ppu_state(PPU* ppu);
void store_object(PPU* ppu, uint16_t object_ba);
uint8_t get_pixel_color_id(PPU* ppu, uint16_t mode_3_time);
uint8_t obj_color_id_from_index(PPU* ppu, uint8_t color_index, uint8_t palette);
uint8_t get_bg_color_index(PPU* ppu, uint16_t mode_3_time);
uint8_t get_win_color_index(PPU* ppu, uint16_t mode_3_time);
ObjColorData get_obj_color_data(PPU* ppu, uint16_t mode_3_time);
uint8_t get_color_index(PPU* ppu, uint8_t tile_x, uint8_t tile_y, uint8_t tile_index, uint8_t tile_area);
uint8_t win_is_visible(PPU* ppu, uint16_t mode_3_time);
int visible_obj_index(PPU* ppu, uint16_t mode_3_time);


#endif 