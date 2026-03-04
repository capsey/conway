#include "options.hpp"

#include <SFML/Config.hpp>
#include <iostream>
#include <syncstream>

Options::Options(int argc, char *argv[]) : m_executable(argv[0])
{
    bool readingOptions = true;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.empty())
            continue;

        if (readingOptions && arg[0] == '-')
        {
            if (arg == "--")
            {
                readingOptions = false;
                continue;
            }

            if (arg == "-h" || arg == "--help")
            {
                help = true;
                continue;
            }

            if (arg == "-v" || arg == "--version")
            {
                version = true;
                continue;
            }

            if (arg == "--info")
            {
                info = true;
                continue;
            }

            if (arg == "--debug")
            {
                debug = true;
                continue;
            }

            if (arg == "--benchmark")
            {
                benchmark = true;
                continue;
            }

            throw Error("Unknown option '" + arg + "'.", m_executable);
        }
    }
}

void Options::printHelp()
{
    std::osyncstream stream(std::cerr);
    stream << "Usage: " << m_executable << " [OPTIONS]\n";
    stream << "\n";
    stream << "Conway's Game of Life.\n";
    stream << "\n";
    stream << "Options:\n";
    stream << "  -h, --help       Show this help message and exit\n";
    stream << "  -v, --version    Show version and exit\n";
    stream << "  --info           Show more logging information\n";
    stream << "  --debug          Show debugging information\n";
    stream << "  --               Stop parsing options (treat following arguments as filename)\n";
}

void Options::printVersion()
{
    std::osyncstream stream(std::cout);
    stream << "conway " << CONWAY_VERSION_STRING << "\n";
    stream << "sfml " << SFML_VERSION_MAJOR << "." << SFML_VERSION_MINOR << "." << SFML_VERSION_PATCH << "\n";
}

LogLevel Options::getLogLevel() const
{
    LogLevel level = LogLevel::Warning;

    if (info)
        level = LogLevel::Info;

    if (debug)
        level = LogLevel::Debug;

    return level;
}
