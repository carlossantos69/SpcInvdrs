#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>
#include <ctype.h>
#include "config.h"

void* context;
void* responder;  // For REQ/REP with astronauts
void* publisher;  // For PUB/SUB with display
time_t last_alien_move = 0;

#define MAX_PLAYERS 8
#define BORDER_OFFSET 2      // Distance from edge for playable area
#define INNER_OFFSET 2       // Distance from player area to alien area
#define ALIEN_AREA_START 2   // Where aliens can start moving
#define ALIEN_AREA_END 17    // Where aliens must stop moving

int game_over = 0;

typedef struct {
    int x;
    int y;
    int active;
    time_t creation_time; 
} Laser_t;

typedef struct {
    char id;
    int zone;
    int x;
    int y;
    int score;
    time_t last_fire_time;
    time_t last_stun_time;
    char session_token[33]; // 32-char hex token + null terminator
    Laser_t laser; //The laser of the player 
} Player_t;

typedef struct {
    int x;
    int y;
    int active;
} Alien_t;

typedef struct {
    char ch;            // Character to display at this cell
    time_t laser_time;  // Timestamp when the laser was drawn
} Cell_t;

// Game state representation
Cell_t grid[GRID_HEIGHT][GRID_WIDTH];
Player_t players[MAX_PLAYERS];
Alien_t aliens[MAX_ALIENS];

// Auxiliary functions to find players by ID or session token
Player_t* find_by_id(const char id) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id == id) {
            return &players[i];
        } 
    }
    return NULL;
}
Player_t* find_by_session_token(const char* session_token) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (strcmp(players[i].session_token, session_token) == 0) {
            return &players[i];
        }
    }
    return NULL;
}
Player_t* find_by_zone(const char zone) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].zone == zone) {
            return &players[i];
        }
    }
    return NULL;
}

// Generate a random session token
void generate_session_token(char* token) {
    const char charset[] = "abcdef0123456789";
    size_t length = 33;
    for (size_t i = 0; i < length - 1; i++) {
        size_t index = rand() % (sizeof(charset) - 1);
        token[i] = charset[index];
    }
    token[length - 1] = '\0';
}

// Randomly assign an ID to a player
char assign_player_id() {
    char ids[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    int available_ids[MAX_PLAYERS];
    int count = 0;

    // Collect all available IDs
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (find_by_id(ids[i]) == NULL) {
            available_ids[count++] = ids[i];
        }
    }

    // If no IDs are available, return NULL
    if (count == 0) {
        return '\0';
    }

    // Select a random available ID
    int random_index = rand() % count;
    return available_ids[random_index];
}


// Return a random zone to put the player
int get_random_zone() {
    int zones[] = {ZONE_A, ZONE_B, ZONE_C, ZONE_D, ZONE_E, ZONE_F, ZONE_G, ZONE_H};
    int random_zone;
    while (true) {
        random_zone = zones[rand() % 8]; // There are 8 zones
        if (find_by_zone(random_zone) == NULL) {
            return random_zone;
        }
    }
}

void clear_player(Player_t *player) {
    if (player == NULL) return;

    player->id = '\0';
    player->score = 0;
    player->last_fire_time = 0;
    player->last_stun_time = 0;
    player->session_token[0] = '\0';
    player->laser.active = 0; 
}


int all_aliens_destroyed() {
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].active) {
            return 0; // There is at least one alien still active
        }
    }
    return 1; // All aliens are inactive
}


void update_game_state();

void initialize_game_state() {
    // Initialize players
    srand(time(NULL));

    for (int i = 0; i < MAX_PLAYERS; i++) {
        clear_player(&players[i]);
    }

    // Initialize aliens at random positions within the inner grid
    srand(time(NULL));
    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].x = 5 + rand() % (GRID_WIDTH - 10);
        aliens[i].y = 5 + rand() % (GRID_HEIGHT - 10);
        aliens[i].active = 1;
    }

    // Initialize grid to empty spaces
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
            grid[y][x].laser_time = 0;
        }
    }
}

void clear_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
        }
    }
}

