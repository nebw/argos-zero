#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "boost/optional.hpp"

#include <random>


#include "argos/Config.h"
#include "argos/TimeControl.h"
#include "argos/Tree.h"
#include "argos/Util.h"
#include "argos/Collector.h"

int main() {

    // if network variable set: collect information
    // if yes, where to send them (IP Adress:port)
    bool collect_data = 1;

    const char* server = "127.0.0.1";
    int port = 800;
    boost::optional<Collector> collector;

    if (collect_data){
        collector = Collector(server, port);
    }

    Tree tree;
    tree.setKomi(7.5);
    std::cout << tree.rootBoard().ToAsciiArt() << std::endl;

    NatMap<Player, BasicTimeControl> tc;
    tc[Player::Black()] = BasicTimeControl(config::engine::totalTime);
    tc[Player::White()] = BasicTimeControl(config::engine::totalTime);

    float resignationThreshold = 0.1f;

    bool noResignMode = false;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0,1.0);
    if (distribution(generator)<0.1f) {
        noResignMode = true;
    }

    while ((!tree.rootBoard().BothPlayerPass())) {
        const Player actPl = tree.rootBoard().ActPlayer();
        const auto time = tc[actPl].getTimeForMove(tree.rootBoard());
        std::cout << "Remaining time: " << format_duration(tc[actPl].getRemainingTime())
                  << std::endl;

        std::cout << "Evaluating in " << format_duration(time) << std::endl;
        tc[actPl].timedAction([&]() { tree.evaluate(time); });

        std::cout << "Best move: " << tree.bestMove().ToGtpString() << std::endl;
        const auto winrate = tree.rootNode()->winrate(tree.rootBoard().ActPlayer());

        if ((winrate < resignationThreshold) && !(noResignMode)){
            std::cout << tree.rootBoard().ActPlayer().ToGtpString() << " resigns." << std::endl;
            break;
        }

        const Vertex move = tree.bestMove();

        printTree(tree.rootNode().get(), tree.rootBoard().ActPlayer());

        // fill capnp struct statprob for the move
        if (collect_data){
            collector.get().collectMove(tree);
        }


        tree.playMove(move);

        std::cout << tree.rootBoard().ToAsciiArt(move) << std::endl;


        // TODO: Append new board state and predictions to Game list

    }

    // convert what we collected to capnp messages
    // only possible here and not before because we do not know the number of moves in advance
    if (collect_data){
        collector.get().sendData(tree);
    }


    //TODO: Export game

}

// TODO: Export Game
