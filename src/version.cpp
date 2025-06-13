/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <iostream>
#include <string_view>

#include "bitdiff_internal/config.hpp"

#include "bitdiff/version.hpp"

namespace bd = isaki::bitdiff;

void bd::print_version(std::ostream& os, const std::string_view name)
{
    os << name
        << " ("
        << bd::cmake::project_name
        << ") v"
        << bd::cmake::project_version
        << std::endl;

    os << "Copyright (C) 2025 isaki@github" << std::endl;
    os << "License: Apache Version 2.0 <https://www.apache.org/licenses/LICENSE-2.0>" << std::endl;
    os << name << " comes with NO WARRANTY, to the extent permitted by law" << std::endl;

    os << "Home page: <" << bd::cmake::project_url << '>' << std::endl;

    os << "Compiled with: "
        << bd::cmake::cxx_compiler
        << " v"
        << bd::cmake::cxx_compiler_ver << std::endl;

    os << "Written by isaki@github" << std::endl;
}
