/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: game-logic.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Description:
 * Handles the game logic for the server. Updates aliens, receives and process client messages and publishes updates to displays
 */

#include "game-logic.h"
#include "config.h"
#include <stdio.h>

void* responder;  // For REQ/REP with astronauts
void* publisher;  // For PUB/SUB with display

// Game state representation
Player_t players[MAX_PLAYERS];
Alien_t aliens[MAX_ALIENS];

// Indicates whether the game is over (1) or not (0)
int game_over = 0;

// Stores the timestamp of the last alien update
time_t last_alien_move = 0;


/**
 * @brief Finds a player by their ID.
 *
 * This function searches through the list of players and returns a pointer
 * to the player with the specified ID.
 *
 * @param id The ID of the player to find.
 * @return A pointer to the player with the specified ID, or NULL if no player
 *         with the given ID is found.
 */
Player_t* find_by_id(const char id) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].id == id) {
            return &players[i];
        } 
    }
    return NULL;
}


/**
 * @brief Finds a player by their session token.
 *
 * This function iterates through the list of players and compares each player's
 * session token with the provided session token. If a match is found, a pointer
 * to the corresponding player is returned.
 *
 * @param session_token The session token to search for.
 * @return A pointer to the player with the matching session token, or NULL if no match is found.
 */
Player_t* find_by_session_token(const char* session_token) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (strcmp(players[i].session_token, session_token) == 0) {
            return &players[i];
        }
    }
    return NULL;
}


/**
 * @brief Finds a player by their zone.
 *
 * This function searches through the list of players and returns a pointer
 * to the player whose zone matches the specified zone.
 *
 * @param zone The zone to search for.
 * @return A pointer to the player in the specified zone, or NULL if no player is found.
 */
Player_t* find_by_zone(const char zone) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].zone == zone) {
            return &players[i];
        }
    }
    return NULL;
}

/**
 * @brief Generates a random session token.
 *
 * This function generates a random session token consisting of 
 * hexadecimal characters (0-9, a-f). The generated token will be 
 * 32 characters long, followed by a null terminator.
 *
 * @param token A pointer to a character array where the generated 
 * token will be stored. The array must be at least 33 characters long.
 */
void generate_session_token(char* token) {
    const char charset[] = "abcdef0123456789";
    size_t length = 33;
    for (size_t i = 0; i < length - 1; i++) {
        size_t index = rand() % (sizeof(charset) - 1);
        token[i] = charset[index];
    }
    token[length - 1] = '\0';
}


/**
 * @brief Assigns a unique player ID from a predefined set of IDs.
 *
 * This function assigns a unique player ID from the set {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'}.
 * It first collects all available IDs by checking which IDs are not currently in use.
 * If no IDs are available, it returns '\0'.
 * Otherwise, it selects a random available ID and returns it.
 *
 * @return A unique player ID if available, otherwise '\0'.
 */
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


/**
 * @brief Selects a random zone that is not currently occupied.
 *
 * This function generates a random zone from a predefined list of zones
 * (ZONE_A to ZONE_H). It continues to generate a random zone until it finds
 * one that is not currently occupied, as determined by the `find_by_zone`
 * function.
 *
 * @return An integer representing the randomly selected unoccupied zone.
 */
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

/**
 * @brief Clears the player's data by resetting its fields to default values.
 *
 * This function sets the player's ID to '\0', score to 0, last fire time to 0,
 * last stun time to 0, session token to an empty string, and deactivates the laser.
 *
 * @param player Pointer to the Player_t structure to be cleared. If the pointer
 *               is NULL, the function returns immediately.
 */
void clear_player(Player_t *player) {
    if (player == NULL) return;

    player->id = '\0';
    player->score = 0;
    player->last_fire_time = 0;
    player->last_stun_time = 0;
    player->session_token[0] = '\0';
    player->laser.active = 0; 
}


/**
 * @brief Checks if all aliens have been destroyed.
 *
 * This function iterates through the array of aliens and checks if any of them are still active.
 * If at least one alien is active, the function returns 0. If all aliens are inactive, it returns 1.
 *
 * @return int 1 if all aliens are inactive, 0 if at least one alien is still active.
 */
int all_aliens_destroyed() {
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].active) {
            return 0; // There is at least one alien still active
        }
    }
    return 1; // All aliens are inactive
}

/**
 * @brief Initializes the game state by setting up players and aliens.
 *
 * This function initializes the game state by clearing player data and
 * placing aliens at random positions within the inner grid. It uses the
 * current time as the seed for the random number generator to ensure
 * different positions for aliens in each game session.
 */
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
}

/**
 * @brief Sends the current game state to all subscribers.
 *
 * This function constructs a message containing the state of all active players,
 * their positions, scores, and laser statuses, as well as the positions of all
 * active aliens. The message is then sent to display subscribers.
 *
 * The message format includes:
 * - Player information: CMD_PLAYER, player ID, x position, y position
 * - Score information: CMD_SCORE, player ID, score
 * - Laser information (if active): CMD_LASER, x position, y position, zone
 * - Alien information: CMD_ALIEN, x position, y position
 *
 * The function iterates through all players and aliens, adding their information
 * to the message if they are active.
 *
 * @note The function assumes that the players and aliens arrays, as well as the
 *       publisher socket, are properly initialized and accessible.
 */
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