void update_grid() {
    clear_grid();

    // Place aliens on the grid
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].active) {
            int x = aliens[i].x;
            int y = aliens[i].y;
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = '*'; // Represent aliens with '*'
            }
        }
    }

    // Place players and lasers on the grid
    time_t now = time(NULL);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id != '\0') {
            // Place player on the grid
            int x = players[i].x;
            int y = players[i].y;
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = players[i].id;
            }

            // Place laser on the grid
            if (players[i].laser.active) {
                Laser_t* laser = &players[i].laser;
                int y = laser->y;
                int x = laser->x;
                if (players[i].zone == ZONE_A || players[i].zone == ZONE_H) {
                    for (int x = laser->x; x < GRID_WIDTH; x++) {
                        grid[y][x].ch = '-';
                        grid[y][x].laser_time = now;
                    }
                } else if (players[i].zone == ZONE_D || players[i].zone == ZONE_F) {
                    for (int x = laser->x; x >= 0; x--) {
                        grid[y][x].ch = '-';
                        grid[y][x].laser_time = now;
                    }
                }
                
                    
                if (players[i].zone == ZONE_E || players[i].zone == ZONE_G) {
                    for (int y = laser->y; y < GRID_HEIGHT; y++) {
                        grid[y][x].ch = '|';
                        grid[y][x].laser_time = now;
                    }
                } else if (players[i].zone == ZONE_B || players[i].zone == ZONE_C) {
                    for (int y = laser->y; y >= 0; y--) {
                        grid[y][x].ch = '|';
                        grid[y][x].laser_time = now;
                    }
                }
            }

        }
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

    int line = 5;

    // Display scores for all active players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id != '\0') {
            attron(COLOR_PAIR(COLOR_ASTRONAUT));
            mvprintw(line++, SCORE_START_X, "Astronaut %c: %d", 
                     players[i].id, players[i].score);
            attroff(COLOR_PAIR(COLOR_ASTRONAUT));
        }
    }
}

// Draw the grid with borders and coordinates
void draw_grid() {
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

    // Draw the grid contents
    time_t now = time(NULL);
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
            } else if (ch == '-' || ch == '|') {
                // Draw laser
                attron(COLOR_PAIR(COLOR_LASER) | A_BOLD);
                mvaddch(display_y, display_x, ch);
                attroff(COLOR_PAIR(COLOR_LASER) | A_BOLD);
                // Remove laser after a short time
                if (difftime(now, grid[y][x].laser_time) > 0.5) {
                    grid[y][x].ch = ' ';
                }
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

void send_game_state() {
    char message[BUFFER_SIZE] = "";
    char temp[100];

    // Add all active players to message
    // Add all active players and their positions
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id != '\0') {
            // Add player information
            snprintf(temp, sizeof(temp), "%c %c %d %d\n",
                    CMD_PLAYER,
                    players[i].id, 
                    players[i].x, 
                    players[i].y);
            strcat(message, temp);
            
            // Add score information
            snprintf(temp, sizeof(temp), "%c %c %d\n",
                    CMD_SCORE,
                    players[i].id,
                    players[i].score);
            strcat(message, temp);

            // Add laser information
            if (players[i].laser.active) {
                snprintf(temp, sizeof(temp), "%c %d %d %d\n",
                        CMD_LASER,
                        players[i].laser.x,
                        players[i].laser.y,
                        players[i].zone);
                strcat(message, temp);
            }

        }
    }

    // Add all active aliens
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].active) {
            snprintf(temp, sizeof(temp), "%c %d %d\n",
                    CMD_ALIEN,
                    aliens[i].x,
                    aliens[i].y);
            strcat(message, temp);
        }
    }

    // Send the message
    zmq_send(publisher, message, strlen(message), 0);
}


