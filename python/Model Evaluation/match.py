from referee import Referee
from player import Player

class Match():
  def __init__(self, blackP, whiteP, mode ="19"):
    self.black = blackP
    self.white = whiteP
    self.mode = mode

  # returns 0 if black wins, 1 if white wins
  def run(self):
    numPasses = 0
    players = [self.black, self.white]
    currentPlayer = 0
    moveList = []
    # start with black player
    pl = players[currentPlayer]
    while (numPasses < 2):
      # generate move
      move = pl.genAndPlayMove()
      
      # switch player
      currentPlayer = (currentPlayer + 1) % 2
      pl = players[currentPlayer]

      # apply generated move to other player
      pl.applyMoveOfOtherPlayer(move)

      # game logic
      if "resign" in move.lower():
        return currentPlayer

      elif "pass" in move.lower():
        numPasses += 1
      else:       #its a normal move
        numPasses = 0
      moveList.append(move.replace("=", ""))

    # match has ended!
    ref = Referee(self.mode)
    winner = ref.simulateGame(moveList)
    ref.terminate()
    return winner

