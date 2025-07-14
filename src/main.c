#include "init.h"
#include "logging.h"

//Simply initializes current emulator for now..
int main() {
    int success = emulator_init();

    if (success == 1)
        printError("Initialization failed");

    return success;
}