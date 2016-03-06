#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "tstrings_cpp/tstrings.h"

#include <string>
#include <unordered_map>
#include <fstream>

using std::string;
using std::wstring;
using std::u16string;
using std::u32string;

namespace templates
{
    const string no_vars     = "Hello World!";
    const string fox         = "The quick ${color} fox.";
    const string fox_utf8    = u8"The quiĉk ${воасл} fox.";
    const string fox_spaces  = "The quick ${ color } fox.";
    const string fox_numeric = "The ${0} ${1} fox.";
    const string fox_numeric_invalid = "The ${123} ${456} ${abcd} fox.";
    const wstring fox_wstr   = L"The quiĉk ${cȌlor} fox.";
}

namespace strings
{
    const string tqbf = "The quick brown fox.";
}

namespace files
{
    const string small_utf8 = "data/small_utf8.template";
    const string small_utf8_expect = "data/small_utf8.expected";
}

TEST(tstrings, interpolate_empty)
{
    const std::unordered_map<string, string> vars = {};
    EXPECT_EQ("", tstrings::interpolate_braces<string>("", vars));
}

TEST(tstrings, interpolate_no_vars)
{
    const std::unordered_map<string, string> vars = {};
    EXPECT_EQ(templates::no_vars, tstrings::interpolate_braces(templates::no_vars, vars));
}

TEST(tstrings, interpolate)
{
    const std::unordered_map<string, string> vars = {
        { "color", "brown" }
    };

    EXPECT_EQ(
        strings::tqbf, 
        tstrings::interpolate_braces(templates::fox, vars)
    );
}

TEST(tstrings, interpolate_utf8)
{
    const std::unordered_map<string, string> vars = {
        { u8"воасл", u8"brʘwn" }
    };

    EXPECT_EQ(
        u8"The quiĉk brʘwn fox.", 
        tstrings::interpolate_braces(templates::fox_utf8, vars)
    );
}

TEST(tstrings, interpolate_wstr)
{
    const std::unordered_map<wstring, wstring> vars = {
        { L"cȌlor", L"brʘwn" }
    };

    EXPECT_EQ(
        L"The quiĉk brʘwn fox.", 
        tstrings::interpolate_braces<wstring>(templates::fox_wstr, vars)
    );
}

TEST(tstrings, interpolate_numeric_map)
{
    const std::unordered_map<string, string> vars = {
        { "0", "quick" },
        { "1", "brown" }
    };

    EXPECT_EQ(
        strings::tqbf, 
        tstrings::interpolate_braces(templates::fox_numeric, vars)
    );
}

TEST(tstrings, interpolate_numeric)
{
    const std::vector<string> vars { "quick", "brown" };
    EXPECT_EQ(
        strings::tqbf, 
        tstrings::interpolate_braces(templates::fox_numeric, vars)
    );

    EXPECT_EQ(
        "The    fox.", 
        tstrings::interpolate_braces(templates::fox_numeric_invalid, vars)
    );
}

TEST(tstrings, interpolate_empty_variable)
{
    const std::unordered_map<string, string> vars = {
        { "color", "" }
    };

    EXPECT_EQ(
        "The quick  fox.",
        tstrings::interpolate_braces(templates::fox, vars)
    );
}

TEST(tstrings, interpolate_with_whitespace)
{
    const std::unordered_map<string, string> vars = {
        { "color", "brown" }
    };

    EXPECT_EQ(
        strings::tqbf, 
        tstrings::interpolate_braces(templates::fox_spaces, vars)
    );
}

TEST(tstrings, interpolate_stream_small)
{
    const std::unordered_map<string, string> vars = {
        { "color", "brown" }
    };

    std::stringstream output;
    // returns an output stream writing to `output`
    auto out = tstrings::interpolate_braces(vars, output);

    // wrap the streambuf in a ostream
    out << templates::fox << std::flush;

    EXPECT_EQ(
        strings::tqbf,
        output.str()
    );
}

TEST(tstrings, interpolate_stream_file)
{
    const std::unordered_map<string, string> vars = {
        { "color", "brown" },
        { "animal", "fox" },
        { "dog", "dalmatian" },
    };

    std::stringstream output;
    {
        std::ifstream template_file(files::small_utf8);
        if (template_file) {
            // read the entire file to the output stream
            tstrings::interpolate_braces(vars, output) 
                << template_file.rdbuf() 
                << std::flush;
        } 
        else {
            FAIL() << "file '" <<  files::small_utf8 << "' not open";
        }
    }

    // test the result
    std::ifstream expected_file(files::small_utf8_expect);
    if (expected_file)
    {
        // simple equality test
        if (!std::equal(std::istreambuf_iterator<char>(output), 
                std::istreambuf_iterator<char>(), 
                std::istreambuf_iterator<char>(expected_file))) {

            output.seekg(0);
            FAIL() << "files not identical after interpolation:\n" << output.rdbuf();
        }
    }
    else {
        FAIL() << "file '" <<  files::small_utf8_expect << "' not open";
    }
}