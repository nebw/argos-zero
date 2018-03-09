from ratingsystem import RatingSystem

# this plays a match between our trained model and gnugo model
# gnugo can be installed using the command: sudo apt-get install gnugo
# pretrained random version of our model is needed or the gtp file in the build folder
numMatches = 1
print "running " + str(numMatches)+ " matches"
print "will use the default ratings available in ./rating/ratings# file"
playerOneParams = ["./argos_model/gtp", "-p","./argos_model/agz_small_9x9"]
playerTwoParams = ["./pachi", "--no-dcnn", "-e", "uct", "-D"]
test = RatingSystem(playerOneParams, playerTwoParams, numMatches=numMatches)
print test.run()
