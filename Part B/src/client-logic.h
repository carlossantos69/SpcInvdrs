/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: client-logic.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Header file for client-logic.h
 */


#ifndef CLIENT_LOGIC_H
#define CLIENT_LOGIC_H

// Extern variables that will be used in other files
extern int input_buffer;
extern char output_buffer_line1[BUFFER_SIZE];
extern char output_buffer_line2[BUFFER_SIZE];
extern int input_ready;
extern int output_ready;
extern pthread_mutex_t client_lock;

void client_main(void* requester);

#endif