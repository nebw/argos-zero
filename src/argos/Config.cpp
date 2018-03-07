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
                          const po::options_description &optional,
                          const po::positional_options_description& positional);

        bool parseDeviceType(const std::string &deviceType, MXNET_DEVICE_TYPE &parsed);

        Config parse(int argc, const char **argv)
        {
            po::options_description options;
            po::options_description required("required options");
            po::options_description optional("optional options");
            po::positional_options_description positional;
            po::variables_map vm;

            required.add_options()
                    ("networkPath,n", po::value<std::string>()->required(), "path to trained model");

            //TODO: add better params description
            optional.add_options()
                    ("help,h", "print this message and exit")
                    ("config,c", po::value<std::string>(), "path to config file")
                    ("logfilePath,l", po::value<std::string>(), "path where log file should be stored\n"
                            "by default logs are stored in `<timestamp>.log` file in the same directory as network")
                    ("deviceType,d", po::value<std::string>(), "set device type (CPU, GPU, CPU_PINNED)")
                    ("server,s", po::value<std::string>(), "set server ip, default is: 127.0.0.1")
                    ("port,p", po::value<int>(), "set port")
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

            positional.add("networkPath", 1);

            options
                    .add(required)
                    .add(optional);
            try
            {
                po::store(po::command_line_parser(argc, argv)
                                  .options(options)
                                  .positional(positional)
                                  .run(), vm);

                if (vm.count("help")) {
                    std::cout << usage(argv[0], optional, positional)
                              << std::endl << options << std::endl;
                    exit(EXIT_SUCCESS);
                }

                po::notify(vm);
            }
            catch (po::required_option&)
            {
                std::cout << usage(argv[0], optional, positional)
                          << std::endl << options << std::endl;
                exit(EXIT_FAILURE);
            }
            catch (po::error& e)
            {
                std::cout << "error: " << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }

            auto networkPath = fs::path(vm["networkPath"].as<std::string>());
            auto currentTimestamp = std::chrono::system_clock::now().time_since_epoch().count();
            auto defaultLogfileName = fs::path(std::to_string(currentTimestamp) + ".log");
            auto logfilePath = vm.count("logfilePath") ? fs::path(vm["logfilePath"].as<std::string>())
                                                       : networkPath.parent_path() / defaultLogfileName;

            auto configBuilder = Config::Builder(networkPath, logfilePath);
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
                        // TODO note: logfilePath will not be parsed from config file
                        configBuilder.server(configParser.get<std::string>("server"));
                        configBuilder.port(configParser.get<int>("port"));

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
                        treeBuilder.batchSize(treeParser.get<size_t>("batch_size"));
                        treeBuilder.numEvaluationThreads(treeParser.get<size_t>("num_evaluation_threads"));
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
            else
            {
                if (vm.count("tree-numThreads"))
                {
                    treeBuilder.numThreads(vm["tree-numThreads"].as<size_t>());
                }
                else
                {
                    auto numThreads = std::max<size_t>(
                            Tree::DEFAULT_NUM_EVALUATION_THREADS * Tree::DEFAULT_BATCH_SIZE,
                            std::thread::hardware_concurrency() ? std::thread::hardware_concurrency()
                                                                : Tree::DEFAULT_NUM_THREADS);
                    treeBuilder.numThreads(numThreads);
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

            if (vm.count("server")) configBuilder.server(vm["server"].as<std::string>());
            if (vm.count("port")) configBuilder.port(vm["port"].as<int>());
            /* deviceType defined above */
            if (vm.count("tree-batchSize")) treeBuilder.batchSize(vm["tree-batchSize"].as<size_t>());
            if (vm.count("tree-numEvaluationThreads")) treeBuilder.numThreads(vm["tree-numEvaluationThreads"].as<size_t>());
            /* tree-numThreads defined above */
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

            auto config = configBuilder
                    .tree(treeBuilder.build())
                    .time(timeBuilder.build())
                    .engine(engineBuilder.build())
                    .build();
#ifdef DEBUG
            using namespace std;
            cout << "<<<<<<<<< CONFIG DEBUG" << endl;
            cout << "config.networkPath: " << config.networkPath << endl;
            cout << "config.logFilePath: " << config.logFilePath << endl;
            cout << "config.deviceType: " << config.deviceType << endl;
            cout << "config.boardSize: " << config.boardSize << endl;
            cout << "config.server: " << config.server << endl;
            cout << "config.port: " << config.port << endl;
            cout << "config.tree.batchSize: " << config.tree.batchSize << endl;
            cout << "config.tree.numEvaluationThreads: " << config.tree.numEvaluationThreads << endl;
            cout << "config.tree.numThreads: " << config.tree.numThreads << endl;
            cout << "config.tree.randomizeFirstNMoves: " << config.tree.randomizeFirstNMoves << endl;
            cout << "config.tree.numLastRootNodes: " << config.tree.numLastRootNodes << endl;
            cout << "config.tree.virtualPlayouts: " << config.tree.virtualPlayouts << endl;
            cout << "config.tree.expandAt: " << config.tree.expandAt << endl;
            cout << "config.tree.priorC: " << config.tree.priorC << endl;
            cout << "config.tree.networkRollouts: " << config.tree.networkRollouts << endl;
            cout << "config.tree.trainingMode: " << config.tree.trainingMode << endl;
            cout << "config.time.C: " << config.time.C << endl;
            cout << "config.time.maxPly: " << config.time.maxPly << endl;
            cout << "config.time.delay: " << config.time.delay.count() << endl;
            cout << "config.engine.totalTime: " << config.engine.totalTime.count() << endl;
            cout << "config.engine.resignThreshold: " << config.engine.resignThreshold << endl;
            cout << ">>>>>>>>> END CONFIG DEBUG" << endl;
#endif
            return config;
        }

        std::string usage(const char *programPath,
                          const po::options_description &optional,
                          const po::positional_options_description& positional)
        {
            std::stringstream ss;
            std::string programName = fs::path(programPath).stem().string();

            ss << "usage: "
               << programName;

            for (unsigned i = 0; i < positional.max_total_count(); i++)
                ss << " " << positional.name_for_position(i);

            ss << (optional.options().empty() ? "" : " [optional options]");

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