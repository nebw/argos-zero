# Model Evaluation

this module runs matches between diffrent models to find the winner, it also detrmines the ELO rating and handles its changes.

it will read the ELO numbers from the file system and write the results back.

## How to use:

### If you just want to run a couple of matches between two models, all you need is to import the MatchSystem class:
* The first and second parameters should be an array of command line arguments to run the model's gtp, ex: `['gnugo','--mode', 'gtp','--boardsize', '9']`
* verbose determines if there should be any logging
* to run the matches call MatchSystem.runMatches(numberOfMatches); it will return a dictonary with the number of wins of each model, they keys of the dictionary are the stringified model paramters defined in the constructor: (str(modelParam))

### If you also want to have ELO ratings use RatingSystem:
* the standard config will read ELO Ratings from `./rating/ratings#` (# highest number available)
* with each run of the system new ratings will be saved in `./rating/ratings(#+1)`
* the paramter for this system are similar to MatchSystem:
  * Players command line parameters
  * num of matches to be played
* it will return a tuple of:
  * wins dictonary (same as MatchSystem)
  * array with ELO ratings of the models

### If you just want an estimated rating of your model, use the estimateElo function:
* it runs a list of matches between your model and a list of predefined models with known ELOs
* will also write the results to the disk (same as RatingSystem)
* the results may not be accurate enough as the list of predefined models is small and has outliers
* the higher the number of matches played, the better ELO would be estimated