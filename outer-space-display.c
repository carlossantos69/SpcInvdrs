#include <ncurses.h>
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define BUFFER_SIZE 4096
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_PLAYERS 8

// Color pair definitions
#define COLOR_ASTRONAUT 1
#define COLOR_ALIEN 2
#define COLOR_LASER 3

#define SCORE_START_Y 5
#define SCORE_START_X (GRID_WIDTH + 6)
#define SCORE_PADDING 4  // Space between grid and scores


#define LASER_HORIZONTAL '-'
#define LASER_VERTICAL '|'
#define LASER_DISPLAY_TIME 500000 // 0.5 seconds in microseconds

#define DEBUG_LINE (GRID_HEIGHT + 5)  // Line to show debug messages


// ZeroMQ socket
void* context;
void* subscriber;

// Player structure
typedef struct {
    char player_id;
    int score;
    int active;  // flag to track if we've seen this player
} Player;

Player players[MAX_PLAYERS];  // Array to store players

// Game state representation
char grid[GRID_HEIGHT][GRID_WIDTH];

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
            grid[y][x] = ' ';
        }
    }

    // Initialize player scores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].player_id = '\0';
        players[i].score = 0;
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
            grid[y][x] = ' ';
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].active = 0;
    }

    move(DEBUG_LINE, 0);
    clrtoeol();

     // Parse the message line by line
    char* line = strtok(update_message, "\n");
    while (line != NULL) {
        if (strncmp(line, "PLAYER", 6) == 0) {
            char id;
            int x, y;
            sscanf(line, "PLAYER %c %d %d", &id, &x, &y);
            
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x] = id;
                // Update player status
                int idx = id - 'A';
                if (idx >= 0 && idx < MAX_PLAYERS) {
                    players[idx].player_id = id;
                    players[idx].active = 1;
                    //mvprintw(DEBUG_LINE, 0, "Player %c active at index %d", id, idx);
                }
            }
        } else if (strncmp(line, "ALIEN", 5) == 0) {
            // Format: ALIEN <x> <y>
            int x, y;
            sscanf(line, "ALIEN %d %d", &x, &y);
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x] = '*'; // Represent aliens with '*'
            }
        } else if (strncmp(line, "LASER", 5) == 0) {
            int x, y;
            char direction[10];
            sscanf(line, "LASER %d %d %s", &x, &y, direction);
            
            if (strcmp(direction, "HORIZONTAL") == 0) {
                // Draw horizontal laser
                for (int i = 0; i < GRID_WIDTH; i++) {
                    grid[y][i] = LASER_HORIZONTAL;
                }
            } else if (strcmp(direction, "VERTICAL") == 0) {
                // Draw vertical laser
                for (int i = 0; i < GRID_HEIGHT; i++) {
                    grid[i][x] = LASER_VERTICAL;
                }
            }
            printf("Drawing laser at (%d,%d) direction: %s\n", x, y, direction); // Debug
        } else if (strncmp(line, "SCORE", 5) == 0) {
            char id;
            int player_score;
            sscanf(line, "SCORE %c %d", &id, &player_score);
            
            int idx = id - 'A';
            if (idx >= 0 && idx < MAX_PLAYERS) {
                players[idx].score = player_score;
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
        if (players[i].active) {  // Only show active players
            attron(COLOR_PAIR(COLOR_ASTRONAUT));
            mvprintw(5 + active_players, SCORE_START_X, "Astronaut %c: %d", 
                    players[i].player_id, players[i].score);
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
    // Draw grid contents
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            char ch = grid[y][x];
            int display_y = y + 3;
            int display_x = x + 4;
            
            if (ch == '*') {
                attron(COLOR_PAIR(COLOR_ALIEN));
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_ALIEN));
            } else if (ch == LASER_HORIZONTAL || ch == LASER_VERTICAL) {
                attron(COLOR_PAIR(COLOR_LASER) | A_BOLD);
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_LASER) | A_BOLD);
            } else if (ch >= 'A' && ch <= 'H') {
                attron(COLOR_PAIR(COLOR_ASTRONAUT));
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_ASTRONAUT));
            } else {
                mvaddch(display_y, display_x, ' ');
            }
        }
    }

    // Draw scores
    draw_scores();

    // Refresh the screen
    refresh();

    //Handle laser timing
    int has_lasers = 0;
    for (int y = 0; y < GRID_HEIGHT && !has_lasers; y++) {
        for (int x = 0; x < GRID_WIDTH && !has_lasers; x++) {
            if (grid[y][x] == LASER_HORIZONTAL || grid[y][x] == LASER_VERTICAL) {
                has_lasers = 1;
            }
        }
    }

    if (has_lasers) {
        usleep(LASER_DISPLAY_TIME);
        
        // Clear lasers
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x] == LASER_HORIZONTAL || grid[y][x] == LASER_VERTICAL) {
                    grid[y][x] = ' ';
                    mvaddch(y + 3, x + 4, ' ');
                }
            }
        }
        refresh();
    }
}

void cleanup(void) {
    endwin();
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
}

int main() {
    // Initialize ZeroMQ
    context = zmq_ctx_new();
    subscriber = zmq_socket(context, ZMQ_SUB);
    
    // Connect to server's PUB socket
    if (zmq_connect(subscriber, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        return 1;
    }
    
    // Subscribe to all messages
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // Initialize the display
    initialize_display();

    // Main loop
    while (1) {
        char buffer[BUFFER_SIZE];
        int recv_size = zmq_recv(subscriber, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        if (recv_size != -1) {
            buffer[recv_size] = '\0';
            update_grid(buffer);
            draw_grid();
        }

        // Check for user input to exit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        //usleep(50000); // 50ms delay
    }

    // Clean up
    cleanup();
    return 0;
}