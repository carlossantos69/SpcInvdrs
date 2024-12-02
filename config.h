#ifndef SPCINVDRS_CONFIG_H
#define SPCINVDRS_CONFIG_H

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
#define MAX_ALIENS 33  // 1/3 of grid area
#define BUFFER_SIZE 2048

#define SCORE_START_Y 5
#define SCORE_START_X (GRID_WIDTH + 6)
#define SCORE_PADDING 4  // Space between grid and scores

// Message Types
#define MSG_CONNECT 1
#define MSG_MOVEMENT 2
#define MSG_ZAP 3
#define MSG_DISCONNECT 4
#define MSG_UPDATE 5

// Color pair definitions
#define COLOR_ASTRONAUT 1
#define COLOR_ALIEN 2
#define COLOR_LASER 3

// Movement Directions
#define MOVE_LEFT 'L'
#define MOVE_RIGHT 'R'
#define MOVE_UP 'U'
#define MOVE_DOWN 'D'

// Game Rules
#define LASER_COOLDOWN 3     // seconds between laser fires
#define STUN_DURATION 10     // seconds an astronaut is stunned
#define ALIEN_MOVE_INTERVAL 1 // seconds between alien movements

// Scoring
#define KILL_POINTS 1

// Error Codes
#define ERR_MAX_PLAYERS -1
#define ERR_INVALID_MOVE -2
#define ERR_LASER_COOLDOWN -3

// Utility Macro for Error Checking
#define CHECK_ZMSG(msg, ...) \
    do { \
        if (!(msg)) { \
            fprintf(stderr, "Error in %s: ", __func__); \
            fprintf(stderr, __VA_ARGS__); \
            exit(1); \
        } \
    } while(0)

#endif // SPCINVDRS_CONFIG_H