/**
 * @brief Checks if the player's move in the specified direction is valid.
 *
 * This function calculates the potential new position of the player based on the
 * given direction and checks if the move is valid within the constraints of the
 * player's current zone.
 *
 * @param player Pointer to the Player_t structure representing the player.
 * @param direction Character representing the direction of the move (MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN).
 * @return int Returns 1 if the move is valid, 0 otherwise.
 */
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

/**
 * @brief Initializes the player's position based on their starting zone.
 *
 * This function sets the initial x and y coordinates of a player based on the
 * zone they are assigned to. Each zone corresponds to a specific starting 
 * position on the grid.
 *
 * @param player A pointer to the Player_t structure whose position is to be initialized.
 */
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
}


/**
 * @brief Processes a message received from a client and generates an appropriate response.
 *
 * This function handles various commands sent by clients, including connecting to the game,
 * moving a player, firing a laser, and disconnecting from the game. It validates the message,
 * checks the session token, and performs the requested action if all validations pass.
 *
 * @param message The message received from the client.
 * @param response The response to be sent back to the client.
 *
 * The message format varies based on the command:
 * - CONNECT: "C"
 * - MOVE: "M <player_id> <session_token> <direction>"
 * - ZAP: "Z <player_id> <session_token>"
 * - DISCONNECT: "D <player_id> <session_token>"
 *
 * The response format also varies based on the result of the command:
 * - Success: "<response_code> [client_score]"
 * - Error: "<error_code>"
 *
 * Error codes come from config.h constants
 */
void process_client_message(char* message, char* response) {
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
                    sprintf(response, "%d %c %s", RESP_OK, new_id, players[i].session_token);
                    //printf("New player %c initialized at (%d,%d) with session token %s\n",new_id, players[i].x, players[i].y, players[i].session_token);

                    send_game_state();
                    return;
                }
            }
        }
        //ERROR Maximum number of players reached
        sprintf(response, "%d", ERR_FULL);
        return;
    }

    // Validate session token and player ID
    char cmd;
    char player_id;
    char session_token[33];
    int num_parsed = sscanf(message, "%c %c %32s", &cmd, &player_id, session_token);

    if (num_parsed < 3) {
        //ERROR Missing session token
        sprintf(response, "%d", ERR_INVALID_TOKEN);
        return;
    }

    // Validate the command string (allowed commands: CONNECT, MOVE, ZAP, DISCONNECT)
    if (cmd != CMD_MOVE && cmd != MSG_ZAP && cmd != CMD_DISCONNECT) {
        //ERROR Unknown command
        sprintf(response, "%d", ERR_UNKNOWN_CMD);
        return;
    }

    // Validate player_id (should be between 'A' and 'H')
    if (player_id < 'A' || player_id > 'H') {
        //ERROR Invalid player ID
        sprintf(response, "%d", ERR_INVALID_PLAYERID);
        return;
    }

    // Validate session token characters (should be hexadecimal)
    for (int i = 0; i < 32; i++) {
        if (!isxdigit(session_token[i])) {
            //ERROR Invalid session token characters
            sprintf(response, "%d", ERR_INVALID_TOKEN);
            return;
        }
    }

    // Validate session_token length (should be exactly 32 characters)
    if (strlen(session_token) != 32) {
        //ERROR Invalid session token length
        sprintf(response, "%d", ERR_INVALID_TOKEN);
        return;
    }

    // Find the player based on the provided player ID and session token
    Player_t* player = find_by_id(player_id);
    if (!player) {
        //ERROR Invalid player ID
        sprintf(response, "%d", ERR_INVALID_PLAYERID);
        return;
    } 
    if (strcmp(player->session_token, session_token) != 0) {
        //ERROR Invalid session token
        sprintf(response, "%d", ERR_INVALID_TOKEN);
        return;
    }


    // Command handling with checks
    if (cmd == CMD_MOVE) {
        time_t current_time = time(NULL);
        char direction;
        if (sscanf(message, "%*c %*c %*s %c", &direction) != 1) {
            //ERROR Invalid MOVE command format
            sprintf(response, "%d", ERR_INVALID_MOVE);
            return;
        }

        if (difftime(current_time, player->last_stun_time) < STUN_DURATION) {
            //ERROR Player stunned
            sprintf(response, "%d", ERR_STUNNED);
            return;
        }

        // Validate direction
        if (direction != MOVE_UP && direction != MOVE_DOWN && direction != MOVE_LEFT && direction != MOVE_RIGHT) {
            //ERROR Invalid direction
            sprintf(response, "%d", ERR_INVALID_DIR);
            return;
        }

        if (is_valid_move(player, direction)) {
            if (direction ==MOVE_LEFT) player->x--;
            else if (direction == MOVE_RIGHT) player->x++;
            else if (direction == MOVE_UP) player->y--;
            else if (direction == MOVE_DOWN) player->y++;
            snprintf(response, BUFFER_SIZE, "%d %d", RESP_OK, player->score);
        } else {
            //ERROR Invalid move direction
            sprintf(response, "%d", ERR_INVALID_MOVE);
            return;
        }
    } else if (cmd == MSG_ZAP)  {
        time_t current_time = time(NULL);
        if (difftime(current_time, player->last_fire_time) < LASER_COOLDOWN) {
            //ERROR Laser cooldown
            sprintf(response, "%d", ERR_LASER_COOLDOWN);
            return;
        }
        if (difftime(current_time, player->last_stun_time) < STUN_DURATION) {
            //ERROR Player stunned
            sprintf(response, "%d", ERR_STUNNED);
            return;
        }
        player->last_fire_time = current_time;
        if (player->laser.active) {
            //ERROR Laser already active // ? IS IT EVER REACHED? BECAUSE LASER WOULD BE IN COOLDOWN
            sprintf(response, "%d", ERR_LASER_COOLDOWN);
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

        // Send updated state to display
        send_game_state();

        
        snprintf(response, BUFFER_SIZE, "%d %d", RESP_OK, player->score);
    } else if (cmd == CMD_DISCONNECT)  {
        clear_player(player);
        //printf("Player %c disconnected.\n", player->player_id);
        sprintf(response, "%c", RESP_OK);
    } else {
        //ERROR Unknown command
        sprintf(response, "%c", ERR_UNKNOWN_CMD);
    }
}



