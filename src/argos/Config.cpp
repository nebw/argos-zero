#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

#include "Config.h"

namespace argos
{
    namespace config
    {
        namespace fs = boost::filesystem;
        namespace po = boost::program_options;
        namespace pt = boost::property_tree;

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
                    ("deviceType,d", po::value<std::string>(), "set device type (CPU, GPU, CPU_PINNED)")
                    ("boardSize,b", po::value<size_t>(), "set boardsize")
                    ("tree-numThreads", po::value<size_t>(), "set numThreads")
                    ("tree-randomizeFirstNMoves", po::value<size_t>(), "set randomizeFirstNMoves")
                    ("tree-numLastRootNodes", po::value<size_t>(), "set numLastRootNodes")
                    ("tree-virtualPlayouts", po::value<size_t>(), "set virtualPlayouts")
                    ("tree-expandAt", po::value<size_t>(), "set expandAt")
                    ("tree-priorC", po::value<float>(), "set priorC")
                    ("tree-networkRollouts", po::value<bool>(), "enable|disable network rollouts")
                    ("tree-trainingMode", po::value<bool>(), "enable|disable training mode")
                    ("time-c", po::value<int>(), "set time c")
                    ("time-maxPly", po::value<int>(), "set maxPly")
                    ("time-delay", po::value<int>(), "set delay")
                    ("engine-totalTime", po::value<int>(), "set totalTime in milliseconds")
                    ("engine-resignThreshold", po::value<float>(), "set resignThreshold");

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
                    fs::ifstream ifs(configPath);
                    pt::ptree pTree;

                    try
                    {
                        pt::xml_parser::read_xml(ifs, pTree);
                        pTree = pTree.get_child("config");
                        auto treeParser = pTree.get_child("tree");
                        treeBuilder.networkRollouts(treeParser.get<bool>("network_rollouts"));
                    }
                    catch (pt::ptree_error&)
                    {
                        std::cout << "error: parsing configuration file failed" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cout << "error: invalid configuration path" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            if (vm.count("deviceType"))
            {
                using namespace std;
                std::string deviceType = vm["deviceType"].as<std::string>();
                if (deviceType.compare("CPU")) { configBuilder.deviceType(CPU); }
                else if (deviceType.compare("GPU")) { configBuilder.deviceType(GPU); }
                else if (deviceType.compare("CPU_PINNED")) { configBuilder.deviceType(CPU_PINNED); }
                else
                {
                    std::cout << "error: invalid device type: " << vm.count("deviceType") << std::endl;
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