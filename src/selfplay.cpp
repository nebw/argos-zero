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

    bool collect_data = config.tree.trainingMode;
    boost::optional<Collector> collector;
    if (collect_data) { collector = Collector(config.server.c_str(), config.port); }

    bool noResignMode = false;
    float resignationThreshold = config.engine.resignThreshold;
    if (collect_data) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0., 1.);
        if (dis(gen) < 0.1f) {
            noResignMode = true;
            resignationThreshold = .01f;
        }
    }

    Tree tree(config);
    tree.setKomi(5.5);
    std::cout << tree.rootBoard().ToAsciiArt() << std::endl;

    Player winner = Player::Invalid();
    while ((!tree.rootBoard().BothPlayerPass())) {
        const auto visits = tree.rootNode()->position()->statistics().num_evaluations.load();
        tree.evaluate(std::min<size_t>(config.engine.selfplayRollouts * 2,
                                       visits + config.engine.selfplayRollouts));
        const auto winrate = tree.rootNode()->winrate(tree.rootBoard().ActPlayer());

        if ((winrate < resignationThreshold)) {
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
