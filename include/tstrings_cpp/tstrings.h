#pragma once

#include <inttypes.h>
#include <assert.h>

#include <codecvt>
#include <streambuf>
#include <iosfwd>
#include <string>
#include <regex>
#include <unordered_map>
#include <memory>
#include <array>

namespace tstrings
{
    namespace detail
    {
        /* matches sequences like:
            - ${VAR}         // single variable ('VAR')
            - ${ VAR }       // variable surrounded by whitespace

           but not:
            - ${VA$}         // no special chars
            - ${VAR1 VAR2}   // no intermediate whitespace
            - ${}            // no undefined variables
        */
        const std::string expr = R"(\$\{\s*([^\u0020-\u002F\u003A-\u0040\u005B-\u0060\u007B-\u007F]+)\s*\})";
    }

    template<
        typename Ch,
        typename Fn,
        typename Tr = std::char_traits<Ch>,
        typename Alloc = std::allocator<Ch>
        >
    inline void interpolate_by_regex(
        const std::basic_string<Ch, Tr, Alloc>& tstr,
        const std::basic_regex<Ch>& reg_exp,
        std::basic_ostream<Ch>& sink,
        Fn& fun
        );

    /* ---------------------------------------------------- */

    template <
        typename Str = std::string
        >
    inline Str
    interpolate_braces(
        const Str& tstring,
        const std::unordered_map<Str, Str>& vars
        );

    /* ---------------------------------------------------- */

    template <
        typename Str = std::string
        >
    inline Str
    interpolate_braces(
        const Str& tstring,
        const std::vector<Str>& vars
        );

    /* ---------------------------------------------------- */

    template<
        typename Ch
        >
    class otstream final
        : public std::ostream
    {
        std::unique_ptr<std::basic_streambuf<Ch>> buf_;

    public:
        otstream(std::unique_ptr<std::basic_streambuf<Ch>>&& ptr)
            : std::ostream(ptr.get())
            , buf_{ std::move(ptr) } {
        }

        otstream(otstream&& other)
            : buf_{ std::move(other.buf_) } {
        }
    };

    /* ---------------------------------------------------- */

    template <
        typename Str = std::string
        >
    inline otstream<typename Str::value_type>
    interpolate_braces(
        const std::unordered_map<Str, Str>& vars,
        std::basic_ostream<typename Str::value_type>& sink
        );
}

// definitions
#include "tstrings-inl.h"