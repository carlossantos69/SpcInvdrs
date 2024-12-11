/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: space-display.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Description:
 * Header file for space-display.h
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