#/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2025-2026 isaki

BUILD_DIR="./build"

cmake_cmd=$(which cmake 2> /dev/null)
if [ $? -ne 0 ]; then
    echo "Unable to locate cmake"
    exit 1
fi

build_cmd=$(which ninja 2> /dev/null)

if [ $? -eq 0 ]; then
    extra_opts=("-G" "Ninja")
else
    build_cmd=$(which make 2> /dev/null)
    if [ $? -ne 0 ]; then
        echo "Unable to locate ninja or make"
        exit 1
    fi

    extra_opts=("-G" "Unix Makefiles")
fi

if [ -d "${BUILD_DIR}" ]; then
    rm -rf "${BUILD_DIR}" || exit $?
fi

mkdir "${BUILD_DIR}" || exit $?

"${cmake_cmd}" "${extra_opts[@]}" -S . -B "${BUILD_DIR}" || exit $?
"${cmake_cmd}" --build "${BUILD_DIR}" -- -v

buildrc=$?
if [ $buildrc -ne 0 ]; then
    echo "Build failure!"
    exit $buildrc
fi

if [ "${BUILD_NO_STRIP}" = "1" ]; then
    echo "Binary strip disabled"
    exit 0
fi

# Strip targets
EXE="${BUILD_DIR}/bin/bitdiff"
if [ -f "${EXE}" ]; then
    osname=$(uname)
    if [ "${osname}" = "Darwin" ]; then
        echo "Stripping macOS executable"
        strip -u -r "${EXE}"
        buildrc=$?
    else
        echo "Stripping Linux executable"
        strip --strip-unneeded "${EXE}"
        buildrc=$?
    fi
else
    echo "Unable to locate ${EXE}"
    buildrc=1
fi

exit $buildrc
