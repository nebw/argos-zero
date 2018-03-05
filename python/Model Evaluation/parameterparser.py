import argparse


class Parser():
    def __init__(self, type):
        parser = argparse.ArgumentParser(
            prog='runs a number of matches between two systems and rates both\n')

        parser.add_argument('--numMatches', type=lambda s: int(s) if int(s) > 0 else 1, default=1,
                            help='number of matches to be run (default 1)')

        parser.add_argument('--verbose', default="true",
                            help='should log? (default true)') 

        if type == "RatingSystem":
            parser.add_argument('--playerOne', type=lambda s: s.split(','), required=True,
                                help='path and launch parameters of first player seperated with commas')

            parser.add_argument('--playerTwo', type=lambda s: s.split(','), required=True,
                                help='path and launch parameters of second player seperated with commas')

            parser.add_argument('--batchSize', type=lambda s: int(s) if int(s) > 0 else 1, default=20,
                                help='number of matches to be used for evaluating the score (default 20)')

            parser.add_argument('--loadFrom', type=str, default= "ratings.txt",
                                help='file in local folder to copy ratings from (default ratings.txt), if no file is found default values of [1500, 350, 0.06] will be used')

        self.parser = parser

    def parse(self):
        args = self.parser.parse_args()
        args.verbose = args.verbose == "True" or args.verbose == "true"
        return args
