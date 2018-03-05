from player import Player
import re

class Referee(Player):
  def __init__(self, processPath):
    super(Referee, self).__init__(processPath, "none")
  
  def simulateGame(self, moveList):
    players = ["black", "white"]
    currentPlayer = 0

    # simulate whole game
    for move in moveList:
      pl = players[currentPlayer]
      self.applyMove(pl, move)
      currentPlayer = (currentPlayer + 1) % 2
    
    # get result
    self.runCommand("final_score")
    score = self.getOutput()
    m = re.search("A|W|B", score)
    winner = score[m.start()]
    return 0 if winner == "B" else 1
