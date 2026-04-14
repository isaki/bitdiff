/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025 isaki */

#include <iostream>
#include <string_view>

#include <boost/version.hpp>

#include "bitdiff_internal/config.hpp"
#include "bitdiff/version.hpp"

namespace bd = isaki::bitdiff;

namespace
{
    //  BOOST_VERSION % 100 is the patch level
    //  BOOST_VERSION / 100 % 1000 is the minor version
    //  BOOST_VERSION / 100000 is the major version
    constexpr int BOOST_VERSION_PATCH = BOOST_VERSION % 100;
    constexpr int BOOST_VERSION_MINOR = BOOST_VERSION / 100 % 1000;
    constexpr int BOOST_VERSION_MAJOR = BOOST_VERSION / 100000;
}

void bd::print_version(std::ostream& os, const std::string_view name)
{
    os << name
        << " ("
        << cmake::project_name
        << ") v"
        << cmake::project_version
        << std::endl;

    os << "Copyright (C) 2025 isaki@github" << std::endl;
    os << "License: GNU GPL version 2 or later <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>" << std::endl;
    os << name << " comes with NO WARRANTY, to the extent permitted by law" << std::endl;

    os << "Home page: <" << bd::cmake::project_url << '>' << std::endl;

    os << "Compiled with: "
        << cmake::cxx_compiler
        << " v"
        << cmake::cxx_compiler_ver
        << " for "
        << cmake::build_platform
        << ' '
        << cmake::build_architecture
        << std::endl;

    os << "Boost: "
        << BOOST_VERSION_MAJOR
        << '.'
        << BOOST_VERSION_MINOR
        << '.'
        << BOOST_VERSION_PATCH
        << std::endl;
}
