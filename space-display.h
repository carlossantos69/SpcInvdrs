// This file is suposed to have all the code for the display


// game-server.c launches a process that runs this code
// outer-space-display.c runs this code (only one process maybe)

// game-server.c and outer-space-display.c need to include this

#ifndef SPACE_DISPLAY_H
#define SPACE_DISPLAY_H

void initialize_display();
void update_grid();
void draw_scores();
void draw_grid();
void show_victory_screen();
void display_main(void* cont, void* sub);

#endif