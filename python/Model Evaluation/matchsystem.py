from match import Match
from player import Player
import time

class MatchSystem(object):
  def __init__(self, firstPlayer, secondPlayer, verbose = False):
    self.first = firstPlayer
    self.second = secondPlayer
    self.verbose = verbose

  def runMatches(self, numMatches):
    wins = [0,0]
    for i in range(numMatches):
      if i % 2 == 0:
        res = self.runMatch(self.first, self.second)
        wins[res] += 1
      else:
        res = self.runMatch(self.second, self.first)
        wins[(res + 1) % 2] += 1

    return {
      str(self.first): wins[0],
      str(self.second): wins[1]
    }

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
