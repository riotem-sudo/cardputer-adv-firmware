#pragma once

#include <stdint.h>

// Lightweight protocol definition for RF brute force - no heap, no vtable, no std::map
struct BruteProtocol {
    const char *name;
    int bits;
    int zero[2];  // {duration1, duration2} for bit '0'
    int one[2];   // {duration1, duration2} for bit '1'
    int pilot[2]; // {duration1, duration2} for pilot/sync (0,0 = none)
    int stop[2];  // {duration1, duration2} for stop bit  (0,0 = none)
};

// All protocols defined as constexpr data - zero RAM cost, lives in flash
// Timing convention: positive = HIGH, negative = LOW
static constexpr BruteProtocol brute_protocols[] = {
    // name               bits  zero           one           pilot            stop
    {"Came 12bit",       12, {-320, 640},  {-640, 320},  {-11520, 320}, {0, 0}       },
    {"Nice 12bit",       12, {-700, 1400}, {-1400, 700}, {-25200, 700}, {0, 0}       },
    {"Ansonic 12bit",    12, {-1111, 555}, {-555, 1111}, {-19425, 555}, {0, 0}       },
    {"Holtek 12bit",     12, {-870, 430},  {-430, 870},  {-15480, 430}, {0, 0}       },
    {"Linear 10bit",     10, {500, -1500}, {1500, -500}, {0, 0},        {500, -21500}},
    {"Chamberlain 9bit", 9,  {-870, 430},  {-430, 870},  {0, 0},        {-3000, 1000}},
    // Princeton / PT2262 family: 1:3 ratio, TE 350us, sync = 1TE high + guard 30TE low
    {"Princeton 24bit",  24, {350, -1050}, {1050, -350}, {0, 0},        {350, -10500} },
    // Linear Delta 3 (8bit) shares Linear timing, shorter code space
    {"LinearDelta3 8bit", 8, {500, -1500}, {1500, -500}, {0, 0},        {500, -21500} },
};

static constexpr int BRUTE_PROTOCOL_COUNT = sizeof(brute_protocols) / sizeof(brute_protocols[0]);

void rf_bruteforce();

// UNILARM fixed-code (25 bit = 16 bit DIP address + 9 bit button instruction).
// Not a 2^25 sweep: enumerates the 16 bit address space in 4096-code chunks.
void rf_unilarm();
