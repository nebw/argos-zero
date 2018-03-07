from player import Player
import re

class Referee(Player):
  def __init__(self, mode = "9"):
    if mode == "9":
      super(Referee, self).__init__(["gnugo", "--mode", "gtp", "--boardsize", "9"], "none")
    elif mode == "19":
      super(Referee, self).__init__(["gnugo", "--mode", "gtp"], "none")
    else:
      raise ValueError("no referee for mode : "+ mode)
    self.mode = mode
  
  def simulateGame(self, moveList):
    players = ["black", "white"]
    currentPlayer = 0

    # simulate whole game
    for move in moveList:
      pl = players[currentPlayer]
      try:
        self.applyMove(pl, move)
      except ValueError:
        print "illegal move, other player wins"
        return (currentPlayer + 1) % 2
      currentPlayer = (currentPlayer + 1) % 2
    
    # get result
    self.runCommand("estimate_score")
    score = self.getOutput()
    print score
    return 0 if "B" in score else 1
