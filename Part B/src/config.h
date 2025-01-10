/*
 * PSIS 2024/2025 - Project Part 2
 *
 * Filename: config.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Definitions for constants used in the apps
 */

#ifndef SPCINVDRS_CONFIG_H
#define SPCINVDRS_CONFIG_H

// Game Limits and Rules
#define LASER_COOLDOWN 3     // seconds between laser fires
#define STUN_DURATION 10     // seconds an astronaut is stunned
#define ALIEN_MOVE_INTERVAL 1 // Time between alien movements (seconds)
#define GAME_UPDATE_INTERVAL 0.05  // Time between game state updates (seconds)
#define ALIEN_RECOVERY_TIME 10 // Time for alien to respawn (seconds)
#define KILL_POINTS 1       // points for killing an alien

// Network Configuration
#define SERVER_ENDPOINT_REQ "tcp://*:5555"    // For REQ/REP with astronauts
#define SERVER_ENDPOINT_PUB "tcp://*:5556"    // For PUB/SUB with display
#define SERVER_ENDPOINT_SCORES "tcp://*:5557" // For PUB/SUB with scores
#define SERVER_ENDPOINT_HEARTBEAT "tcp://*:5558" // For PUB/SUB with heartbeat
#define CLIENT_CONNECT_REQ "tcp://localhost:5555"  // For astronauts to connect
#define CLIENT_CONNECT_SUB "tcp://localhost:5556"  // For display to connect
#define CLIENT_CONNECT_HEARTBEAT "tcp://localhost:5558"  // For client heartbeats
#define HEARTBEAT_FREQUENCY 1 // seconds between heartbeats

// Game Constants
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_PLAYERS 8
#define MAX_ALIENS 4 // 1/3 of grid area?
#define BUFFER_SIZE 2048
#define LASER_DURATION 0.5

// UI Positions
#define BORDER_OFFSET 2      // Distance from edge for playable area
#define ALIEN_AREA_START 2   // Where aliens can start moving
#define ALIEN_AREA_END 17    // Where aliens must stop moving
#define SCORE_START_X (GRID_WIDTH + 6)
#define LASER_HORIZONTAL '-'
#define LASER_VERTICAL '|'

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
#define RESP_OK 0
#define ERR_UNKNOWN_CMD -1
#define ERR_UNKNOWN_CMD_MSG "Unknown command"
#define ERR_TOLONG -2
#define ERR_TOLONG_MSG "Message too long"
#define ERR_FULL -3
#define ERR_FULL_MSG "Maximum number of players reached"
#define ERR_INVALID_TOKEN -4
#define ERR_INVALID_TOKEN_MSG "Invalid session token"
#define ERR_INVALID_PLAYERID -5
#define ERR_INVALID_PLAYERID_MSG "Invalid session id"
#define ERR_STUNNED -6
#define ERR_STUNNED_MSG "Player stunned"
#define ERR_INVALID_MOVE -7
#define ERR_INVALID_MOVE_MSG "Invalid move command"
#define ERR_INVALID_DIR -8
#define ERR_INVALID_DIR_MSG "Invalid dirrection"
#define ERR_LASER_COOLDOWN -9
#define ERR_LASER_COOLDOWN_MSG "Laser cooldown"

#endif