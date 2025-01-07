/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: space-display.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Header file for space-display.h
 */

#ifndef SPACE_DISPLAY_H
#define SPACE_DISPLAY_H

/**
 * @brief Structure to represent a player in the display system.
 * 
 * This structure holds information about a player, including their
 * unique identifier, score, and an active flag to indicate if the
 * player is currently active or has been seen.
 */
typedef struct {
    char id;
    int score;
    int active;  // flag to track if we've seen this player
} disp_Player_t;

/**
 * @struct disp_Cell_t
 * @brief Represents a cell in the display grid.
 *
 * This structure is used to represent a single cell in the display grid.
 * Each cell contains a character
 * was drawn in that cell.
 *
 * @var disp_Cell_t::ch
 * Character representing the content of the cell.
 *
 */
typedef struct {
    char ch;
} disp_Cell_t;


// Extern variables that will be used in other files
extern char game_state_display[BUFFER_SIZE];
extern pthread_mutex_t display_lock;


/**
 * @brief Initializes the display for the game.
 * 
 * This function sets up the necessary resources and configurations
 * to start displaying the game interface.
 */
void initialize_display();

/**
 * @brief Updates the game grid.
 * 
 * This function refreshes the game grid based on the current state
 * of the game. It should be called whenever the game state changes.
 */
void update_grid();

/**
 * @brief Draws the scores on the display.
 * 
 * This function updates the score display with the current scores
 * of the players.
 */
void draw_scores();

/**
 * @brief Draws the game grid on the display.
 * 
 * This function renders the game grid on the display, showing the
 * current positions of all game elements.
 */
void draw_grid();

/**
 * @brief Shows the victory screen.
 * 
 * This function displays the victory screen when a player wins the game.
 */
void show_victory_screen();

/**
 * @brief Main display function.
 * 
 * @param sub A pointer to a subroutine or data structure used by the display.
 * 
 * This function is the main entry point for the display module, handling
 * the overall display logic and coordination.
 */
void display_main();

#endif