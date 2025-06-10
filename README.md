# Building
## Linux and macOS

This requires:

- CMake v4.0 or newer
- Ninja or Make
- GCC or CLang with support for C++20
- Boost Program Options v1.83.0 or newer

```
cd <checkout location> && ./build.sh
```
The `bitdiff` application will be located in `<checkout location>/build/bin`.

# Usage
```
$ ./build/bin/bitdiff --help
bitdiff <file> <file>

Options:
  -h [ --help ]         print this message message
  -v [ --version ]      display version information
  --print-header        add a header to the output
```
