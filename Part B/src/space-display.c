/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: game-logic.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Logic for the UI and screen using ncurses. All the UI code is here
 */

#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "space-display.h"
#include <string.h>

// Array to store the display information of players
disp_Player_t players_disp[MAX_PLAYERS];

// 2D array to represent the game grid
disp_Cell_t grid[GRID_HEIGHT][GRID_WIDTH];

// Flag indicating whether the game is over
int game_over_display = 0;

// Mutex for internal display data
pthread_mutex_t display_lock;

// Flag to request the display to update the game grid
int state_changed = 0;
// Condition variable to signal display
pthread_cond_t state_changed_cond;

// This string is parsed to obtain the grid and player data
//The game state string contains information about the game state, including player positions.
//The string can either be passed from the server thread for the game-server.c application or
//read via zeroMQ for outer-space-display.c. and astronaut-client.c applications.
char game_state_display[BUFFER_SIZE];


/**
 * @brief Initializes the display for the space game.
 *
 * This function sets up the ncurses environment, initializes color pairs,
 * clears the grid, initializes player scores, and draws the grid with row
 * and column numbers as well as a border around the grid.
 *
 * The grid is initialized to empty spaces, and player scores are set to zero.
 * Row numbers are drawn on the left side of the grid, and column numbers are
 * drawn on the top. A border is drawn around the grid to visually separate it
 * from the rest of the display.
 */
void initialize_display() {
    // Initialize grid to empty spaces
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
        }
    }

    // Initialize player scores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players_disp[i].id = '\0';
        players_disp[i].score = 0;
    }

    // Initialize color pairs
    init_pair(COLOR_ASTRONAUT, COLOR_GREEN, COLOR_BLACK);   // Astronauts
    init_pair(COLOR_ALIEN, COLOR_RED, COLOR_BLACK);         // Aliens
    init_pair(COLOR_LASER, COLOR_RED, COLOR_BLACK);      // Lasers

    // Draw row numbers on the left
    for (int i = 0; i < GRID_HEIGHT; i++) {
        mvprintw(i + 3, 1, "%d", (i + 1) % 10);
    }

    // Draw column numbers on the top
    for (int i = 0; i < GRID_WIDTH; i++) {
        mvprintw(1, i + 4, "%d", (i + 1) % 10);
    }

    // Draw border around the grid
    for (int y = 0; y <= GRID_HEIGHT; y++) {
        mvaddch(y + 2, 3, '|');
        mvaddch(y + 2, GRID_WIDTH + 4, '|');
    }
    for (int x = 3; x <= GRID_WIDTH + 4; x++) {
        mvaddch(2, x, '-');
        mvaddch(GRID_HEIGHT + 3, x, '-');
    }

    mvaddch(2, 3, '+');
    mvaddch(2, GRID_WIDTH + 4, '+');
    mvaddch(GRID_HEIGHT + 3, 3, '+');
    mvaddch(GRID_HEIGHT + 3, GRID_WIDTH + 4, '+');

    refresh();
}

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
 */
void update_grid() {
    // Clear the grid first
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        players_disp[i].active = 0;
    }

     // Parse the message line by line
    char* line = strtok(game_state_display, "\n");
    while (line != NULL) {
        if (line[0] == CMD_GAME_OVER) {
            game_over_display = 1; // Set a flag to indicate the game is over

        } else if (line[0] == CMD_PLAYER) {
            char id;
            int x, y;
            sscanf(line, "%*c %c %d %d", &id, &x, &y);
            
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = id;
                // Update player status
                int idx = id - 'A';
                if (idx >= 0 && idx < MAX_PLAYERS) {
                    players_disp[idx].id = id;
                    players_disp[idx].active = 1;
                }
            }
        } else if (line[0] == CMD_ALIEN) {
            // Format: ALIEN <x> <y>
            int x, y;
            sscanf(line, "%*c %d %d", &x, &y);
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = '*'; // Represent aliens with '*'
            }
        } else if (line[0] == CMD_LASER) {
            int x, y;
            int zone;
            sscanf(line, "%*c %d %d %d", &x, &y, &zone);
            
            if (zone == ZONE_A || zone == ZONE_H) {
                for (int i = x; i < GRID_WIDTH; i++) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                }
            } else if (zone == ZONE_D || zone == ZONE_F) {
                for (int i = x; i >= 0; i--) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                }
            }
            if (zone == ZONE_E || zone == ZONE_G) {
                for (int i = y; i < GRID_HEIGHT; i++) {
                    grid[i][x].ch = LASER_VERTICAL;
                }
            } else if (zone == ZONE_B || zone == ZONE_C) {
                for (int i = y; i >= 0; i--) {
                    grid[i][x].ch = LASER_VERTICAL;
                }
            }
            
        } else if (line[0] == CMD_SCORE) {
            char id;
            int player_score;
            sscanf(line, "%*c %c %d", &id, &player_score);
            
            int idx = id - 'A';
            if (idx >= 0 && idx < MAX_PLAYERS) {
                players_disp[idx].active = 1;
                players_disp[idx].score = player_score;
            }
        }
        // Move to the next line
        line = strtok(NULL, "\n");
    }
}

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
 */
