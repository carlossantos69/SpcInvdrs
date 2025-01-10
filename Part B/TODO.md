# TODOS

## Game Server
    Decidir se mutex é preciso nos while (!game_over)
    send_game_state e send_score_updates bloqueiam o mutex do game state enquanto estão a mandar a mensagem
