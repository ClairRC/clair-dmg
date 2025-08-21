#include <stdio.h>
#include "logging.h"

//Helper function that will print error IF suppression isnt enabled
//This is ONLY so my unit tests dont give error messages.

//#define SUPPRESS_ERROR_OUTPUT
void printError(char* msg) {
    #ifndef SUPPRESS_ERROR_OUTPUT
        fprintf(stderr, "%s\n", msg);
    #endif
}