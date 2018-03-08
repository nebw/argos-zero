import argparse


class Parser():
    def __init__(self, type):
        parser = argparse.ArgumentParser(
            prog='runs a number of matches between two systems and rates both\n make sure that the players parameters are comma seperated! ex: --playerOne gnugo,--mode,gtp,--boeardsie,9 ')

        parser.add_argument('--numMatches', type=lambda s: int(s) if int(s) > 0 else 1, default=1,
                            help='number of matches to be run (default 1)')

        parser.add_argument('--verbose', default="true",
                            help='should log? (default true)') 

        if type == "RatingSystem":
            parser.add_argument('--playerOne', type=lambda s: s.split(','), required=True,
                                help='path and launch parameters of first player seperated with commas')

            parser.add_argument('--playerTwo', type=lambda s: s.split(','), required=True,
                                help='path and launch parameters of second player seperated with commas')

            parser.add_argument('--loadFromFile', type=str, default= "ratings",
                                help='file in local folder to copy ratings from (default: "ratings"), if no file is found default values of [1000, 40] will be used')

            parser.add_argument('--loadFromPath', type=str, default="./rating",
                                help='folder in which the ratings file defined in the --loadFromFile parameter exists (default "./rating")')
<<<<<<< HEAD

        if type == "EstimateElo":
          parser.add_argument('--newPlayer', type=lambda s: s.split(','), required=True,help='path and launch parameters of the to be evaluated engine seperated with commas')
=======
>>>>>>> 70bc59669a3a3b825bf208839e4cdaf06b84bb22

        self.parser = parser

    def parse(self):
        args = self.parser.parse_args()
        args.verbose = args.verbose == "True" or args.verbose == "true"
        return args
