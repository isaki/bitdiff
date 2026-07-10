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
bitdiff <fileA> <fileB>

Options:
  -h [ --help ]            Print this message.
  -v [ --version ]         Display version information.
  -p [ --print-header ]    Add a header to the output.
  -f [ --fast ]            Disable flushing after each result line. Improves 
                           throughput when redirecting output.
  -m [ --output-mode ] arg The operating mode.

Output Modes:
  a : Bit difference format (default).
  b : Binary format.
  x : Hexadecimal format.
```
