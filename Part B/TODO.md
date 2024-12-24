# Game Server

Vai ter de ser reformulado para usar threads.
Recebe mensagens dos clients: astronaut e astronaut-display, são a mesma coisa
Manda mensagens para os displays: astronaut-display e outer-display; scores para a app de scores 
Threads para:
    - movimentos dos aliens
    - receber mensagens
    - mandar mensagens
    - ...

Usar mutexes quando se aceder a memoria partilhada

# Astronaut-client

Devia de ficar basicamente igual.
Só manda mensagens para o game server e recebe respostas do seu score.

# OuterSpaceDisplay

Também devia de ficar igual, só recebe mensagens do game server e atualiza o ecrã da mesma maneira.

# AstronautDisplay

Astronaut client + outer space display.
Manda e recebe mensagens ZMQ, tem de receber todas as mensagens necessárias para atualizar o ecrã da mesma forma que o outer-space-display.
Mutexes para as estruturas que precisa para atualizar o ecrã, da mesma forma que o game server vai fazer.

# Score Update

Recebe mensagens de todos os scores e atualiza o ecrã com essa informação.