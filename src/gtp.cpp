#include "ego.hpp"
#include "argos/Config.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "argos/Engine.h"
#include "sgf.hpp"

int main(int argc, const char** argv) {
    auto config = argos::config::parse(argc, argv);

    // no buffering to work well with gogui
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);

    //std::cerr.setstate(std::ios_base::failbit);
    Engine engine(config);
    //std::cerr.clear();
    //setbuf(stderr, nullptr);
    Gtp::ReplWithGogui& gtp = engine.getGtp();

    gtp.RegisterStatic("name", "Argos Zero");
    gtp.RegisterStatic("version", "0.1");
    gtp.RegisterStatic("protocol_version", "2");

    gtp.Run(cin, cout);
    /*
    reps(ii, 1, argc) {
        if (ii == argc - 1 && string(argv[ii]) == "gtp") continue;
        string response;
        switch (gtp.RunOneCommand(argv[ii], &response)) {
            case Gtp::Repl::Success:
                std::cerr << response << std::endl;
                break;
            case Gtp::Repl::Failure:
                std::cerr << "Command: \"" << argv[ii] << "\" failed." << std::endl;
                return 1;
            case Gtp::Repl::NoOp:
                break;
            case Gtp::Repl::Quit:
                std::cerr << response << std::endl;
                return 0;
        }
    }

    if (argc == 1 || string(argv[argc - 1]) == "gtp") { gtp.Run(cin, cout); }
     */
}
