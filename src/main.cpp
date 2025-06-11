/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include <cstdint>

#include "boost/program_options.hpp"
#include "bitdiff_internal/config.hpp"

#include "bitdiff/dataout.hpp"
#include "bitdiff/bitdiff.hpp"

namespace po = boost::program_options;
namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    inline constexpr size_t BUFFER_SIZE = 4 * 1024 * 128;

    std::string _argv_basename(const char * name)
    {
        const std::string_view tmp(name);
        const fs::path p(tmp);
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
            ("print-header", "add a header to the output")
            ("output-mode,m", po::value<char>(), "The operating mode")
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
                << " ("
                << bd::cmake::project_name
                << ") v"
                << bd::cmake::project_version
                << std::endl;

            std::cout << "Copyright (C) 2025 isaki@github" << std::endl;
            std::cout << "License: Apache Version 2.0 <https://www.apache.org/licenses/LICENSE-2.0>" << std::endl;

            std::cout << "Compiled with: "
                << bd::cmake::cxx_compiler
                << " v"
                << bd::cmake::cxx_compiler_ver << std::endl;

            return 0;
        }

        if (vm.count("help")) {
            const std::string name = _argv_basename(argv[0]);
            std::cout << name << " <file> <file>" << std::endl << std::endl;
            std::cout << desc << std::endl;

            std::cout << "Output Modes:" << std::endl;
            std::cout << "  a : Output the byte differences in bit difference format (default)." << std::endl;
            std::cout << "  b : Output the byte differences in binary format." << std::endl;
            std::cout << "  x : Output the byte differences in hexidecimal format." << std::endl;

            return 0;
        }

        bd::DataOutType dataType;
        if (vm.count("output-mode"))
        {
            const char mode = vm["output-mode"].as<char>();
            switch (mode)
            {
                case 'a':
                    dataType = bd::DataOutType::Bits;
                    break;

                case 'b':
                    dataType = bd::DataOutType::Binary;
                    break;
                
                case 'x':
                    dataType = bd::DataOutType::Hex;
                    break;
                
                default:
                    std::cerr << "Invalid output-mode: " << mode << std::endl;
                    return 1;
            }
        }
        else
        {
            dataType = bd::DataOutType::Bits;
        }

        if (!vm.count("fileA") || !vm.count("fileB"))
        {
            std::cerr << "Invalid usage; please run with --help" << std::endl;
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

        const bd::diff_count dcount = diff.process(std::cout, vm.count("print-header"), dataType);

        std::cerr << "Found " << dcount.bits << " bit difference";
        if (dcount.bits != 1)
        {
            std::cerr << "s";
        }

        std::cerr << " across " << dcount.bytes << " byte";
        if (dcount.bytes != 1)
        {
            std::cerr << "s";
        }

        std::cerr << std::endl;

        if (dcount.bytes == 0)
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
    return 11;
}
