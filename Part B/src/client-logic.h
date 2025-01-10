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
#include "config.h"
#include "pthread.h"

void input_key(int ch);

void client_main(void* requester, int ncurses);

#endif