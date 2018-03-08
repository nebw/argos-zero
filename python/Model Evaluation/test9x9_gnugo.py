from ratingsystem import RatingSystem

# this plays a match between our trained model and gnugo model
# gnugo can be installed using the command: sudo apt-get install gnugo
# pretrained random version of our model is needed or the gtp file in the build folder
numMatches = 1
print "running " + str(numMatches) + " matches"
print "will use the default ratings available in ./rating/ratings#"
print "if this test fails you maybe have not installed GnuGo yet? apt-get install gnugo"
playerOneParams = ["gnugo", "--mode", "gtp", "--boardsize", "9"]
playerTwoParams = ["./argos_model/gtp", "-p",
                   "./argos_model/agz_small_9x9", "-l", "./argos_model/agz_small_9x9.log"]
test = RatingSystem(playerOneParams, playerTwoParams, numMatches=numMatches, filePath ="./rating")
print test.run()
