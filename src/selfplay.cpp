#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include "argos/Collector.h"
#include "argos/Config.h"
#include "argos/TimeControl.h"
#include "argos/Tree.h"
#include "argos/Util.h"

int main(int argc, const char** argv) {
    auto config = argos::config::parse(argc, argv);

    // if network variable set: collect information
    bool collect_data = config.tree.trainingMode;

    const char* server = "127.0.0.1";
    int port = 1345;
    boost::optional<Collector> collector;

    if (collect_data) { collector = Collector(config.server.c_str(), config.port); }

    Tree tree(config);
    tree.setKomi(5.5);
    std::cout << tree.rootBoard().ToAsciiArt() << std::endl;

    float resignationThreshold = 0.025f;

    bool noResignMode = false;
    if (collect_data) {
        std::default_random_engine generator;
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        if (distribution(generator) < 0.1f) { noResignMode = true; }
    }

    Player winner = Player::Invalid();
    while ((!tree.rootBoard().BothPlayerPass())) {
        tree.evaluate(500);
        const auto winrate = tree.rootNode()->winrate(tree.rootBoard().ActPlayer());

        if ((winrate < resignationThreshold) && !(noResignMode)) {
            std::cout << tree.rootBoard().ActPlayer().ToGtpString() << " resigns." << std::endl;
            winner = tree.rootBoard().ActPlayer().Other();
            break;
        }

        const Vertex move = tree.bestMove();
        std::cout << "Chosen move: " << move.ToGtpString() << std::endl;
        printTree(tree.rootNode().get(), tree.rootBoard().ActPlayer());

        // fill capnp struct statprob for the move
        if (collect_data) { collector.get().collectMove(tree); }

        tree.playMove(move);
        std::cout << tree.rootBoard().ToAsciiArt(move) << std::endl;
    }

    if (!(winner.IsValid())) { winner = tree.rootBoard().TrompTaylorWinner(); }
    collector.get().collectWinner(winner);

    std::cout << "Winner: " << winner.ToGtpString() << std::endl;

    // convert what we collected to capnp messages
    // only possible here and not before because we do not know the number of moves in advance
    if (collect_data) { collector.get().sendData(tree, noResignMode); }

}

