/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: game-logic.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
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
#include <unistd.h>
#include <zmq.h>
#include <ctype.h>
#include "config.h"

/**
 * @brief Structure representing a laser in the game.
 * 
 * This structure holds the coordinates, active status, and creation time of a laser.
 */
typedef struct {
    int x;
    int y;
    int active;
    double creation_time; // Seconds since epoch with microsecond precision
} Laser_t;

/**
 * @struct Player_t
 * @brief Represents a player in the game.
 *
 * This structure holds information about a player, including their position,
 * score, and various timestamps related to their actions in the game.
 *
 * @var Player_t::id
 * The unique identifier for the player.
 *
 * @var Player_t::zone
 * The zone in which the player is currently located.
 *
 * @var Player_t::x
 * The x-coordinate of the player's position.
 *
 * @var Player_t::y
 * The y-coordinate of the player's position.
 *
 * @var Player_t::score
 * The current score of the player.
 *
 * @var Player_t::last_fire_time
 * The timestamp of the last time the player fired their weapon. Seconds since epoch with microsecond precision 
 *
 * @var Player_t::last_stun_time
 * The timestamp of the last time the player was stunned. Seconds since epoch with microsecond precision
 *
 * @var Player_t::session_token
 * A 32-character hexadecimal session token used to identify the player's session.
 *
 * @var Player_t::laser
 * The laser associated with the player.
 */
typedef struct {
    char id;
    int zone;
    int x;
    int y;
    int score;
    double last_fire_time; // Seconds since epoch with microsecond precision
    double last_stun_time; // Seconds since epoch with microsecond precision
    char session_token[33]; // 32-char hex token + null terminator
    Laser_t laser; //The laser of the player 
} Player_t;

/**
 * @brief Structure representing an alien in the game.
 * 
 * This structure holds the coordinates and active status of an alien.
 */
typedef struct {
    int x;
    int y;
    int active;
} Alien_t;


// Extern variables that will be used in other files
extern char game_state_server[BUFFER_SIZE];
extern int request_game_over;
extern pthread_mutex_t server_lock;

/**
 * @brief Returns the number of seconds since the epoch as a double.
 *
 * This function uses the gettimeofday function to get the current time
 * and returns the number of seconds since the epoch (January 1, 1970)
 * as a double.
 *
 * @return The number of seconds since the epoch as a double.
 */
double get_time_in_seconds();

/**
 * @brief Checks if the specified duration has passed since the given time.
 *
 * This function compares the current time with the given time and returns 1
 * if the difference is greater than or equal to the specified duration.
 *
 * @param start_time The start time in seconds since the epoch.
 * @param duration The duration to check in seconds.
 * @return int Returns 1 if the duration has passed, 0 otherwise.
 */
int has_duration_passed(double start_time, double duration);

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
 * @brief Sends the current game state to all subscribers.
 */
void send_game_state();

/**
 * @brief Sends score updates to all score subscribers.
 */
void send_score_updates();

/**
 * @brief Sends the game over state to all subscribers.
 */
void send_game_over_state();

/**
 * @brief Sets the game over flag to end the game.
 *
 * This function sets the game over flag to 1, indicating that the game has ended.
 * It is used to stop the game logic loop .
 */
void end_game_logic();

/**
 * @brief Main game logic function.
 * 
 * @param resp Pointer to the response object.
 * @param pub Pointer to the publish object.
 */
void game_logic(void* resp, void* pub, void* score_pub);


#endif