// Checks if movement is valid based on player zone
int is_valid_move(Player_t* player, const char direction) {
    // Get current position
    int new_x = player->x;
    int new_y = player->y;
    
    // Calculate potential new position
    if (direction == MOVE_LEFT) new_x--;
    else if (direction == MOVE_RIGHT) new_x++;
    else if (direction == MOVE_UP) new_y--;
    else if (direction == MOVE_DOWN) new_y++;
    
    // Check movement based on id
    switch(player->zone) {
        case ZONE_A: // Left side vertical movement
            if (new_x != 0) return 0;  // Must stay in leftmost column
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case ZONE_H: // Left side vertical movement (middle)
            if (new_x != 1) return 0;
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case ZONE_D: // Right side vertical movement
            if (new_x != GRID_WIDTH - 2) return 0;  // Must stay in rightmost column
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case ZONE_F: // Right side vertical movement (middle)
            if (new_x != GRID_WIDTH - 1) return 0;
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case ZONE_E: // Top horizontal movement
            if (new_y != 0) return 0;  // Must stay in top row
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case ZONE_G: // Top horizontal movement (right side)
            if (new_y != 1) return 0;
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case ZONE_B: // Bottom horizontal movement (left side)
            if (new_y != GRID_HEIGHT - 2) return 0;  // Must stay in bottom row
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case ZONE_C: // Bottom horizontal movement
            if (new_y != GRID_HEIGHT - 1) return 0;
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
    }
    return 0;
}

// TODO: Make random spawn position inside of availabe row/collum
void initialize_player_position(Player_t* player) {
    switch(player->zone) {
        case ZONE_A: // First column (x=0)
            player->x = 0;
            player->y = 2; // Start in the first available position after corner
            break;
            
        case ZONE_H: // Second column (x=1)
            player->x = 1;
            player->y = 2; // Start in the first available position after corner
            break;
            
        case ZONE_G: // Second-to-last row (y=1)
            player->x = 2; // Start after corner
            player->y = 1;
            break;
            
        case ZONE_E: // Last row (y=0)
            player->x = 2; // Start after corner
            player->y = 0;
            break;
            
        case ZONE_D: // Second-to-last column (x=18)
            player->x = GRID_WIDTH - 2;
            player->y = 2; // Start after corner
            break;
            
        case ZONE_F: // Last column (x=19)
            player->x = GRID_WIDTH - 1;
            player->y = 2; // Start after corner
            break;
            
        case ZONE_C: // First row from bottom (y=19)
            player->x = 2; // Start after corner
            player->y = GRID_HEIGHT - 1;
            break;
            
        case ZONE_B: // Second row from bottom (y=18)
            player->x = 2; // Start after corner
            player->y = GRID_HEIGHT - 2;
            break;
    }
    //printf("Initialized player %c at position (%d, %d)\n", id, player->x, player->y);
}


