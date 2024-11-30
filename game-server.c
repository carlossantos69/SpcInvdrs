// game-server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>
#include "config.h"

void* context;
void* responder;  // For REQ/REP with astronauts
void* publisher;  // For PUB/SUB with display
time_t last_alien_move = 0;

#define BORDER_OFFSET 2      // Distance from edge for playable area
#define INNER_OFFSET 2       // Distance from player area to alien area
#define ALIEN_AREA_START 2   // Where aliens can start moving
#define ALIEN_AREA_END 17    // Where aliens must stop moving

#define MAX_PLAYERS 8
#define MAX_LASERS 8

typedef struct {
    char player_id;
    int x;
    int y;
    char region_id;
    int score;
    time_t last_fire_time;
} Player;

typedef struct {
    int x;
    int y;
    char direction[10];
    int active;
    char player_id;
    time_t creation_time; 
} Laser;

typedef struct {
    int x;
    int y;
    int active;
} Alien;

Player players[MAX_PLAYERS];
Laser lasers[MAX_PLAYERS]; // One laser per player
Alien aliens[MAX_ALIENS];

void initialize_game_state() {
    // Initialize players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].player_id = '\0';
        players[i].score = 0;
    }

    // Initialize aliens at random positions within the inner grid
    srand(time(NULL));
    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].x = 5 + rand() % (GRID_WIDTH - 10);
        aliens[i].y = 5 + rand() % (GRID_HEIGHT - 10);
        aliens[i].active = 1;
    }

    // Initialize lasers
    for (int i = 0; i < MAX_PLAYERS; i++) {
        lasers[i].active = 0;
    }
}

char assign_player_id() {
    // Check IDs sequentially from A to H
    for (char id = 'A'; id <= 'H'; id++) {
        int id_taken = 0;
        // Check if this ID is already taken
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].player_id == id) {
                id_taken = 1;
                break;
            }
        }
        // If ID is not taken, return it
        if (!id_taken) {
            return id;
        }
    }
    return '\0'; // No available slots
}

void send_game_state() {
    char message[BUFFER_SIZE] = "";
    char temp[100];

    // Add all active players to message
    // Add all active players and their positions
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].player_id != '\0') {
            snprintf(temp, sizeof(temp), "PLAYER %c %d %d\n", 
                    players[i].player_id, 
                    players[i].x, 
                    players[i].y);
            strcat(message, temp);
            
            // Add score information
            snprintf(temp, sizeof(temp), "SCORE %c %d\n",
                    players[i].player_id,
                    players[i].score);
            strcat(message, temp);
        }
    }

    // Add all active aliens
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].active) {
            snprintf(temp, sizeof(temp), "ALIEN %d %d\n",
                    aliens[i].x,
                    aliens[i].y);
            strcat(message, temp);
        }
    }

    // Add all active lasers
    for (int i = 0; i < MAX_LASERS; i++) {
        if (lasers[i].active) {
            snprintf(temp, sizeof(temp), "LASER %d %d %s\n",
                    lasers[i].x,
                    lasers[i].y,
                    lasers[i].direction);
            strcat(message, temp);
        }
    }

    // Send the message
    zmq_send(publisher, message, strlen(message), 0);
}

int is_valid_move(Player* player, const char* direction) {
    // Get current position
    int new_x = player->x;
    int new_y = player->y;
    
    // Calculate potential new position
    if (strcmp(direction, "LEFT") == 0) new_x--;
    else if (strcmp(direction, "RIGHT") == 0) new_x++;
    else if (strcmp(direction, "UP") == 0) new_y--;
    else if (strcmp(direction, "DOWN") == 0) new_y++;
    
    // Check movement based on region
    switch(player->region_id) {
        case 'A': // Left side vertical movement
            if (new_x != 0) return 0;  // Must stay in leftmost column
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case 'H': // Left side vertical movement (middle)
            if (new_x != 1) return 0;
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case 'D': // Right side vertical movement
            if (new_x != GRID_WIDTH - 2) return 0;  // Must stay in rightmost column
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case 'F': // Right side vertical movement (middle)
            if (new_x != GRID_WIDTH - 1) return 0;
            return (new_y >= BORDER_OFFSET && new_y <= GRID_HEIGHT - BORDER_OFFSET - 1);
            
        case 'E': // Top horizontal movement
            if (new_y != 0) return 0;  // Must stay in top row
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case 'G': // Top horizontal movement (right side)
            if (new_y != 1) return 0;
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case 'B': // Bottom horizontal movement (left side)
            if (new_y != GRID_HEIGHT - 2) return 0;  // Must stay in bottom row
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
            
        case 'C': // Bottom horizontal movement
            if (new_y != GRID_HEIGHT - 1) return 0;
            return (new_x >= BORDER_OFFSET && new_x <= GRID_WIDTH - BORDER_OFFSET - 1);
    }
    return 0;
}

