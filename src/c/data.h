#pragma once
#include <pebble.h>
struct ROUTE {
  char *name;
  char *colour;
};
typedef struct ROUTE Route;

extern uint16_t selectedStopIndex;
extern uint16_t numETAs;
extern int numStops;
extern char *errorMessage;