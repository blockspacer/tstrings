# tstrings: c++ template strings

Header-only utilities for performing simple string interpolation; compatible with [ES6 template strings/literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals) (but without arbitrary expression evaluation).

## Examples

#### std::string interpolation

```c++
// the template
const string templ = "The quick ${color} fox.";

// variables
const std::unordered_map<string, string> vars = {
    { "color", "brown" }
};

// interpolate and print to std out
cout << tstrings::interpolate_braces(templ, vars);
```

#### Numeric interpolation

```c++
// the template
const string templ = "The ${0} ${1} fox.";

// variables
const std::vector<string> vars = { "quick", "brown" };

// interpolate and print to cout
std::cout << tstrings::interpolate_braces(templ, vars);
```

#### Interpolate a stream

```c++
// variables
const std::unordered_map<string, string> vars = {
    { "username", "..." },
    { "DOB", "..." }
};

// open file
std::ifstream template_file("my_template.html");

// pipe the template file to the output stream,
// interpolating variables
tstrings::interpolate_braces(vars, std::cout) 
    << template_file.rdbuf() 
    << std::flush;
```

## Requirements

- cmake `>= 2.8.4`
- A c++11 compatible compiler

## Tests

To run the tests, first configure with the cmake option `tstrings_cpp_WITH_TESTS` For example (with MSVC):

```cmake
$ mkdir build_tests && cd build_tests

$ cmake .. -G "Visual Studio 12 2013 Win64" \
    -Dtstrings_cpp_WITH_TESTS:BOOL=on
```

#### Build & Run Tests

```cmake
$ cd test && cmake --build .. --config Debug

$ ctest . -VV -C Debug
```