// This code needs to include space-display.h to have the display logic
// Display logic removed from game-server.c

#include <stdbool.h>
#include <unistd.h>
#include <zmq.h>
#include "config.h"
#include "game-logic.h"
#include "space-display.h"

void* cont;
void* resp;  // For REQ/REP with astronauts
void* pub;  // For PUB/SUB with display

int main() {
    
    // Initialize ZeroMQ context
    cont = zmq_ctx_new();
    
    // Set up REQ/REP socket for astronaut clients
    resp = zmq_socket(cont, ZMQ_REP);
    zmq_bind(resp, SERVER_ENDPOINT_REQ);
    
    // Set up PUB socket for display client
    pub = zmq_socket(cont, ZMQ_PUB);
    zmq_bind(pub, SERVER_ENDPOINT_PUB);

    game_logic(resp, pub);
    
    return 0;
}