void process_client_message(char* message, char* response) {
    //printf("Received message: %s\n", message);
    if (message[0] == CMD_CONNECT) {
        char new_id = assign_player_id();
        if (new_id != '\0') {
            // Find available player id and initialize a new player
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (players[i].id == '\0') {
                    clear_player(&players[i]); // Probably redundant
                    players[i].id = new_id;
                    generate_session_token(players[i].session_token);
                    players[i].zone = get_random_zone();
                    initialize_player_position(&players[i]);
                    sprintf(response, "%c %s", new_id, players[i].session_token);
                    //printf("New player %c initialized at (%d,%d) with session token %s\n",new_id, players[i].x, players[i].y, players[i].session_token);

                    send_game_state();
                    return;
                }
            }
        }
        strcpy(response, "FULL Maximum number of players (8) reached");
        return;
    }

    // Validate session token and player ID
    char cmd;
    char player_id;
    char session_token[33];
    int num_parsed = sscanf(message, "%c %c %32s", &cmd, &player_id, session_token);

    if (num_parsed < 3) {
        strcpy(response, "ERROR Missing session token");
        return;
    }

    // Validate the command string (allowed commands: CONNECT, MOVE, ZAP, DISCONNECT)
    if (cmd != CMD_MOVE && cmd != MSG_ZAP && cmd != CMD_DISCONNECT) {
        strncpy(response, "ERROR Unknown command", BUFFER_SIZE - 1);
        response[BUFFER_SIZE - 1] = '\0';
        return;
    }

    // Validate player_id (should be between 'A' and 'H')
    if (player_id < 'A' || player_id > 'H') {
        strncpy(response, "ERROR Invalid player ID", BUFFER_SIZE - 1);
        response[BUFFER_SIZE - 1] = '\0';
        return;
    }

    // Validate session token characters (should be hexadecimal)
    for (int i = 0; i < 32; i++) {
        if (!isxdigit(session_token[i])) {
            strncpy(response, "ERROR Invalid session token characters", BUFFER_SIZE - 1);
            response[BUFFER_SIZE - 1] = '\0';
            return;
        }
    }

    // Validate session_token length (should be exactly 32 characters)
    if (strlen(session_token) != 32) {
        strncpy(response, "ERROR Invalid session token length", BUFFER_SIZE - 1);
        response[BUFFER_SIZE - 1] = '\0';
        return;
    }

    // Find the player based on the provided player ID and session token
    Player_t* player = find_by_id(player_id);
    if (!player) {
        strncpy(response, "ERROR Invalid player ID or session token", BUFFER_SIZE - 1);
        response[BUFFER_SIZE - 1] = '\0';
        return;
    } 
    if (strcmp(player->session_token, session_token) != 0) {
        strncpy(response, "ERROR Invalid player ID or session token", BUFFER_SIZE - 1);
        response[BUFFER_SIZE - 1] = '\0';
        return;
    }


    // Command handling with checks
    if (cmd == CMD_MOVE) {
        time_t current_time = time(NULL);
        char direction;
        if (sscanf(message, "%*s %*c %*s %c", &direction) != 1) {
            strcpy(response, "ERROR Invalid MOVE command format");
            return;
        }

        if (difftime(current_time, player->last_stun_time) < STUN_DURATION) {
            strcpy(response, "ERROR Player stunned");
            return;
        }

        // Validate direction
        if (direction != MOVE_UP && direction != MOVE_DOWN && direction != MOVE_LEFT && direction != MOVE_RIGHT) {
            strncpy(response, "ERROR Invalid direction", BUFFER_SIZE - 1);
            response[BUFFER_SIZE - 1] = '\0';
            return;
        }

        if (is_valid_move(player, direction)) {
            if (direction ==MOVE_LEFT) player->x--;
            else if (direction == MOVE_RIGHT) player->x++;
            else if (direction == MOVE_UP) player->y--;
            else if (direction == MOVE_DOWN) player->y++;
            snprintf(response, BUFFER_SIZE, "OK %d", player->score);
        } else {
            strcpy(response, "ERROR Invalid move");
        }
    } else if (cmd == MSG_ZAP)  {
        time_t current_time = time(NULL);
        if (difftime(current_time, player->last_fire_time) < LASER_COOLDOWN) {
            strcpy(response, "ERROR Laser cooldown");
            return;
        }
        if (difftime(current_time, player->last_stun_time) < STUN_DURATION) {
            strcpy(response, "ERROR Player stunned");
            return;
        }
        player->last_fire_time = current_time;
        if (player->laser.active) {
            strcpy(response, "ERROR Laser already active");
            return;
        }

        // Determine laser direction based on player's id
        if (player->zone == ZONE_A || player->zone == ZONE_H) {
            player->laser.x = player->x + 1; // Start right of player
            player->laser.y = player->y;
            //printf("Player %c fired a laser from %d, %d\n", player_id, player->laser.x, player->laser.y);
        } else if (player->zone == ZONE_D || player->zone == ZONE_F) {
            player->laser.x = player->x - 1;  // Start left of player
            player->laser.y = player->y;
        } 
        else if (player->zone == ZONE_B || player->zone == ZONE_C) {
            player->laser.y = player->y - 1;
            player->laser.x = player->x;
        } else if (player->zone == ZONE_E || player->zone == ZONE_G) {
            player->laser.y = player->y + 1;
            player->laser.x = player->x;
        }
        

        // Initialize laser position
        player->laser.active = 1;
        player->laser.creation_time = current_time;

        // Update game state
        update_game_state();

        // Update the grid based on the current game state
        update_grid();

        // Draw the updated grid
        draw_grid();

        // Send updated state to display
        send_game_state();

        //printf("Player %c fired a laser %s.\n", player_id, player->laser.direction);
        snprintf(response, BUFFER_SIZE, "OK %d", player->score);
    } else if (cmd == CMD_DISCONNECT)  {
        clear_player(player);
        //printf("Player %c disconnected.\n", player->player_id);
        strcpy(response, "OK");
    } else {
        strcpy(response, "ERROR Unknown command");
    }
}



