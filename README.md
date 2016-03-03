# c++ template strings.

A header-only set of c++ utilities for performing simple string interpolation similar to ES6 template strings/literals.

## Examples

#### std::string interpolation

    // the template
    const string templ = "The quick ${color} fox."

    // variables
    const unordered_map<string, string>& vars = {
        { "color", "brown" }
    };

    // interpolate and print to std out
    cout << tstrings::interpolate_braces(templ, vars);

## Requirements

- cmake `>= 2.8.4`
- A c++11 compatible compiler

## Tests

To run the tests, first configure with the cmake option `tstrings_cpp_WITH_TESTS` For example (with MSVC):

    $ mkdir build_tests && cd build_tests

    $ cmake .. -G "Visual Studio 12 2013 Win64" \
        -Dtstrings_cpp_WITH_TESTS:BOOL=on

### Build & Run Tests

    $ cd test && cmake --build .. --config Debug

    $ ctest . -VV -C Debug