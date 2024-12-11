/*
 * File: space-display.c
 * Author: Carlos Santos e Tomás Corral
 * Description: This is the header file for the space display program.
 * Date: 11/12/2024
 */

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