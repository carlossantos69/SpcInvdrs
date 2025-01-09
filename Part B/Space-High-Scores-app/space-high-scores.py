import zmq
from scores_pb2 import ScoreUpdate

def receive_score_updates():
    current_scores = {}
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://localhost:5557")
    socket.setsockopt_string(zmq.SUBSCRIBE, '')

    while True:
        message = socket.recv()
        score_update = ScoreUpdate()
        score_update.ParseFromString(message)

        for score in score_update.scores:
            current_scores[score.player_id] = score.score

        print("\033c", end="")  # Clear screen
        print("Current Scores:")
        for pid, sc in sorted(current_scores.items()):
            print(f"Player {chr(pid)}: {sc}")

if __name__ == "__main__":
    receive_score_updates()