#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "argos/Config.h"
#include "argos/TimeControl.h"
#include "argos/Tree.h"
#include "argos/Util.h"
#include "argos/Collector.h"

int main() {

    // if network variable set: collect information
    // if yes, where to send them (IP Adress:port)
    bool collect_data = 0;

    string server = "hallo";
    int port = 80;
    Collector collector;

    if (collect_data){
        collector = Collector(server, port);
    }

    Tree tree;
    tree.setKomi(7.5);
    std::cout << tree.rootBoard().ToAsciiArt() << std::endl;

    NatMap<Player, BasicTimeControl> tc;
    tc[Player::Black()] = BasicTimeControl(config::engine::totalTime);
    tc[Player::White()] = BasicTimeControl(config::engine::totalTime);


    while ((!tree.rootBoard().BothPlayerPass())) {
        const Player actPl = tree.rootBoard().ActPlayer();
        const auto time = tc[actPl].getTimeForMove(tree.rootBoard());
        std::cout << "Remaining time: " << format_duration(tc[actPl].getRemainingTime())
                  << std::endl;

        std::cout << "Evaluating in " << format_duration(time) << std::endl;
        tc[actPl].timedAction([&]() { tree.evaluate(time); });

        std::cout << "Best move: " << tree.bestMove().ToGtpString() << std::endl;
        const auto winrate = tree.rootNode()->winrate(tree.rootBoard().ActPlayer());
        if (winrate < .1f) {
            std::cout << tree.rootBoard().ActPlayer().ToGtpString() << " resigns." << std::endl;
            break;
        }
        const Vertex move = tree.bestMove();

        printTree(tree.rootNode().get(), tree.rootBoard().ActPlayer());

        // fill capnp struct statprob for the move
        if (collect_data){
            collector.collectMove(&tree);
        }


        tree.playMove(move);

        std::cout << tree.rootBoard().ToAsciiArt(move) << std::endl;
    }

    // convert what we collected to capnp messages
    // only possible here and not before because we do not know the number of moves in advance
    if (collect_data){
        collector.sendData(&tree);
    }

}
