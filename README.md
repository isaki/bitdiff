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
$ bitdiff --help
bitdiff <file> <file>

Options:
  -h [ --help ]            print this message message
  -v [ --version ]         display version information
  --print-header           add a header to the output
  -m [ --output-mode ] arg The operating mode

Output Modes:
  a : Output the byte differences in bit difference format (default).
  b : Output the byte differences in binary format.
  x : Output the byte differences in hexidecimal format.
```