void initialize_player_position(Player* player, char id) {
    switch(id) {
        case 'A': // First column (x=0)
            player->x = 0;
            player->y = 2; // Start in the first available position after corner
            break;
            
        case 'H': // Second column (x=1)
            player->x = 1;
            player->y = 2; // Start in the first available position after corner
            break;
            
        case 'G': // Second-to-last row (y=1)
            player->x = 2; // Start after corner
            player->y = 1;
            break;
            
        case 'E': // Last row (y=0)
            player->x = 2; // Start after corner
            player->y = 0;
            break;
            
        case 'D': // Second-to-last column (x=18)
            player->x = GRID_WIDTH - 2;
            player->y = 2; // Start after corner
            break;
            
        case 'F': // Last column (x=19)
            player->x = GRID_WIDTH - 1;
            player->y = 2; // Start after corner
            break;
            
        case 'C': // First row from bottom (y=19)
            player->x = 2; // Start after corner
            player->y = GRID_HEIGHT - 1;
            break;
            
        case 'B': // Second row from bottom (y=18)
            player->x = 2; // Start after corner
            player->y = GRID_HEIGHT - 2;
            break;
    }
    player->region_id = id;
    printf("Initialized player %c at position (%d, %d)\n", id, player->x, player->y);
}


void process_client_message(char* message, char* response) {
    printf("Received message: %s\n", message);
    if (strncmp(message, "CONNECT", 7) == 0) {
        char new_id = assign_player_id();
        if (new_id != '\0') {
        // Find empty slot and initialize player
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].player_id == '\0') {
                players[i].player_id = new_id;
                players[i].score = 0;
                players[i].last_fire_time = 0;
                
                // Initialize player position
                initialize_player_position(&players[i], new_id);
                
                // Send back player ID and region
                sprintf(response, "%c %c", new_id, players[i].region_id);
                
                printf("New player %c initialized at (%d,%d)\n", 
                       new_id, players[i].x+1, players[i].y+1);
                
                // Send updated game state to display
                send_game_state();
                break;
            }
        }
    } else {
            // Changed from "ERROR No available slots" to be more informative
            strcpy(response, "FULL Maximum number of players (8) reached");
    }
    }
    else if (strncmp(message, "MOVE", 4) == 0) {
        char player_id = message[5];
        char direction[10];
        sscanf(message + 7, "%s", direction);
        
        Player* player = NULL;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].player_id == player_id) {
                player = &players[i];
                break;
            }
        }
        
        if (player) {
            
            // Check if move is valid for player's region
            if (is_valid_move(player, direction)) {
                // Update position
                if (strcmp(direction, "LEFT") == 0) player->x--;
                else if (strcmp(direction, "RIGHT") == 0) player->x++;
                else if (strcmp(direction, "UP") == 0) player->y--;
                else if (strcmp(direction, "DOWN") == 0) player->y++;
                
                snprintf(response, BUFFER_SIZE, "OK %d", player->score);

            } else {
                snprintf(response, BUFFER_SIZE, "ERROR Invalid move %d", player->score);
            }
        } else {
            strcpy(response, "ERROR Player not found");
        }
    } else if (strncmp(message, "ZAP", 3) ==0) {
        char player_id = message[4];
        // Find the player
        Player* player = NULL;
        for (int i =0; i < MAX_PLAYERS; i++) {
            if (players[i].player_id == player_id) {
                player = &players[i];
                break;
            }
        }
        if (player) {
            time_t current_time = time(NULL);
            if (current_time - player->last_fire_time < LASER_COOLDOWN) {
                strcpy(response, "ERROR Laser cooldown");
                return;
            }
            player->last_fire_time = current_time;

            // Check if player already has an active laser
            int laser_index = player_id - 'A';
            if (lasers[laser_index].active) {
                strcpy(response, "ERROR Laser already active");
                return;
            }

            // Determine laser direction based on player's region
            if (player->player_id == 'A' || player->player_id == 'H' || player->player_id == 'D' || player->player_id == 'F') {
                strcpy(lasers[laser_index].direction, "HORIZONTAL");
            } else {
                strcpy(lasers[laser_index].direction, "VERTICAL");
            }

            // Initialize laser position
            lasers[laser_index].x = player->x;
            lasers[laser_index].y = player->y;
            lasers[laser_index].active = 1;
            lasers[laser_index].player_id = player_id;

            printf("Player %c fired a laser %s.\n", player_id, lasers[laser_index].direction);
            snprintf(response, BUFFER_SIZE, "OK %d", player->score);
        }
        else {
            snprintf(response, BUFFER_SIZE, "ERROR %s %d", "erro", player->score);
        }
    }
    else if (strncmp(message, "DISCONNECT", 10) ==0) {
        char player_id = message[11];
        // Find and remove the player
        for (int i =0; i < MAX_PLAYERS; i++) {
            if (players[i].player_id == player_id) {
                players[i].player_id = '\0';
                players[i].score =0;
                lasers[i].active =0;
                printf("Player %c disconnected.\n", player_id);
                break;
            }
        }
        strcpy(response, "OK");
    }
    else {
        strcpy(response, "ERROR Unknown command");
    }
}