void draw_scores() {
    // Clear the score area first
    for (int y = 3; y < GRID_HEIGHT + 3; y++) {
        for (int x = SCORE_START_X; x < SCORE_START_X + 20; x++) {
            mvaddch(y, x, ' ');
        }
    }

    // Draw scores header
    attron(A_BOLD);
    mvprintw(3, SCORE_START_X, "SCORES:");
    attroff(A_BOLD);

    // Track number of active players
    int active_players = 0;

    // Display scores for all active players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players_disp[i].active) {  // Only show active players
            attron(COLOR_PAIR(COLOR_ASTRONAUT));
            mvprintw(5 + active_players, SCORE_START_X, "Astronaut %c: %d", 
                    players_disp[i].id, players_disp[i].score);
            attroff(COLOR_PAIR(COLOR_ASTRONAUT));
            active_players++;
        }
    }

    // Draw border around scores
    int score_height = active_players + 4;  // Header + padding + scores
    
    // Vertical lines
    for (int y = 2; y < score_height + 3; y++) {
        mvaddch(y, SCORE_START_X - 2, '|');
        mvaddch(y, SCORE_START_X + 18, '|');
    }
    
    // Horizontal lines
    for (int x = SCORE_START_X - 2; x < SCORE_START_X + 19; x++) {
        mvaddch(2, x, '-');
        mvaddch(score_height + 3, x, '-');
    }
    
    // Corners
    mvaddch(2, SCORE_START_X - 2, '+');
    mvaddch(2, SCORE_START_X + 18, '+');
    mvaddch(score_height + 3, SCORE_START_X - 2, '+');
    mvaddch(score_height + 3, SCORE_START_X + 18, '+');
}


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
 */
void draw_grid(void) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            char ch = grid[y][x].ch;
            int display_y = y + 3;
            int display_x = x + 4;

            if (ch == '*') {
                // Draw alien
                attron(COLOR_PAIR(COLOR_ALIEN));
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_ALIEN));
            } else if (ch == LASER_HORIZONTAL || ch == LASER_VERTICAL) {
                // Draw laser
                attron(COLOR_PAIR(COLOR_LASER) | A_BOLD);
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_LASER) | A_BOLD);
            } else if (ch >= 'A' && ch <= 'H') {
                // Draw astronaut
                attron(COLOR_PAIR(COLOR_ASTRONAUT));
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_ASTRONAUT));
            } else {
                // Draw empty cell
                mvaddch(display_y, display_x, ' ');
            }
        }
    }

    // Draw scores
    draw_scores();

    // Refresh the screen
    refresh();
}

/**
 * @brief Displays the victory screen at the end of the game.
 *
 * This function clears the terminal screen and displays the victory screen,
 * which includes the game over message, the winner's information, and the 
 * final scores of all active players. It waits for user input before exiting.
 *
 * It is called when the game ends to show the final results to the players.
 */
void show_victory_screen() {
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);

    // Calculate center positions
    int center_y = term_height / 2;
    int center_x = term_width / 2;

    // Find the player with the highest score
    int max_score = -1;
    char winner_id = '\0';
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players_disp[i].active && players_disp[i].score > max_score) {
            max_score = players_disp[i].score;
            winner_id = players_disp[i].id;
        }
    }

    // Display victory message
    char title[] = "GAME OVER";
    attron(A_BOLD);
    mvprintw(center_y - 4, center_x - (int)(strlen(title) / 2), "%s", title);
    attroff(A_BOLD);

    // Display winner information
    char winner_msg[100];
    if (winner_id != '\0') {
        snprintf(winner_msg, sizeof(winner_msg), "Winner: Astronaut %c with %d points!", winner_id, max_score);
    } else {
        snprintf(winner_msg, sizeof(winner_msg), "No winner!");
    }
    mvprintw(center_y - 2, center_x - (int)(strlen(winner_msg) / 2), "%s", winner_msg);

    // Display scores of all players
    mvprintw(center_y, center_x - 7, "Final Scores:");
    int line = center_y + 1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players_disp[i].active) {
            char score_msg[50];
            snprintf(score_msg, sizeof(score_msg), "Astronaut %c: %d", players_disp[i].id, players_disp[i].score);
            mvprintw(line++, center_x - (int)(strlen(score_msg) / 2), "%s", score_msg);
        }
    }

    // Prompt to exit
    char exit_msg[] = "Press any key to exit...";
    mvprintw(line + 2, center_x - (int)(strlen(exit_msg) / 2), "%s", exit_msg);

    // Refresh to display changes
    refresh();
}

void set_display_game_state(char* buffer) {
    pthread_mutex_lock(&display_lock);
    strcpy(game_state_display, buffer);
    state_changed = 1;
    pthread_cond_signal(&state_changed_cond);
    pthread_mutex_unlock(&display_lock);
}

/**
 * @brief Main display function that initializes the display and handles the main display loop.
 *
 * This function initializes the display and enters the main loop where it draws the screen. 
 * The grid and display info should be modified in memory using the mutex in .h file. This function only draws the screen. External code must update the grid.
 * When the game is over, it shows the victory screen.
 *
 * @return Returns 0 if no error, -1 otherwise.
 */
int display_main() {
    // Initialize the display
    initialize_display();

    // Initialize Mutex
    if (pthread_mutex_init(&display_lock, NULL) != 0) {
        perror("Failed to initialize display_lock mutex");
        return -1;
    }
    if (pthread_cond_init(&state_changed_cond, NULL) != 0) {
        perror("Failed to initialize display_cond condition");
        return -1;
    }


    // Main loop
    while (!game_over_display) {
        // Read buffer data and update the grid
        pthread_mutex_lock(&display_lock);

        // Wait for the state to change, no active waiting
        while (!state_changed) {
            pthread_cond_wait(&state_changed_cond, &display_lock);
        }

        // Update the grid and draw it
        update_grid();
        draw_grid();

        state_changed = 0;
        pthread_mutex_unlock(&display_lock);
    }

    // Show victory screen if game_over flag is set
    show_victory_screen();

    // Destroy mutex
    // TODO: Some mutexes can be blocked so this will block not returning the function. Fix later
    //pthread_mutex_destroy(&display_lock);
    //pthread_cond_destroy(&state_changed_cond);

    return 0;
}