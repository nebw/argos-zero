from matchsystem import MatchSystem
from parameterparser import Parser
import ast


class RatingSystem(MatchSystem):
    def __init__(self, playerOneArgs, playerTwoArgs, numMatches=1, verbose=True, fileName="ratings.txt"):
        super(RatingSystem, self).__init__(playerOneArgs, playerTwoArgs, verbose)
        self.numMatches = numMatches
        self.args = [playerOneArgs, playerTwoArgs]
        self.verbose = verbose
        self.ratings = self.getPlayerRatingsFromFile(fileName)
        if verbose:
          print "Ratings:", self.ratings

    def getPlayerRatingsFromFile(self, fileName="ratings.txt"):
        modelsParams = [[1000, 40], [1000, 40]]
        try:
            with open(fileName, "r") as stats:
              content = stats.readlines()
              i = 0
              while i < (len(content)):
                line = content[i]
                if str(self.args[0]) in line:
                    i += 1
                    line = content[i]
                    modelsParams[0] = ast.literal_eval(line)
                elif str(self.args[1]) in line:
                    i += 1
                    line = content[i]
                    modelsParams[1] = ast.literal_eval(line)
                i += 1
            return [modelsParams[0], modelsParams[1]]
        except:
            return [[1000, 40], [1000, 40]]

    def run(self):
        endResults = self.runMatches(self.numMatches)

        for i in ([1] * endResults[0] + [0] * endResults[1]):
          self.ratings[0][0] = self.calculateNewElo(
              self.ratings[0][0], self.ratings[1][0], i, self.ratings[0][1])

          self.ratings[1][0] = self.calculateNewElo(
              self.ratings[1][0], self.ratings[0][0], 1 - i, self.ratings[1][1])

        if self.verbose:
            print "\n -------------\ndone!"
            print "total wins: " + str(endResults)
            print "ratings:"
            print self.args[0][0], "(", self.ratings[0][0], ")"
            print self.args[1][0], "(", self.ratings[1][0], ")"
        return endResults, [self.ratings[0][0], self.ratings[1][0]]

    def calculateNewElo(self, elo1, elo2, erg1, k=20):
      dif = elo2 - elo1
      e1=1/(1+pow(10, dif / 400.))
      return int(round((elo1+k*(erg1-e1)), 0)) 


if __name__ == "__main__":
    args = Parser("RatingSystem").parse()
    RatingSystem(
        args.playerOne, args.playerTwo, args.numMatches, args.verbose, args.loadFrom
    ).run()
