#include "Engine.h"

#include <ego.hpp>
#include <ios>

#include "Config.h"
#include "Util.h"

Engine::Engine(const argos::config::Config &config) :
        _tree(std::make_unique<Tree>(config)),
        _komi(6.5),
        _config(config) {
    RegisterCommands();
    RegisterParams();

    _tc.setRemainingTime(config::engine::totalTime);
    _dbg.open(config.logFilePath.string(), ios_base::out | ios_base::app);
}

void Engine::RegisterCommands() {
    _gtp.Register("boardsize", this, &Engine::Cboardsize);
    _gtp.Register("clear_board", this, &Engine::Cclear_board);
    _gtp.Register("komi", this, &Engine::Ckomi);
    _gtp.Register("play", this, &Engine::Cplay);
    _gtp.Register("genmove", this, &Engine::Cgenmove);
    _gtp.Register("showboard", this, &Engine::Cshowboard);
    _gtp.Register("time_settings", this, &Engine::Ctime_settings);
    _gtp.Register("time_left", this, &Engine::Ctime_left);
}

void Engine::RegisterParams() {
    string tree = "param.tree";
    string other = "param.other";
    string set = "set";
}

void Engine::Cclear_board(Gtp::Io &io) {
    io.CheckEmpty();

    _tree = std::make_unique<Tree>(_config);
    _tree->setKomi(_komi);
    _tc.setRemainingTime(config::engine::totalTime);
}

void Engine::Cgenmove(Gtp::Io &io) {
    _dbg << "Total time left: " << format_duration(_tc.getRemainingTime()) << std::endl;
    const auto timeForMove = _tc.getTimeForMove(_tree->rootBoard());
    _dbg << "Evaluating in " << format_duration(timeForMove) << std::endl;

    _tc.timedAction([&]() {
#ifndef NDEBUG
        const Player player = io.Read<Player>();
        assert(player == _tree->rootBoard().ActPlayer());
#else
        io.Read<Player>();
#endif
        io.CheckEmpty();

        _tree->evaluate(timeForMove);
        const auto bestPosition = _tree->bestMove();
        Move move(_tree->rootBoard().ActPlayer(), bestPosition);
        const auto &rootNode = _tree->rootNode();
        const auto &winrate = rootNode->winrate(rootNode->position()->actPlayer());

        if ((winrate) < config::engine::resignThreshold) {
            printTree(_tree->rootNode().get(), _tree->rootBoard().ActPlayer(), _dbg);
            _dbg << "Current value: " << winrate * 100 << "%\n";
            _dbg << "Best move: " << move.ToGtpString() << "\n";
            _dbg << "resign" << std::endl;
            io.out << "resign";
        } else {
            printTree(_tree->rootNode().get(), _tree->rootBoard().ActPlayer(), _dbg);
            _dbg << _tree->rootBoard().ToAsciiArt(move.GetVertex()) << std::endl;
            _tree->playMove(bestPosition);
            _dbg << "Current value: " << winrate * 100 << "%\n";
            _dbg << "Best move: " << move.ToGtpString() << "\n";
            _dbg << _tree->rootBoard().ToAsciiArt(move.GetVertex()) << std::endl;

            io.out << (move.IsValid() ? move.GetVertex().ToGtpString() : "resign");
        }
    });
}

void Engine::Cboardsize(Gtp::Io &io) {
    int new_board_size = io.Read<int>();
    io.CheckEmpty();
    return;

    Cclear_board(io);
    if (board_size != new_board_size) {
        io.SetError("unacceptable size");
        return;
    }
}

void Engine::Ckomi(Gtp::Io &io) {
    float new_komi = io.Read<float>();
    io.CheckEmpty();

    _komi = new_komi;
    _tree->setKomi(_komi);
}

void Engine::Cplay(Gtp::Io &io) {
    Move move = io.Read<Move>();
    io.CheckEmpty();

    Board const &board = _tree->rootBoard();

    if (board.IsReallyLegal(move)) {
        _tree->playMove(move.GetVertex());
    } else {
        io.SetError("illegal move");
        return;
    }
}

void Engine::Cshowboard(Gtp::Io &io) {
    io.CheckEmpty();

    io.out << _tree->rootBoard().ToAsciiArt();
}

void Engine::Ctime_settings(Gtp::Io &io) {
    // TODO: implement different time controls

    _tc = BasicTimeControl(parseGtpTime(io));

    _dbg << "Total time set to " << format_duration(_tc.getRemainingTime()) << std::endl;

    io.CheckEmpty();
}

void Engine::Ctime_left(Gtp::Io &io) {
    _tc.setRemainingTime(parseGtpTime(io));

    _dbg << "Remaining time set to " << format_duration(_tc.getRemainingTime()) << std::endl;

    io.CheckEmpty();
}

ms Engine::parseGtpTime(Gtp::Io &io) {
    const int timeInSeconds = io.Read<int>();

    return std::chrono::seconds(timeInSeconds);
}
