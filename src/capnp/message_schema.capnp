

struct StateProb {

    idx @0 :UInt16;                         # sequential index of game move starting at 0
    state @1 : List(Float32);               # the current state representing the board position
    probs @2 : List(Float32);               # probabilities for each legal move at the given state

}

struct Game {

    id @0 :UInt64;                          # global unique id of the game
    stateprobs @2 :List(StateProb);         # states and probabilities of the game
    timestamp @3 :Float64;                  # unix time stamp of the game

}
