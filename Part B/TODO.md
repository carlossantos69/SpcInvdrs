# TODOS

## Makefile

    Meter o makefile a apagar ficheiros de compilação temporarios. Apenas guardar os outputs


## Game Server
    Decidir o que se faz quando não se consegue começar as threads no game-logic.c
    Decidir se mutex é preciso nos while (!game_over)
    Decidir se é preciso sleep em alguns dos sitios
    Arranjar forma se só publicar o novo estado quando se processa uma mensagem se realmente for preciso
    send_game_state e send_score_updates bloqueiam o mutex do game state enquanto estão a mandar a mensagem
    Arranjar forma de nao usar extern no game-logic.c. Criar uma função get_game_state() no game-logic. que é chamada no game-server.c




## Geral
    Pesquisar se é preciso pthread_mutex_destroy para um mutex inicializado com PTHREAD_MUTEX_INITIALIZER