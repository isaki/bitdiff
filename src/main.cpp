/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>

// ReSharper disable once CppUnusedIncludeDirective
#include <cstddef>

#include <boost/program_options.hpp>

#include "bitdiff/dataout.hpp"
#include "bitdiff/bitdiff.hpp"
#include "bitdiff/version.hpp"

namespace po = boost::program_options;
namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    // Default to a 64k buffer
    constexpr std::size_t READ_BUFFER_LENGTH = 64 * 1024;

    std::string argv_basename(const char* name)
    {
        const std::string_view tmp(name);
        const fs::path p(tmp);
        return p.filename().string();
    }

    void print_help(std::ostream& os, const std::string_view name, const po::options_description& desc)
    {
        os << name << " <fileA> <fileB>" << std::endl << std::endl;
        os << desc << std::endl;

        os << "Output Modes:" << std::endl;
        os << "  a : Bit difference format (default)." << std::endl;
        os << "  b : Binary format." << std::endl;
        os << "  x : Hexadecimal format." << std::endl;
    }
}

int main(int argc, char** argv)
{
    // Improve performance for high-volume output.
    // Safe because we only use C++ streams for output (no mixing with C stdio).
    std::ios_base::sync_with_stdio(false);
    std::cout.tie(nullptr);

    try
    {
        // Declare the supported options.
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "Print this message.")
            ("version,v", "Display version information.")
            ("print-header,p", "Add a header to the output.")
            ("fast,f", "Disable flushing after each result line. Improves throughput when redirecting output.")
            ("output-mode,m", po::value<char>(), "The operating mode.")
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

        if (vm.contains("version"))
        {
            const std::string name = argv_basename(argv[0]);
            bd::print_version(std::cout, name);
            return 0;
        }

        if (vm.contains("help")) {
            const std::string name = argv_basename(argv[0]);
            print_help(std::cout, name, desc);
            return 0;
        }

        bd::DataOutType dataType;
        if (vm.contains("output-mode"))
        {
            switch (const char mode = vm["output-mode"].as<char>())
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

        if (!vm.contains("fileA") || !vm.contains("fileB"))
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

        bd::BitDiff diff(fileA, fileB, READ_BUFFER_LENGTH, vm.contains("fast"));

        std::cerr << "Size " << fileA << ": " << diff.getFileASize() << std::endl;
        std::cerr << "Size " << fileB << ": " << diff.getFileBSize() << std::endl;

        const bd::diff_count dcount = diff.process(std::cout, vm.contains("print-header"), dataType);

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