void check_laser_collisions() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (lasers[i].active) {
            Laser* laser = &lasers[i];
            // Traverse the grid based on laser direction
            if (strcmp(laser->direction, "HORIZONTAL") == 0) {
                // Send the full laser line information
                for (int x = 0; x < GRID_WIDTH; x++) {
                    // Check for aliens in this line
                    for (int a = 0; a < MAX_ALIENS; a++) {
                        if (aliens[a].active && aliens[a].x == x && aliens[a].y == laser->y) {
                            aliens[a].active = 0;
                            // Update player score
                            for (int p = 0; p < MAX_PLAYERS; p++) {
                                if (players[p].player_id == laser->player_id) {
                                    players[p].score += 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            } else if (strcmp(laser->direction, "VERTICAL") == 0) {
                // Vertical laser traverses full height
                for (int y = 0; y < GRID_HEIGHT; y++) {
                    // Check for aliens in this line
                    for (int a = 0; a < MAX_ALIENS; a++) {
                        if (aliens[a].active && aliens[a].x == laser->x && aliens[a].y == y) {
                            aliens[a].active = 0;
                            // Update player score
                            for (int p = 0; p < MAX_PLAYERS; p++) {
                                if (players[p].player_id == laser->player_id) {
                                    players[p].score += 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

        }
    }
}

void update_alien_positions() {
    time_t current_time = time(NULL);
    
    // Only move aliens if 1 second has passed since last move
    if (current_time - last_alien_move >= 1) {
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

// Add near other function declarations
void update_game_state() {
    time_t current_time = time(NULL);
    
    // Update alien positions (existing functionality)
    update_alien_positions();
    
    // Check laser collisions and update scores
    check_laser_collisions();
    
    // Deactivate lasers after 0.5 seconds
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (lasers[i].active && (current_time - lasers[i].creation_time >= 1)) {
            lasers[i].active = 0;
        }
    }
}

int main() {
    initialize_game_state();
    
    // Initialize ZeroMQ context
    context = zmq_ctx_new();
    
    // Set up REQ/REP socket for astronaut clients
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, SERVER_ENDPOINT_REQ);
    
    // Set up PUB socket for display client
    publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, SERVER_ENDPOINT_PUB);
    
    while (1) {
        char buffer[BUFFER_SIZE];
        // Handle astronaut messages (REQ/REP)
        int recv_size = zmq_recv(responder, buffer, BUFFER_SIZE - 1, ZMQ_DONTWAIT);
        if (recv_size != -1) {
            buffer[recv_size] = '\0';
            char response[BUFFER_SIZE];
            process_client_message(buffer, response);
            zmq_send(responder, response, strlen(response), 0);
        }
        
        // Update game state
        update_game_state();
        
        // Send updated state to display
        send_game_state();
        
        usleep(50000); // 50ms delay
    }
    
    zmq_close(responder);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 0;
}