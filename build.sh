#/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 isaki

BUILD_DIR="./build"

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
cd "${BUILD_DIR}" || exit $?

cmake "${extra_opts[@]}" .. || exit $?
exec "${build_cmd}"
exit $?
