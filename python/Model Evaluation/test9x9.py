from ratingsystem import RatingSystem

# this plays a match between our trained model and gnugo model
# gnugo can be installed using the command: sudo apt-get install gnugo
# pretrained random version of our model is needed or the gtp file in the build folder
numMatches = 1
batchSize = 1
print "running " + str(numMatches)+ " matches with batch size of "+ str(batchSize)
print "will use the default ratings available in ratings.txt"
playerOneParams = ["gnugo", "--mode", "gtp", "--boardsize", "9"]
playerTwoParams = ["./precompiled_gtp9_random"]
test = RatingSystem(playerOneParams, playerTwoParams,
                    numMatches=numMatches, batchSize=batchSize)
test.run()
