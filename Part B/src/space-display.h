/*
 * PSIS 2024/2025 - Project Part 2
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

/**
 * @brief Initializes the display for the space game.
 *
 * This function initializes color pairs, clears the grid, initializes player scores, 
 * and draws the grid with row and column numbers as well as a border around the grid.
 *
 * The grid is initialized to empty spaces, and player scores are set to zero.
 * Row numbers are drawn on the left side of the grid, and column numbers are
 * drawn on the top. A border is drawn around the grid to visually separate it
 * from the rest of the display.
 */
void initialize_display();

/**
 * @brief Updates the game grid and player statuses based on the current game state string.
 *
 * This function clears the current grid and player statuses, then parses the state string
 * to update the grid with new player positions, alien positions, laser beams, and player scores.
 * It also sets a flag if the game is over.
 * 
 *
 * The game state string contains information about the game state, including player positions.
 * The string can either be passed from the server thread for the game-server.c application or
 * read via zeroMQ for outer-space-display.c. and astronaut-display-client.c applications.
 * 
 * @note This functions is not thread-safe and should be called with the display_lock mutex held.
 */
void update_grid();

/**
 * @brief Draws the scores of active players on the screen.
 *
 * This function clears the score area, displays the scores header, 
 * and lists the scores of all active players. It also draws a border 
 * around the scores area.
 *
 * The scores are displayed in bold and color-coded for astronauts.
 * The function assumes that the players_disp array contains information 
 * about the players, including their active status, ID, and score.
 *
 * The score area is cleared before displaying the scores, and a border 
 * is drawn around the scores area to visually separate it from other 
 * parts of the screen.
 * 
 * @note This functions is not thread-safe and should be called with the display_lock mutex held.
 */
void draw_scores();

/**
 * @brief Draws the game grid on the screen.
 *
 * This function iterates through the game grid and draws each cell based on its content.
 * It handles the following elements:
 * - Aliens ('*') with a specific color.
 * - Lasers (horizontal and vertical) with a specific color and bold attribute.
 * - Astronauts (characters 'A' to 'H') with a specific color.
 * - Empty cells as spaces.
 *
 * The function also draws the scores and refreshes the screen.
 * 
 * @note This functions is not thread-safe and should be called with the display_lock mutex held.
 */
void draw_grid(void);

/**
 * @brief Displays the victory screen at the end of the game.
 *
 * This function clears the terminal screen and displays the victory screen,
 * which includes the game over message, the winner's information, and the 
 * final scores of all active players. It waits for user input before exiting.
 *
 * It is called when the game ends to show the final results to the players.
 * 
 * @note This functions is not thread-safe.
 */
void show_victory_screen();

/**
 * @brief Sets the game state display to the provided buffer content.
 *
 * This function locks the display mutex, copies the provided buffer content
 * to the game state display, sets the state_changed flag to 1, signals the
 * condition variable to indicate the state has changed, and then unlocks the
 * display mutex.
 *
 * @param buffer A pointer to the buffer containing the new game state to display.
 */
void set_display_game_state(char* buffer);

/**
 * @brief Main display function that initializes the display and handles the main display loop.
 *
 * This function initializes the display and enters the main loop where it draws the screen. 
 * The grid and display info should be modified in memory using the mutex in .h file. This function only draws the screen. External code must update the grid.
 * When the game is over, it shows the victory screen.
 *
 * @return Returns 0 if no error, -1 otherwise.
 */
int display_main();

#endif