void check_laser_collisions() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].laser.active) {
            Laser_t* laser = &players[i].laser;
            // Traverse the grid based on laser direction and player position
            if (players[i].zone == ZONE_A || players[i].zone == ZONE_H || players[i].zone == ZONE_D || players[i].zone == ZONE_F) {

                // Check collision with alliens
                for (int j = 0; j < MAX_ALIENS; j++) {
                    if (aliens[j].active && aliens[j].y == laser->y) {
                        // Destroy allien and update player score
                        aliens[j].active = 0;
                        players[i].score += KILL_POINTS;
                    }
                }
                // Check colision with player
                for (int j = 0; j < MAX_PLAYERS; j++) {
                    if (players[j].id == players[i].id) continue;
                    if (players[j].zone == ZONE_A && players[i].zone == ZONE_H) continue; //Player is behind laser
                    if (players[j].zone == ZONE_F && players[i].zone == ZONE_D) continue; //Player is behind laser
                    if (players[j].y == laser->y) {
                        players[j].last_stun_time = time(NULL);
                    }
                }
            } else {
                // Check collision with alliens
                for (int j = 0; j < MAX_ALIENS; j++) {
                    if (aliens[j].active && aliens[j].x == laser->x) {
                        // Destroy allien and update player score
                        aliens[j].active = 0;
                        players[i].score += KILL_POINTS;
                    }
                }
                // Check colision with player
                for (int j = 0; j < MAX_PLAYERS; j++) {
                    if (players[j].id == players[i].id) continue;
                    if (players[j].zone == ZONE_E && players[i].zone == ZONE_G) continue; //Player is behind laser
                    if (players[j].zone == ZONE_C && players[i].zone == ZONE_B) continue; //Player is behind laser
                    if (players[j].x == laser->x) {
                        players[j].last_stun_time = time(NULL);
                    }
                }
            }
        }
    }
}


//TODO: CHECK IF OTHER ALLIEN IS IN NEW POSITION
void update_alien_positions() {
    time_t current_time = time(NULL);
    
    // Only move aliens if 1 second has passed since last move
    if (current_time - last_alien_move >= ALIEN_MOVE_INTERVAL) {
        for (int i = 0; i < MAX_ALIENS; i++) {
            if (aliens[i].active) {
                int direction = rand() % 4;
                int new_x = aliens[i].x;
                int new_y = aliens[i].y;
                
                switch (direction) {
                    case 0: // Move up
                        new_y--;
                        break;
                    case 1: // Move down
                        new_y++;
                        break;
                    case 2: // Move left
                        new_x--;
                        break;
                    case 3: // Move right
                        new_x++;
                        break;
                }
                
                // Check if new position is within alien area
                if (new_x >= ALIEN_AREA_START && new_x <= ALIEN_AREA_END &&
                    new_y >= ALIEN_AREA_START && new_y <= ALIEN_AREA_END) {
                    aliens[i].x = new_x;
                    aliens[i].y = new_y;
                }
            }
        }
        // Update the last move time
        last_alien_move = current_time;
    }
}

void update_game_state() {
    time_t current_time = time(NULL);
    
    // Update alien positions
    update_alien_positions();
    
    // Check laser collisions and update scores
    check_laser_collisions();
    
    // Deactivate lasers after 0.5 seconds
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (players[i].laser.active && difftime(current_time, players[i].laser.creation_time) >= 0.5) {
            players[i].laser.active = 0;
        }
    }

    // Check if all aliens are destroyed
    if (all_aliens_destroyed()) {
        game_over = 1; // Set a game over flag
    }
}

