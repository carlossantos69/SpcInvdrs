// This code needs to include space-display.h to have the display logic
// Display logic to be removed from outer-space.display.c


#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "space-display.h"

// ZeroMQ socket
void* cont;
void* disp_sub;

void cleanup(void) {
    zmq_close(disp_sub);
    zmq_ctx_destroy(cont);
}

int main() {
    // Initialize ZeroMQ
    cont = zmq_ctx_new();
    disp_sub = zmq_socket(cont, ZMQ_SUB);

    // Connect to server's PUB socket
    if (zmq_connect(disp_sub, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        return 1;
    }

    // Subscribe to all messages
    zmq_setsockopt(disp_sub, ZMQ_SUBSCRIBE, "", 0);

    display_main(disp_sub);

    // Clean up
    cleanup();
    return 0;
}