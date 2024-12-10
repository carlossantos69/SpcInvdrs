// This file is suposed to have all the code for the display


// game-server.c launches a process that runs this code
// outer-space-display.c runs this code (only one process maybe)

// game-server.c and outer-space-display.c need to include this

#ifndef SPACE_DISPLAY_H
#define SPACE_DISPLAY_H

#include <time.h>

void initialize_display();
void update_grid();
void draw_scores();
void draw_grid();
void show_victory_screen();
void display_main(void* sub);

// Player structure
typedef struct {
    char id;
    int score;
    int active;  // flag to track if we've seen this player
} disp_Player_t;

typedef struct {
    char ch;
    time_t laser_time; // Timestamp when the laser was drawn
} disp_Cell_t;

#endif