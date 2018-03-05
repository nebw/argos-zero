from evaluator import Evaluator
from parameterparser import Parser
import glicko2
import ast


class RatingSystem(Evaluator):
    def __init__(self, playerOneArgs, playerTwoArgs, mode ="19",numMatches=1, batchSize=20, verbose=True, fileName="ratings.txt"):
        super(RatingSystem, self).__init__(playerOneArgs, playerTwoArgs, mode, verbose=verbose)
        self.numMatches = numMatches
        self.batchSize = batchSize
        self.ratings = self.getPlayerRatingsFromFile(fileName)
        self.args = [playerOneArgs, playerTwoArgs]
        self.verbose = verbose

    def getPlayerRatingsFromFile(self, fileName):
        modelsParams = [[1500, 350, 0.06], [1500, 350, 0.06]]
        try:
          with open(fileName, "r") as stats:
              content = stats.readlines()
              i = 0
              while i < (len(content)):
                  line = content[i]
                  if self.args[0][0] in line:
                      i += 1
                      line = content[i]
                      modelsParams[0] = ast.literal_eval(line)
                  elif self.args[1][0] in line:
                      i += 1
                      line = content[i]
                      modelsParams[1] = ast.literal_eval(line)
                  i += 1
          return [glicko2.Player(*modelsParams[0]), glicko2.Player(*modelsParams[1])]
        except:
          return [glicko2.Player(1500, 350, 0.06), glicko2.Player(1500, 350, 0.06)]

    def run(self):
        endResults = [0, 0]
        numMatches = self.numMatches
        while numMatches > 0:
            length = min(numMatches, self.batchSize)
            results = self.evaluate(length)
            endResults = [sum(x) for x in zip(results, endResults)]

            self.ratings[0].update_player([self.ratings[1].getRating()] * length,
                                          [self.ratings[1].getRd()] * length,
                                          [1] * results[0] + [0] * results[1])

            self.ratings[1].update_player([self.ratings[0].getRating()] * length,
                                          [self.ratings[0].getRd()] * length,
                                          [0] * results[0] + [1] * results[1])

            numMatches -= length
            if self.verbose:
                print "results for batch:\n wins: " + str(results)
                print "ratings: "
                print self.args[0][0], "(", self.ratings[0].getRating(), ")"
                print self.args[1][0], "(", self.ratings[1].getRating(), ")"

        if self.verbose:
            print "\n -------------\ndone!"
            print "total wins: " + str(endResults)
            print "ratings:"
            print self.args[0][0], "(", self.ratings[0].getRating(), ")"
            print self.args[1][0], "(", self.ratings[1].getRating(), ")"
        return endResults, [self.ratings[0].getRating(), self.ratings[1].getRating()]


if __name__ == "__main__":
  args=Parser("RatingSystem").parse()
  RatingSystem(
    args.playerOne, args.playerTwo, args.numMatches, args.batchSize, args.verbose, args.loadFrom
  ).run()
