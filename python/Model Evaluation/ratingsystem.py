from matchsystem import MatchSystem
from ratingmanager import RatingManager
from parameterparser import Parser
import ast


class RatingSystem(MatchSystem):
    def __init__(self, playerOneArgs, playerTwoArgs, numMatches=1, verbose=True, fileName="ratings", filePath = "./"):
        super(RatingSystem, self).__init__(playerOneArgs, playerTwoArgs, verbose)
        self.numMatches = numMatches
        self.args = [playerOneArgs, playerTwoArgs]
        self.verbose = verbose
        self.ratingManager = RatingManager(fileName, filePath)
        if verbose:
          ratings = self.ratingManager.getEloRatingOfModels(self.args)
          print "ratings:"
          print self.args[0], "(", ratings[0], ")"
          print self.args[1], "(", ratings[1], ")"

    def run(self):
        endResults = self.runMatches(self.numMatches)
        ratings = self.ratingManager.updateRatings(self.args, endResults)
        if self.verbose:
            print "\n -------------\ndone!"
            print "total wins: " + str(endResults)
            print "ratings:", ratings
            print self.args[0], "(", ratings[0],")"
            print self.args[1], "(", ratings[1], ")"
        self.ratingManager.writeRatings()
        return endResults, ratings


if __name__ == "__main__":
    args = Parser("RatingSystem").parse()
    RatingSystem(
        args.playerOne, args.playerTwo, args.numMatches, args.verbose, args.loadFromFile, args.loadFromPath
    ).run()
