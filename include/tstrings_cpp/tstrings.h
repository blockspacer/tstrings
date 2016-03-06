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

#include <iostream>

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

        RegexIt rit(std::begin(tstr), std::end(tstr), reg_exp), rend;
        std::basic_string<Ch, Tr, Alloc> result;

        /* iterate over all matches */
        int64_t idx = 0;
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

        Str result;

        return interpolate_by_regex(tstring,
            detail::get_regex<Ch>(),
            fn);
    }

    /* ---------------------------------------------------- */

    namespace detail
    {
        template<typename Ch>
        struct brace_tokens {
            static const Ch escape_sequence;
            static const Ch delimiter;
            static const Ch head;
            static const Ch tail;
        };

        template<>
        struct brace_tokens<char> {
            static const char escape_sequence = '\\';
            static const char delimiter = '$';
            static const char head = '{';
            static const char tail = '}';
        };

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        class templ_streambuf final
            : public std::basic_streambuf<Ch>
        {
            using TOK = brace_tokens<Ch>;
            using base = std::basic_streambuf<Ch>;
            using char_type = typename base::char_type;
            using int_type = typename base::int_type;

            public:
                explicit templ_streambuf(std::basic_ostream<Ch> &sink, Fn resolve);

            private:
                bool scan();
                bool parse_and_resolve(const Ch* ch);

                int_type overflow(int_type ch) override;
                int sync() override;

                // copy ctor and assignment not implemented;
                // copying not allowed
                templ_streambuf(const templ_streambuf &);
                templ_streambuf &operator= (const templ_streambuf &);

                std::ostream &sink_;
                Fn resolve_;

                std::array<Ch, LEN+1> buffer_;
                std::string lookahead_;
                Ch prev_;

                // indicates whether the parser is
                // in an interpolated region
                bool region_;

        };

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        templ_streambuf<Ch, Fn, LEN>::templ_streambuf(std::basic_ostream<Ch>& sink, Fn fn)
            : sink_(sink)
            , resolve_(fn)
            , lookahead_()
            , prev_(0)
            , region_(false)
        {
            sink_.clear();
            char *base = &buffer_.front();
            base::setp(base, base + buffer_.size() - 1); // -1 to make overflow() easier
        }

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        bool templ_streambuf<Ch, Fn, LEN>::parse_and_resolve(const Ch* ch)
        {
            auto& buf = lookahead_;
            buf.append(ch, 1);

            auto n = buf.length();

            // sanity
            if (n >= 2) {
                if (buf[0] != TOK::delimiter ||
                    buf[1] != TOK::head)
                {
                    // lookahead is invalid; flush to sink
                    sink_.write(buf.data(), n);
                    buf.clear();

                    return true;
                }
            }

            // complete template must be
            // at least 3 characters
            if (n >= 3) {
                if (buf[n - 1] == TOK::tail &&
                    buf[n - 2] != TOK::escape_sequence)
                {
                    auto str = interpolate_by_regex(buf,
                        detail::get_regex<Ch>(),
                        resolve_);

                    if (str.length() > 0) {
                        sink_.write(str.data(), str.length());
                    }

                    buf.clear();
                    return true;
                }
            }
            return false;
        }

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        bool templ_streambuf<Ch, Fn, LEN>::scan()
        {
            Ch* flush_start = nullptr;
            std::size_t n = 0;

            for (Ch* p = this->pbase(), *e = this->pptr(); p != e; ++p) {
                if (n == 0) { flush_start = p; }

                if (!region_) {
                    if (prev_ != TOK::escape_sequence && *p == TOK::delimiter) {
                        if (n > 0) {
                            sink_.write(flush_start, n);
                            n = 0;
                        }
                        region_ = true;
                    }
                }

                if (region_) {
                    if (parse_and_resolve(p)) {
                        region_ = false;
                        n = 0;
                    }
                }
                else { ++n; }
                prev_ = *p;
            }

            if (n > 0) sink_.write(flush_start, n);

            {
                std::ptrdiff_t n = base::pptr() - base::pbase();
                base::pbump(-n);
            }

            return sink_.good();
        }

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        int templ_streambuf<Ch, Fn, LEN>::sync() {
            return scan() ? 0 : -1;
        }

        template<
            typename Ch,
            typename Fn,
            std::size_t LEN
            >
        typename templ_streambuf<Ch, Fn, LEN>::int_type
        templ_streambuf<Ch, Fn, LEN>::overflow(templ_streambuf<Ch, Fn, LEN>::int_type ch)
        {
            if (sink_ && ch != base::traits_type::eof()) {
                assert(std::less_equal<char *>()(base::pptr(), base::epptr()));

                *base::pptr() = ch;
                base::pbump(1);

                if (scan())
                    return ch;
            }
            return base::traits_type::eof();
        }
    }

    /* ---------------------------------------------------- */

    class otstream final
        : public std::ostream
    {
        std::unique_ptr<std::streambuf> buf_;

        public:
            otstream(std::unique_ptr<std::streambuf>&& ptr)
                : std::ostream(ptr.get())
                , buf_{ std::move(ptr) } {
            }

            otstream(otstream&& other)
                : buf_{ std::move(other.buf_) } {
            }

            virtual ~otstream() {}
    };

    /* ---------------------------------------------------- */

    inline otstream
    interpolate_braces(
        const std::unordered_map<std::string, std::string>& vars,
        std::ostream& sink
        )
    {
        auto fn = [&vars](const std::string& var_name, std::string& buff) {
            auto val = vars.find(var_name);
            if (val != std::end(vars)) {
                buff.append(val->second);
            }
        };

        return otstream(std::unique_ptr<std::streambuf>(
            new detail::templ_streambuf<char, decltype(fn), 128>(sink, fn)
        ));
    }
}