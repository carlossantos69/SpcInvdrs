#ifndef SPCINVDRS_CONFIG_H
#define SPCINVDRS_CONFIG_H

// Game Limits and Rules
#define LASER_COOLDOWN 3     // seconds between laser fires
#define STUN_DURATION 10     // seconds an astronaut is stunned
#define ALIEN_MOVE_INTERVAL 1 // seconds between alien movements
#define KILL_POINTS 1

// Network Configuration
// config.h
#define SERVER_ENDPOINT_REQ "tcp://*:5555"    // For REQ/REP with astronauts
#define SERVER_ENDPOINT_PUB "tcp://*:5556"    // For PUB/SUB with display
#define CLIENT_CONNECT_REQ "tcp://localhost:5555"  // For astronauts to connect
#define CLIENT_CONNECT_SUB "tcp://localhost:5556"  // For display to connect

// Game Constants
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_PLAYERS 8
#define MAX_ALIENS 6  // 1/3 of grid area
#define BUFFER_SIZE 2048

// UI Positions
// TODO: START USING THESE
#define SCORE_START_Y 5
#define SCORE_START_X (GRID_WIDTH + 6)
#define SCORE_PADDING 4  // Space between grid and scores

// Color pair definitions
#define COLOR_ASTRONAUT 1
#define COLOR_ALIEN 2
#define COLOR_LASER 3

// Command types
#define CMD_PLAYER 'P'
#define CMD_LASER 'L'
#define CMD_ALIEN 'A'
#define CMD_SCORE 'S'
#define CMD_GAME_OVER 'G'
#define CMD_CONNECT 'C'
#define CMD_DISCONNECT 'D'
#define CMD_MOVE 'M'
#define MSG_ZAP 'Z'

// Command Movement Directions
#define MOVE_UP 'U'
#define MOVE_DOWN 'D'
#define MOVE_LEFT 'L'
#define MOVE_RIGHT 'R'

// Player Zones
#define ZONE_A 1
#define ZONE_B 2
#define ZONE_C 3
#define ZONE_D 4
#define ZONE_E 5
#define ZONE_F 6
#define ZONE_G 7
#define ZONE_H 8

// Command responses and error Codes
//TODO: START USING THEM
// TODO: Add more responses and use them
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define ERR_MAX_PLAYERS -1
#define ERR_INVALID_MOVE -2
#define ERR_LASER_COOLDOWN -3

#endif // SPCINVDRS_CONFIG