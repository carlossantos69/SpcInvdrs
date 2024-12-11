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
 * Header file for game-logic.c
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>
#include <ctype.h>
#include "config.h"

#define BORDER_OFFSET 2      // Distance from edge for playable area
#define ALIEN_AREA_START 2   // Where aliens can start moving
#define ALIEN_AREA_END 17    // Where aliens must stop moving

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

/**
 * @brief Finds a player by their unique ID.
 * 
 * @param id The unique ID of the player.
 * @return Player_t* Pointer to the player structure if found, otherwise NULL.
 */
Player_t* find_by_id(const char id);

/**
 * @brief Finds a player by their session token.
 * 
 * @param session_token The session token of the player.
 * @return Player_t* Pointer to the player structure if found, otherwise NULL.
 */
Player_t* find_by_session_token(const char* session_token);

/**
 * @brief Finds a player by their zone.
 * 
 * @param zone The zone of the player.
 * @return Player_t* Pointer to the player structure if found, otherwise NULL.
 */
Player_t* find_by_zone(const char zone);

/**
 * @brief Generates a new session token for a player.
 * 
 * @param token Pointer to a character array where the generated token will be stored.
 */
void generate_session_token(char* token);

/**
 * @brief Assigns a unique ID to a player.
 * 
 * @return char The assigned unique ID.
 */
char assign_player_id();

/**
 * @brief Gets a random zone for a player.
 * 
 * @return int The randomly selected zone.
 */
int get_random_zone();

/**
 * @brief Clears the player structure, resetting its values.
 * 
 * @param player Pointer to the player structure to be cleared.
 */
void clear_player(Player_t *player);

/**
 * @brief Checks if all aliens have been destroyed in the game.
 * 
 * @return int Returns 1 if all aliens are destroyed, otherwise 0.
 */
int all_aliens_destroyed();

/**
 * @brief Initializes the game state.
 */
void initialize_game_state();

/**
 * @brief Sends the current game state to the clients.
 */
void send_game_state();

/**
 * @brief Checks if a move is valid for a player.
 * 
 * @param player Pointer to the player structure.
 * @param direction The direction of the move.
 * @return int Returns 1 if the move is valid, otherwise 0.
 */
int is_valid_move(Player_t* player, const char direction);

/**
 * @brief Initializes the position of a player.
 * 
 * @param player Pointer to the player structure.
 */
void initialize_player_position(Player_t* player);

/**
 * @brief Processes a message from a client and generates a response.
 * 
 * @param message The message received from the client.
 * @param response The response to be sent back to the client.
 */
void process_client_message(char* message, char* response);

/**
 * @brief Checks for collisions between lasers and other objects.
 */
void check_laser_collisions();

/**
 * @brief Updates the positions of the aliens in the game.
 */
void update_alien_positions();

/**
 * @brief Updates the overall game state.
 */
void update_game_state();

/**
 * @brief Sends the game over state to the clients.
 */
void send_game_over_state();

/**
 * @brief Cleans up resources used by the game.
 */
void cleanup();

/**
 * @brief Main game logic function.
 * 
 * @param resp Pointer to the response object.
 * @param pub Pointer to the publish object.
 */
void game_logic(void* resp, void* pub);


#endif