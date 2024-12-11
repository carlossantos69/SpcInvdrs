/*
 * File: space-display.c
 * Author: Carlos Santos e Tomás Corral
 * Description: This is the header file for the game logic program.
 * Date: 11/12/2024
 */

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

Player_t* find_by_id(const char id);
Player_t* find_by_session_token(const char* session_token);
Player_t* find_by_zone(const char zone);
void generate_session_token(char* token);
char assign_player_id();
int get_random_zone();
void clear_player(Player_t *player);
int all_aliens_destroyed();
void initialize_game_state();
void send_game_state();
int is_valid_move(Player_t* player, const char direction);
void initialize_player_position(Player_t* player);
void process_client_message(char* message, char* response);
void check_laser_collisions();
void update_alien_positions();
void update_game_state();
void send_game_over_state();
void cleanup();
void game_logic(void* resp, void* pub);
