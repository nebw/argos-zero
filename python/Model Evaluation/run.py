from match import Match
from player import Player

class Evaluator():
  def __init__(self, blackPlayer, whitePlayer, boardSize = 19, komi = 6.5):
    # params = ["--boardsize", str(boardSize), "--komi", str(komi)]
    params = []
    self.black = blackPlayer + params
    self.white = whitePlayer + params

  def evalute(self, numMatches):
    wins = [0,0]
    for _ in range(numMatches):
      blackP = Player(self.black, "black")
      print "black player started"
      whiteP = Player(self.white, "white")
      print "white player started"
      match = Match(blackP, whiteP)
      res = match.run()
      wins[res] += 1
      # blackP.runCommand("quit")
      # whiteP.runCommand("quit")

    return wins


print Evaluator(["gnugo", "--mode", "gtp"],
          ["/home/abdulllah/Desktop/Linux/argos/argos-zero/argos-zero-build/src/gtp"]).evalute(1)


