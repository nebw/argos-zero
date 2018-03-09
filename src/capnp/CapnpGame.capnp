@0xc6310a89d98447e4;

struct StateProb {

    idx @0 :UInt16;                         # sequential index of game move starting at 0
    state @1 :List(Float32);                # the current state representing the board position, flattended array
    probs @2 :List(Float32);                # probabilities for each legal move at the given state
    winrate @3 :Float32;                    # win rate at given node

}

struct Game {

    id @0 :Text;                            # global unique id of the game in text format
    stateprobs @1 :List(StateProb);         # states and probabilities of the game
    timestamp @2 :Float64;                  # unix time stamp of the game
    result @3 :Bool;                        # 1 black wins, 0 white wins
    network1 @4 :Text;                      # network that played player one in the selfplay
    network2 @5 :Text;                      # network that played player two in the selfplay
    boardsize @6: UInt8;                    # size of the board, 19 in real go, anything smaller to speed up training
    noresignmode @7: Bool;                  # is resignation allowed during the game

}
