#include "Config.h"
#include <boost/program_options.hpp>
#include <iostream>


ArgosConfig::Config ArgosConfig::initializeConfig(int argc, const char **argv) {

    std::string networkPath;
    std::string logFilePath;
    bool networkRollouts = ArgosConfig::Tree::DEFAULT_NETWORK_ROLLOUTS;
    namespace po = boost::program_options;

    po::options_description desc("usage: " + std::string(argv[0]) + " network_path [options]\noptions:");
    desc.add_options()
            ("help", "print this message and exit")
            ("network_path", po::value<std::string>()->required(), "path to trained model")
            ("logfile_path", po::value<std::string>()->required(), "path where logs should be stored")
            ("network_rollouts", po::value<bool>(&networkRollouts), "enable network rollouts");

    po::positional_options_description positionalOptions;
    // TODO network_path -> positional arg
    po::variables_map vm;

    po::store(po::command_line_parser(argc, argv)
                      .options(desc)
                      .positional(positionalOptions)
                      .run(), vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    po::notify(vm);

    networkPath = vm["network_path"].as<std::string>();
    logFilePath = vm["logfile_path"].as<std::string>();

    auto treeConfig = ArgosConfig::Tree::Builder()
            .networkRollouts(networkRollouts)
            .build();

    return ArgosConfig::Config::Builder(networkPath, logFilePath)
            .tree(treeConfig)
            .build();
}
