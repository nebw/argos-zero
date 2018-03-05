from match import Match
from player import Player

class Evaluator(object):
  def __init__(self, firstPlayer, secondPlayer, boardSize = 19, komi = 6.5, verbose = False):
    # params = ["--boardsize", str(boardSize),"--komi", str(komi)]
    params = []
    self.first = firstPlayer + params
    self.second = secondPlayer + params
    self.verbose = verbose

  def evaluate(self, numMatches):
    wins = [0,0]
    for i in range(numMatches):
      if i % 2 == 0:
        res = self.runMatch(self.first, self.second)
        wins[res] += 1
      else:
        res = self.runMatch(self.second, self.first)
        wins[(res + 1) % 2] += 1

    return wins

  def runMatch(self, playerOne, playerTwo):
    if self.verbose:
      print "match started"

    blackP = Player(playerOne, "black")

    if self.verbose:
      print "black player started: " + str(playerOne)

    whiteP = Player(playerTwo, "white")

    if self.verbose:
      print "white player started: " + str(playerTwo)

    match = Match(blackP, whiteP)
    res = match.run()
    
    if self.verbose:
      print "match done"
      if res == 0:
        print "black won ! " + str(playerOne)
      else:
        print "white won ! " + str(playerTwo)

    blackP.terminate()
    whiteP.terminate()
    return res
