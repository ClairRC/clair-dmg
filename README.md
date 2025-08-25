# ClairDMG
This is an emulator written in C that is capable of running games released for the original GameBoy. It has visuals and even audio! It also has a speed-up function. It supports most games, including games that use MBC1, MBC2, and MBC5 memory bank controllers. Currently, I am planning to add support for MBC3 as well. 

I made this project primarily to understand the deeper hardware aspects that we tend to overlook in our daily lives, and so by implementing all the little subsystems like the CPU, PPU, APU, and MMU (and dealing with the annoying hardware quirks of the GameBoy..), I got to learn a lot about that :)

## Installation and Usage

### Installation
To install, simply download the release for your operating system (currently only a Windows release is available).

### Building
#### Dependencies
* CMake
* SDL2

If you prefer to build the project yourself, a CMake file is included.

To build, clone the repository, navigate to the directory you want to install to, and run
```
cmake path/to/CMakeLists.txt
```

**Note**: This project uses find_package(SDL2) in CMake. Make sure SDL2 is installed and provides a valid configuration file. If you built SDL2 with CMake, this file should already be available.

### Usage
To run a game, play a ROM file in the same directory as the executable and name it "game.gb". If you also would like to include a boot ROM, then be sure to name that "boot.bin", although the boot ROM handling is currently
buggy.

#### Controls
Z -> A\
X -> B\
Enter -> Start\
Right Shift -> Select\
Arrow Keys -> D-Pad\
Space (Hold) -> Speed-up (x4 speed)

### TODO
* Add support for MBC3
* Fix audio issues
* Add GameBoy Color Support
* Add more customization for controls/opening ROMs
