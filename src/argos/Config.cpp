#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

#include "Config.h"

namespace argos
{
    namespace config
    {
        namespace fs = boost::filesystem;
        namespace po = boost::program_options;

        std::string usage(const char *programPath,
                          const po::options_description &required,
                          const po::options_description &optional);

        Config parse(int argc, const char **argv)
        {
            po::options_description options;
            po::options_description required("required options");
            po::options_description optional("optional options");
            po::variables_map vm;

            required.add_options()
                    ("networkPath,p", po::value<std::string>()->required(), "path to trained model")
                    ("logfilePath,l", po::value<std::string>()->required(), "path where log file should be stored");

            optional.add_options()
                    ("help,h", "print this message and exit")
                    ("config,c", po::value<std::string>(), "path to config file")
                    ("tree-networkRollouts", po::value<bool>(), "enable|disable network rollouts");

            options
                    .add(required)
                    .add(optional);
            try
            {
                po::store(po::command_line_parser(argc, argv)
                                  .options(options)
                                  .run(), vm);

                if (vm.count("help")) {
                    std::cout << usage(argv[0], required, optional) << std::endl << options << std::endl;
                    exit(EXIT_SUCCESS);
                }

                po::notify(vm);
            }
            catch (po::required_option&)
            {
                std::cout << usage(argv[0], required, optional) << std::endl << options << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (po::error& e)
            {
                std::cout << "error: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }

            auto configBuilder = Config::Builder(vm["networkPath"].as<std::string>(),
                                                 vm["logfilePath"].as<std::string>());
            auto treeBuilder = Tree::Builder();
            auto timeBuilder = Time::Builder();
            auto engineBuilder = Engine::Builder();

            if (vm.count("config"))
            {
                fs::path configPath(vm["config"].as<std::string>());

                if (fs::exists(configPath))
                {
                    // TODO parse config file
                }
                else
                {
                    std::cout << "error: invalid configuration path" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            if (vm.count("tree-networkRollouts")) treeBuilder.networkRollouts(vm["tree-networkRollouts"].as<bool>());

            return configBuilder
                    .tree(treeBuilder.build())
                    .time(timeBuilder.build())
                    .engine(engineBuilder.build())
                    .build();
        }

        std::string usage(const char *programPath,
                          const po::options_description &required,
                          const po::options_description &optional)
        {
            std::stringstream ss;
            std::string programName = fs::path(programPath).stem().string();

            ss << "usage: "
               << programName
               << (required.options().empty() ? "" : " [required options]")
               << (optional.options().empty() ? "" : " [optional options]");

            return ss.str();
        }
    }
}