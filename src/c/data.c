#include <pebble.h>
#include "data.h"

//these variables are exposed and shared among all files that include data.h
//char stops[10][60]; //done: dynamic allocation
//char buses[10][60]; //done: dynamic allocation

uint16_t selectedStopIndex;
uint16_t numETAs;
int numStops;
char *errorMessage = "An error occurred";