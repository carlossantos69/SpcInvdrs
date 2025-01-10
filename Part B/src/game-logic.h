/*
 * PSIS 2024/2025 - Project Part 2
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
#include "scores.pb-c.h"
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


/**
 * @brief Returns the current time in seconds since the epoch.
 *
 * @return The current time in seconds as a double.
 */
double get_time_in_seconds();

/**
 * @brief Checks if the specified duration has passed since the start time.
 *
 * @param start_time The start time in seconds.
 * @param duration The duration to check in seconds.
 * @return 1 if the duration has passed, 0 otherwise.
 */
int has_duration_passed(double start_time, double duration);

/**
 * @brief Finds a player by their ID.
 *
 * @param id The ID of the player to find.
 * @return A pointer to the player with the specified ID, or NULL if not found.
 */
Player_t* find_by_id(const char id);

/**
 * @brief Finds a player by their session token.
 *
 * @param session_token The session token of the player to find.
 * @return A pointer to the player with the specified session token, or NULL if not found.
 */
Player_t* find_by_session_token(const char* session_token);

/**
 * @brief Finds a player by their zone.
 *
 * @param zone The zone of the player to find.
 * @return A pointer to the player in the specified zone, or NULL if not found.
 */
Player_t* find_by_zone(const char zone);

/**
 * @brief Generates a random session token.
 *
 * @param token A pointer to a character array where the generated token will be stored.
 */
void generate_session_token(char* token);

/**
 * @brief Assigns a unique player ID.
 *
 * @return A unique player ID, or '\0' if no IDs are available.
 */
char assign_player_id();

/**
 * @brief Selects a random unoccupied zone.
 *
 * @return An integer representing the randomly selected unoccupied zone.
 */
int get_random_zone();

/**
 * @brief Clears the player's data.
 *
 * @param player A pointer to the player to clear.
 */
void clear_player(Player_t *player);

/**
 * @brief Checks if all aliens have been destroyed.
 *
 * @return 1 if all aliens are inactive, 0 if at least one alien is still active.
 */
int all_aliens_destroyed();

/**
 * @brief Initializes the game state.
 */
void initialize_game_state();

/**
 * @brief Checks if the player's move in the specified direction is valid.
 *
 * @param player A pointer to the player.
 * @param direction The direction of the move.
 * @return 1 if the move is valid, 0 otherwise.
 */
int is_valid_move(Player_t* player, const char direction);

/**
 * @brief Initializes the player's position based on their zone.
 *
 * @param player A pointer to the player.
 */
void initialize_player_position(Player_t* player);

/**
 * @brief Processes a message from a client and generates a response.
 *
 * @param message The message received from the client.
 * @param response The response to be sent back to the client.
 * @return 0 if the game state was not updated, 1 if it was updated.
 */
int process_client_message(char* message, char* response);

/**
 * @brief Checks for collisions between lasers and aliens or players.
 */
void check_laser_collisions();

/**
 * @brief Updates the positions of active aliens.
 */
void update_alien_positions();

/**
 * @brief Updates the game state.
 */
void update_game_state();

/**
 * @brief Sends the current game state to all subscribers.
 */
void send_game_state();

/**
 * @brief Sends score updates for all players.
 */
void send_score_updates();

/**
 * @brief Sends the game over state to all subscribers.
 */
void send_game_over_state();

/**
 * @brief Thread routine for alien movement and game state updates.
 *
 * @param arg Unused parameter.
 * @return None.
 */
void* thread_alien_routine(void* arg);

/**
 * @brief Thread routine to periodically update the game state.
 *
 * @param arg Unused parameter.
 * @return None.
 */
void* thread_updater_routine(void* arg);

/**
 * @brief Thread routine for listening to client messages and processing them.
 *
 * @param arg Unused parameter.
 * @return None.
 */
void* thread_listener_routine(void* arg);

/**
 * @brief Thread routine for publishing game state and score updates.
 *
 * @param arg Unused parameter.
 * @return void* Always returns NULL.
 */
void* thread_publisher_routine(void* arg);

/**
 * @brief Sets the game over flag to end the game.
 */
void end_server_logic();

/**
 * @brief Retrieves the current game state from the server.
 *
 * @param buffer A pointer to a character array where the game state string will be copied.
 */
void get_server_game_state(char* buffer);

/**
 * @brief Main server logic function that initializes mutexes, condition variables,
 *        game state, and creates necessary threads for game operation.
 *
 * @param responder Pointer to the responder object.
 * @param publisher Pointer to the publisher object.
 * @param score_publisher Pointer to the score publisher object.
 * @return int Returns 0 on success, -1 on failure.
 */
int server_logic(void* responder, void* publisher, void* score_publisher);

#endif