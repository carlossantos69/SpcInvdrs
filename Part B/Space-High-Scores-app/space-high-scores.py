"""
PSIS 2024/2025 - Project Part 2

Filename: space-high-scores.py

Authors:
- Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
- Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt

Group ID: 20

Description:
Code that handles high score updates display. Receives updates from the server.
"""


import zmq
from scores_pb2 import ScoreUpdate

def receive_score_updates():
    print("Starting the high scores subscriber, waiting for server to send updates...")

    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://localhost:5557")
    socket.setsockopt_string(zmq.SUBSCRIBE, '')

    current_scores = {}

    while True:
        message = socket.recv()
        score_update = ScoreUpdate()
        score_update.ParseFromString(message)

        if score_update.game_over:
            print("\nGame Over!")
            break

        # Rebuild the scores dictionary
        new_scores = {}
        for score in score_update.scores:
            new_scores[score.player_id] = score.score
        current_scores = new_scores

        print("\033c", end="")
        print("Current Scores:")
        for pid, sc in sorted(current_scores.items()):
            print(f"Player {chr(pid)}: {sc}")

if __name__ == "__main__":
    receive_score_updates()