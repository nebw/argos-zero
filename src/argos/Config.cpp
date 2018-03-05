#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <chrono>

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

        bool parseDeviceType(const std::string &deviceType, MXNET_DEVICE_TYPE &parsed);

        Config parse(int argc, const char **argv)
        {
            po::options_description options;
            po::options_description required("required options");
            po::options_description optional("optional options");
            po::variables_map vm;

            required.add_options()
                    ("networkPath,p", po::value<std::string>()->required(), "path to trained model")
                    ("logfilePath,l", po::value<std::string>()->required(), "path where log file should be stored");

            //TODO: add better params description
            optional.add_options()
                    ("help,h", "print this message and exit")
                    ("config,c", po::value<std::string>(), "path to config file")
                    ("deviceType,d", po::value<std::string>(), "set device type (CPU, GPU, CPU_PINNED)")
                    ("boardSize,b", po::value<size_t>(), "set boardsize")
                    ("server,s", po::value<std::string>(), "set server ip, default is: 127.0.0.1")
                    ("port", po::value<int>(), "set port")
                    ("tree-batchSize", po::value<size_t>(), "set batchSize")
                    ("tree-numEvaluationThreads", po::value<size_t>(), "set numEvaluationThreads")
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
                    pt::ptree tree;

                    try
                    {
                        pt::xml_parser::read_xml(ifs, tree);
                        MXNET_DEVICE_TYPE deviceType;

                        auto configParser = tree.get_child("config");
                        configBuilder.boardSize(configParser.get<size_t>("board_size"));
                        if (parseDeviceType(configParser.get<std::string>("device_type"), deviceType))
                        {
                            configBuilder.deviceType(deviceType);
                        }
                        else
                        {
                            std::cout << "error: invalid device type" << std::endl;
                            exit(EXIT_FAILURE);
                        }

                        auto treeParser = configParser.get_child("tree");
                        treeBuilder.numThreads(treeParser.get<size_t>("num_threads"));
                        treeBuilder.randomizeFirstNMoves(treeParser.get<size_t>("randomize_first_n_moves"));
                        treeBuilder.numLastRootNodes(treeParser.get<size_t>("num_last_root_nodes"));
                        treeBuilder.virtualPlayouts(treeParser.get<size_t>("virtual_playouts"));
                        treeBuilder.expandAt(treeParser.get<size_t>("expand_at"));
                        treeBuilder.priorC(treeParser.get<float>("prior_c"));
                        treeBuilder.networkRollouts(treeParser.get<bool>("network_rollouts"));
                        treeBuilder.trainingMode(treeParser.get<bool>("training_mode"));

                        auto timeParser = configParser.get_child("time");
                        timeBuilder.C(timeParser.get<int>("c"));
                        timeBuilder.maxPly(timeParser.get<int>("max_ply"));
                        timeBuilder.delay(std::chrono::milliseconds(timeParser.get<size_t>("delay")));

                        auto engineParser = configParser.get_child("engine");
                        engineBuilder.resignThreshold(engineParser.get<float>("resign_threshold"));
                        engineBuilder.totalTime(std::chrono::milliseconds(engineParser.get<size_t>("total_time")));
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
                MXNET_DEVICE_TYPE deviceType;
                if (parseDeviceType(vm["deviceType"].as<std::string>(), deviceType))
                {
                    configBuilder.deviceType(deviceType);
                }
                else
                {
                    std::cout << "error: invalid device type" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            if (vm.count("boardSize")) configBuilder.boardSize(vm["boardSize"].as<size_t>());
            if (vm.count("server")) configBuilder.server(vm["server"].as<std::string>());
            if (vm.count("port")) configBuilder.port(vm["port"].as<int>());
            if (vm.count("boardSize")) configBuilder.boardSize(vm["boardSize"].as<size_t>());
            if (vm.count("tree-batchSize")) treeBuilder.batchSize(vm["tree-batchSize"].as<size_t>());
            if (vm.count("tree-numEvaluationThreads")) treeBuilder.numThreads(vm["tree-numEvaluationThreads"].as<size_t>());
            if (vm.count("tree-numThreads")) treeBuilder.numThreads(vm["tree-numThreads"].as<size_t>());
            if (vm.count("tree-randomizeFirstNMoves")) treeBuilder.randomizeFirstNMoves(vm["tree-randomizeFirstNMoves"].as<size_t>());
            if (vm.count("tree-numLastRootNodes")) treeBuilder.numLastRootNodes(vm["tree-numLastRootNodes"].as<size_t>());
            if (vm.count("tree-virtualPlayouts")) treeBuilder.virtualPlayouts(vm["tree-virtualPlayouts"].as<size_t>());
            if (vm.count("tree-expandAt")) treeBuilder.expandAt(vm["tree-expandAt"].as<size_t>());
            if (vm.count("tree-priorC")) treeBuilder.priorC(vm["tree-priorC"].as<float>());
            if (vm.count("tree-networkRollouts")) treeBuilder.networkRollouts(vm["tree-networkRollouts"].as<bool>());
            if (vm.count("tree-trainingMode")) treeBuilder.trainingMode(vm["tree-trainingMode"].as<bool>());
            if (vm.count("time-c")) timeBuilder.C(vm["time-c"].as<int>());
            if (vm.count("time-maxPly")) timeBuilder.maxPly(vm["time-maxPly"].as<int>());
            if (vm.count("time-delay")) timeBuilder.delay(std::chrono::milliseconds(vm["time-delay"].as<int>()));
            if (vm.count("engine-totalTime")) engineBuilder.totalTime(std::chrono::milliseconds(vm["engine-totalTime"].as<int>()));
            if (vm.count("engine-resignThreshold")) engineBuilder.resignThreshold(vm["engine-resignThreshold"].as<float>());

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

        bool parseDeviceType(const std::string &deviceType, MXNET_DEVICE_TYPE &parsed)
        {
            if (deviceType == "CPU") { parsed = CPU; return true; }
            else if (deviceType == "GPU") { parsed = GPU; return true; }
            else if (deviceType == "CPU_PINNED") { parsed = CPU_PINNED; return true; }
            else { return false; }
        }
    }
}