void send_game_over_state() {
    char message[BUFFER_SIZE] = "";
    char temp[100];

    // Include game over command
    temp[0] = CMD_GAME_OVER;
    temp[1] = '\n';
    
    //snprintf(temp, sizeof(temp), "%c\n", MSG_GAME_OVER);
    strcat(message, temp);

    // Send final scores of all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id != '\0') {
            snprintf(temp, sizeof(temp), "%c %c %d\n",
                    CMD_SCORE,
                    players[i].id,
                    players[i].score);
            strcat(message, temp);
        }
    }
    

    // Send the message
    zmq_send(publisher, message, strlen(message), 0);

    // Give the display client time to process the message before terminating
    sleep(1);
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
        if (players[i].id != '\0' && players[i].score > max_score) {
            max_score = players[i].score;
            winner_id = players[i].id;
        }
    }

    // Display victory message
    attron(A_BOLD);
    mvprintw(center_y - 4, center_x - 5, "GAME OVER");
    attroff(A_BOLD);

    // Display winner information
    if (winner_id != '\0') {
        mvprintw(center_y - 2, center_x - 15, "Winner: Astronaut %c with %d points!", winner_id, max_score);
    } else {
        mvprintw(center_y - 2, center_x - 5, "No winner!");
    }

    // Display scores of all players
    mvprintw(center_y, center_x - 7, "Final Scores:");
    int line = center_y + 1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id != '\0') {
            mvprintw(line++, center_x - 10, "Astronaut %c: %d", players[i].id, players[i].score);
        }
    }

    // Prompt to exit
    mvprintw(line + 2, center_x - 10, "Press any key to exit...");

    // Refresh to display changes
    refresh();

    // Wait for user input to exit
    nodelay(stdscr, FALSE);  // Make getch() blocking
    getch();
}

void cleanup() {
    endwin(); // Restore normal terminal behavior
    // Close ZeroMQ sockets if necessary
    zmq_close(responder);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
}

int main() {
    initialize_game_state();

    // Initialize ncurses
    initscr();
    noecho();
    curs_set(FALSE); // Hide the cursor
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // Non-blocking input
    start_color();

    // Initialize color pairs (define these constants as needed)
    init_pair(COLOR_ASTRONAUT, COLOR_GREEN, COLOR_BLACK);   // Astronauts
    init_pair(COLOR_ALIEN, COLOR_RED, COLOR_BLACK);         // Aliens
    init_pair(COLOR_LASER, COLOR_YELLOW, COLOR_BLACK);      // Lasers
    
    // Initialize ZeroMQ context
    context = zmq_ctx_new();
    
    // Set up REQ/REP socket for astronaut clients
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, SERVER_ENDPOINT_REQ);
    
    // Set up PUB socket for display client
    publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, SERVER_ENDPOINT_PUB);
    
    while (!game_over) {
        // Define safer buffer sizes
        char buffer[BUFFER_SIZE] = {0};

        // Receive messages with a maximum limit
        int recv_size = zmq_recv(responder, buffer, BUFFER_SIZE - 1, ZMQ_DONTWAIT);
        if (recv_size > 0 && recv_size < BUFFER_SIZE) {
            buffer[recv_size] = '\0';
            char response[BUFFER_SIZE];
            process_client_message(buffer, response);
            zmq_send(responder, response, strlen(response), 0);
        } else if (recv_size >= BUFFER_SIZE) {
            // Message too long, possible overflow attempt
            char response[] = "ERROR Message too long";
            zmq_send(responder, response, strlen(response), 0);
        }
        
        // Update game state
        update_game_state();

        // Update the grid based on the current game state
        update_grid();

        // Draw the updated grid
        draw_grid();

        // Send updated state to display
        send_game_state();
        
        usleep(50000); // 50ms delay
    }

    send_game_over_state();

    // Show game over screen
    show_victory_screen();
    
    zmq_close(responder);
    zmq_close(publisher);
    zmq_ctx_destroy(context);   // Shutdown context
    zmq_ctx_term(context);     // Terminate context
    cleanup();
    return 0;
}