/**
 * @brief Checks for collisions between active lasers and aliens or players.
 * 
 * This function iterates through all players and checks if their laser is active.
 * If the laser is active, it checks for collisions with aliens and other players
 * based on the player's zone and the laser's direction.
 * 
 * - If the player's zone is ZONE_A, ZONE_H, ZONE_D, or ZONE_F, it checks for collisions
 *   with aliens and players along the y-axis.
 * - Otherwise, it checks for collisions with aliens and players along the x-axis.
 * 
 * When a collision with an alien is detected, the alien is marked as inactive and
 * the player's score is updated.
 * 
 * When a collision with another player is detected, the other player's last stun time
 * is updated to the current time.
 */
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


/**
 * @brief Updates the positions of active aliens in the game.
 *
 * This function moves each active alien in a random direction (up, down, left, or right)
 * if at least one second has passed since the last move. The new position is only applied
 * if it is within the defined alien area boundaries.
 *
 * The function uses the current time to determine if the aliens should be moved and updates
 * the last move time accordingly.
 */
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

/**
 * @brief Updates the game state by performing several actions:
 *        - Updates the positions of all aliens.
 *        - Checks for laser collisions and updates scores accordingly.
 *        - Deactivates lasers that have been active for longer than LASER_DURATION seconds.
 *        - Checks if all aliens are destroyed and sets the game over flag if true.
 */
void update_game_state() {
    time_t current_time = time(NULL);
    
    // Update alien positions
    update_alien_positions();
    
    // Check laser collisions and update scores
    check_laser_collisions();
    
    // Deactivate lasers after LASER_DURATION seconds
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (players[i].laser.active && difftime(current_time, players[i].laser.creation_time) >= LASER_DURATION) {
            players[i].laser.active = 0;
        }
    }

    // Check if all aliens are destroyed
    if (all_aliens_destroyed()) {
        game_over = 1; // Set a game over flag
    }
}

/**
 * @brief Sends the game over state message to all displays.
 *
 * This function constructs a message indicating the game is over and includes
 * the final scores of all players. The message is then sent using ZeroMQ.
 *
 * The message format includes:
 * - A game over command.
 * - The final scores of all players who participated in the game.
 *
 * The message is sent via the ZeroMQ publisher socket.
 */
void send_game_over_state() {
    char message[BUFFER_SIZE] = "";
    char temp[100];

    // Include game over command
    temp[0] = CMD_GAME_OVER;
    temp[1] = '\n';
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
}

/**
 * @brief Main game logic loop.
 *
 * This function initializes the game state and enters a loop where it processes
 * client messages, updates the game state, and sends updates to the display.
 * The loop continues until the game is over.
 *
 * @param resp Pointer to the responder socket.
 * @param pub Pointer to the publisher socket.
 */
void game_logic(void* resp, void* pub) {
    initialize_game_state();
    printf("Game logic started\n");

    publisher = pub;
    responder = resp;

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
            //char response[] = "ERROR Message too long";
            char response = ERR_TOLONG;
            zmq_send(responder, &response, sizeof(response), 0);
        }
        
        // Update game state
        update_game_state();

        // Send updated state to display
        send_game_state();
        
        usleep(50000); // 50ms delay
    }

    send_game_over_state();
}