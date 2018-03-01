#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include "argos/Config.h"

namespace
{
  const size_t ERROR_IN_COMMAND_LINE = 1;
  const size_t SUCCESS = 0;
  const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  try
  {
    std::string networkPath;
    std::string logFilePath;
    bool networkRollouts = argosConfig::globalConfig().tree.networkRollouts;

    /** Define and parse the program options
     */
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "Print help messages")
      ("networkPath", po::value<std::string>()->required(), "networkPath")
      ("logFilePath", po::value<std::string>()->required(), "logFilePath")
      ("networkRollouts", po::value<bool>(&networkRollouts), "networkRollouts");

    po::positional_options_description positionalOptions;

    po::variables_map vm;

    try
    {
      po::store(po::command_line_parser(argc, argv).options(desc)
                  .positional(positionalOptions).run(),
                vm); // throws on error

      /** --help option
       */
      if ( vm.count("help")  )
      {
        std::cout << "App Descrroption copy..."
                  << " print out paramters and description" << std::endl << std::endl;
        return SUCCESS;
      }

      po::notify(vm); // throws on error, so do after help in case
                      // there are any problems
    }
    catch(boost::program_options::required_option& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      return ERROR_IN_COMMAND_LINE;
    }
    catch(boost::program_options::error& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      return ERROR_IN_COMMAND_LINE;
    }

    networkPath = vm["networkPath"].as<std::string>();
    logFilePath = vm["logFilePath"].as<std::string>();

      argosConfig::globalConfig().networkPath = networkPath;
      argosConfig::globalConfig().logFilePath = logFilePath;
      argosConfig::globalConfig().tree.networkRollouts = networkRollouts;

    std::cout << argosConfig::globalConfig().networkPath << std::endl;
    std::cout << argosConfig::globalConfig().logFilePath << std::endl;
    std::cout << argosConfig::globalConfig().tree.networkRollouts << std::endl;
    std::cout << argosConfig::globalConfig().time.delay.count() << std::endl;

  }
  catch(std::exception& e)
  {
    std::cerr << "Unhandled Exception reached the top of main: "
              << e.what() << ", application will now exit" << std::endl;
    return ERROR_UNHANDLED_EXCEPTION;

  }

  return SUCCESS;

} // main
