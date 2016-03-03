#pragma once 

#include <inttypes.h>
#include <codecvt>
#include <iostream>
#include <string>
#include <regex>
#include <unordered_map>

namespace tstrings
{
    namespace detail 
    {
        /* matches sequences like: 
            - ${VAR}         // single variable ('VAR')
            - ${ VAR }       // variable surrounded by whitespace

           but not
            - ${VA$}         // no special chars 
            - ${VAR1 VAR2}   // no intermediate whitespace
            - ${}            // no undefined variables
        */
        const std::string expr = R"(\$\{\s*([^\u0020-\u002F\u003A-\u0040\u005B-\u0060\u007B-\u007F]+)\s*\})";

        template<typename Ch>
        inline std::basic_regex<Ch> get_regex() 
        {
            std::wstring_convert<std::codecvt_utf8<Ch>, Ch> convert;
            std::basic_string<Ch> utf = convert.from_bytes(expr);

            return std::basic_regex<Ch>(utf);
        }

        template<>
        inline std::basic_regex<char> get_regex<char>() {
            return std::basic_regex<char>(expr);
        }
    }

    template<
        typename Ch,
        typename Fn,
        typename Tr = std::char_traits<Ch>, 
        typename Alloc = std::allocator<Ch>
        >
    std::basic_string<Ch, Tr, Alloc> 
    inline interpolate_by_regex(
        const std::basic_string<Ch, Tr, Alloc>& tstr,
        const std::basic_regex<Ch>& reg_exp,
        Fn& fun
        )
    {
        using It = typename std::basic_string<Ch, Tr, Alloc>::const_iterator;
        using RegexIt = std::regex_iterator<It, Ch>;

        RegexIt rit(std::begin(tstr), std::end(tstr), reg_exp);
        RegexIt rend;

        std::basic_string<Ch, Tr, Alloc> result;

        int64_t idx = 0;

        /* iterate over all matches */
        while (rit != rend) 
        {
            auto& m = *rit;
            auto start_idx = m.position(0);
            auto len = m.length(0);

            result.append(tstr, idx, start_idx - idx);

            /* interpolate */
            const auto& var_name = m[1];
            if (var_name.length() > 0) {
                fun(var_name, result); 
            }
            
            idx = start_idx + len;
            ++rit;
        }

        result.append(tstr, idx, tstr.size() - idx);
        return result;
    }

    /* ---------------------------------------------------- */

    template <
        typename Str = std::string
        >
    inline Str 
    interpolate_braces(
        const Str& tstring,
        const std::unordered_map<Str, Str>& vars
        )
    {
        using Ch = typename Str::value_type;

        auto fn = [&vars](const Str& var_name, Str& buff) {
            auto val = vars.find(var_name);
            if (val != std::end(vars)) {
                buff.append(val->second);
            }
        };

        return interpolate_by_regex(tstring, 
            detail::get_regex<Ch>(), 
            fn);
    }

    /* ---------------------------------------------------- */

    template <
        typename Str = std::string
        >
    inline Str 
    interpolate_braces(
        const Str& tstring,
        const std::vector<Str>& vars
        )
    {
        using Ch = typename Str::value_type;

        auto fn = [&vars](const Str& var_name, Str& buff) {
            unsigned idx = 0u;
            try {
                idx = stoul(var_name);
            } 
            catch (std::invalid_argument) { return; } 
            catch (std::out_of_range) { return; }

            if (idx < vars.size()) {
                buff.append(vars[idx]);
            }
        };

        return interpolate_by_regex(tstring, 
            detail::get_regex<Ch>(), 
            fn);
    }

    /* ---------------------------------------------------- */
}