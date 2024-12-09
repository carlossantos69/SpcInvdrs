// This file is suposed to have all the code for the display


// game-server.c launches a process that runs this code
// outer-space-display.c runs this code (only one process maybe)

// game-server.c and outer-space-display.c need to include this

// This file need to include config.h to have acess to the config aswell
#include <ncurses.h>
#include <unistd.h>
#include <zmq.h>
#include "config.h"
#include "space-display.h"
#include <time.h>
#include <string.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 20

#define LASER_HORIZONTAL '-'
#define LASER_VERTICAL '|'
#define LASER_DISPLAY_TIME 500000 // 0.5 seconds in microseconds

#define DEBUG_LINE (GRID_HEIGHT + 5)  // Line to show debug messages

int game_over_display = 0;

// ZeroMQ socket
void* subscriber;

disp_Player_t players_disp[MAX_PLAYERS];  // Array to store players

// Game state representation
disp_Cell_t grid[GRID_HEIGHT][GRID_WIDTH];


void initialize_display() {
    // Initialize ncurses mode
    initscr();
    noecho();
    curs_set(FALSE); // Hide the cursor
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // Non-blocking input
    start_color();

    // Initialize color pairs
    init_pair(COLOR_ASTRONAUT, COLOR_GREEN, COLOR_BLACK);   // Astronauts
    init_pair(COLOR_ALIEN, COLOR_RED, COLOR_BLACK);         // Aliens
    init_pair(COLOR_LASER, COLOR_RED, COLOR_BLACK);      // Lasers

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

void update_grid(char* update_message) {
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
    char* line = strtok(update_message, "\n");
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
                    //mvprintw(DEBUG_LINE, 0, "Player %c active at index %d", id, idx);
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
            
            time_t now = time(NULL);
            
            if (zone == ZONE_A || zone == ZONE_H) {
                for (int i = x; i < GRID_WIDTH; i++) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                    grid[y][i].laser_time = now;
                }
            } else if (zone == ZONE_D || zone == ZONE_F) {
                for (int i = x; i >= 0; i--) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                    grid[y][i].laser_time = now;
                }
            }
            if (zone == ZONE_E || zone == ZONE_G) {
                for (int i = y; i < GRID_HEIGHT; i++) {
                    grid[i][x].ch = LASER_VERTICAL;
                    grid[i][x].laser_time = now;
                }
            } else if (zone == ZONE_B || zone == ZONE_C) {
                for (int i = y; i >= 0; i--) {
                    grid[i][x].ch = LASER_VERTICAL;
                    grid[i][x].laser_time = now;
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
                //mvprintw(DEBUG_LINE, 40, "Score update: Player %c = %d", id, player_score);
            }
        }
        // Move to the next line
        line = strtok(NULL, "\n");
    }
}

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


void draw_grid(void) {
    time_t now = time(NULL);
    double elapsed;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            char ch = grid[y][x].ch;
            int display_y = y + 3;
            int display_x = x + 4;

            // Clear lasers after LASER_DISPLAY_TIME
            if ((ch == LASER_HORIZONTAL || ch == LASER_VERTICAL) && grid[y][x].laser_time > 0) {
                elapsed = difftime(now, grid[y][x].laser_time);
                if (elapsed >= LASER_DISPLAY_TIME / 1000000.0) {
                    grid[y][x].ch = ' ';
                    grid[y][x].laser_time = 0;
                    mvaddch(display_y, display_x, ' ');
                    continue;
                }
            }

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

    // Wait for user input to exit
    nodelay(stdscr, FALSE);  // Make getch() blocking
    getch();
}

void display_main(void* sub) {
    subscriber = sub;

    // Initialize the display
    initialize_display();

    // Main loop
    while (!game_over_display) {
        char buffer[BUFFER_SIZE];
        int recv_size = zmq_recv(subscriber, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        if (recv_size != -1) {
            buffer[recv_size] = '\0';
            update_grid(buffer);
        } else {
            int err = zmq_errno();
            if (err == EAGAIN) {
                // No message received, continue
            } else if (err == ETERM || err == ENOTSOCK) {
                // The context was terminated or socket invalid, exit loop
                break;
            } else {
                // Other errors, handle or log as needed
                break;
            }
        }

        draw_grid();

        // Check for user input to exit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        usleep(50000); // 50ms delay
    }

    // Show victory screen if game_over flag is set
    if (game_over_display) {
        show_victory_screen();
    } else {
        // If game_over is not set, inform the user
        clear();
        mvprintw(GRID_HEIGHT / 2, (GRID_WIDTH / 2) - 12, "Connection lost. Press any key to exit...");
        refresh();
        getch();
    }
    
    endwin();
}