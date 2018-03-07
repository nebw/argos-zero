#pragma once

#include <fstream>
#include <memory>

#include "TimeControl.h"
#include "Tree.h"
#include "Config.h"
#include "ego.hpp"
#include "gtp_gogui.hpp"

class Engine {
public:
    Engine(const argos::config::Config &config);

    Gtp::ReplWithGogui& getGtp() { return _gtp; }
    inline argos::config::Config const& configuration() const { return _config; }

private:
    void RegisterCommands();
    void RegisterParams();

    void Cclear_board(Gtp::Io& io);
    void Cgenmove(Gtp::Io& io);
    void Cboardsize(Gtp::Io& io);
    void Ckomi(Gtp::Io& io);
    void Cplay(Gtp::Io& io);
    void Cshowboard(Gtp::Io& io);
    void CShowTree(Gtp::Io& io);
    void Ctime_settings(Gtp::Io& io);
    void Ctime_left(Gtp::Io& io);

    ms parseGtpTime(Gtp::Io& io);

    Gtp::ReplWithGogui _gtp;
    BasicTimeControl _tc;

    std::unique_ptr<Tree> _tree;
    float _komi;

    ofstream _dbg;
    argos::config::Config _config;
};
