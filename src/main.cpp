/* Copyright 2024 isaki */

#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include <cstdint>

#include "boost/program_options.hpp"
#include "bitdiff_internal/config.hpp"

#include "bitdiff/bitdiff.hpp"

namespace po = boost::program_options;
namespace bd = isaki::bitdiff;

namespace
{
    inline constexpr size_t BUFFER_SIZE = 4 * 1024 * 128;

    std::string _argv_basename(const char * name)
    {
        const std::string_view tmp(name);
        const std::filesystem::path p(tmp);
        return p.filename().string();
    }
}

int main(int argc, char ** argv)
{ 
    try
    {
        // Declare the supported options.
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "print this message message")
            ("version,v", "display version information")
        ;

        po::options_description hidden("Hidden options");
        hidden.add_options()
            ("fileA", po::value<std::string>(), "The file A to diff")
            ("fileB", po::value<std::string>(), "The file B to diff")
        ;

        po::options_description all;
        all.add(desc).add(hidden);

        po::positional_options_description posdesc;
        posdesc.add("fileA", 1);
        posdesc.add("fileB", 2);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(all).positional(posdesc).run(), vm);
        po::notify(vm);

        if (vm.count("version"))
        {
            const std::string name = _argv_basename(argv[0]);
            std::cout << name
                << " v"
                << isaki::bitdiff::cmake::project_version
                << std::endl;

            return 0;
        }

        if (vm.count("help")) {
            const std::string name = _argv_basename(argv[0]);
            std::cout << name << " <file> <file>" << std::endl << std::endl;
            std::cout << desc << std::endl;
            return 1;
        }

        if (!vm.count("fileA") || !vm.count("fileB"))
        {
            std::cerr << "Invalid usage" << std::endl;
            return 1;
        }

        const std::string fileA = vm["fileA"].as<std::string>();
        const std::string fileB = vm["fileB"].as<std::string>();

        if (fileA == fileB)
        {
            std::cerr << "File A and B are the same path" << std::endl;
            return 0;
        }

        std::cerr << "Initializing diff object" << std::endl;

        bd::BitDiff diff(fileA, fileB, BUFFER_SIZE);

        std::cerr << "Size " << fileA << ": " << diff.getFileASize() << std::endl;
        std::cerr << "Size " << fileB << ": " << diff.getFileBSize() << std::endl;

        const uintmax_t diffCount = diff.process(std::cout);
        std::cerr << diffCount << " difference";

        if (diffCount != 1)
        {
            std::cerr << "s";
        }

        std::cerr << " found" << std::endl;

        if (diffCount == 0)
        {
            return 0;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 10;
    }

    // Differences found.
    return 1;
}
