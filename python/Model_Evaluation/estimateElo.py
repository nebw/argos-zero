from ratingsystem import RatingSystem
from parameterparser import Parser

def estimateNewPlayerElo(newPlayer, numMatches=10):
  listOfOpponents=[["gnugo", "--mode", "gtp", "--boardsize", "9"], ["./argos_model/gtp", "-p", "./argos_model/randomnet_small_9x9", "-l", "./argos_model/randomnet_small_9x9.log"], ["./pachi", "--no-dcnn", "-e", "uct", "-D"]]
  for opponent in listOfOpponents:
    playerOneParams = newPlayer
    playerTwoParams = opponent
    game = RatingSystem(playerOneParams, playerTwoParams, numMatches, filePath="./rating")
    game.run()


#estimateNewPlayerElo(["./argos_model/gtp", "-p","./argos_model/agz_small_9x9", "-l", "./argos_model/agz_small_9x9.log"], matchesToPlay=2)

if __name__ == "__main__":
  args = Parser("EstimateElo").parse()
  estimateNewPlayerElo(args.newPlayer, args.numMatches)
