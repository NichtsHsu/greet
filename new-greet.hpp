/* MIT License
 *
 * Copyright (c) 2023 Nichts Hsu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef GREET_HPP
#define GREET_HPP

#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace greet {
/**
 * @brief Constexpr overloaded version of <ctype.h> functions.
 */
namespace ascii {
    constexpr bool islower(char ch) { return ch >= 'a' && ch <= 'z'; }
    constexpr bool issuper(char ch) { return ch >= 'A' && ch <= 'Z'; }
    constexpr bool isdigit(char ch) { return ch >= '0' && ch <= '9'; }
    constexpr bool isalpha(char ch) { return islower(ch) || issuper(ch); }
    constexpr bool isalnum(char ch) { return isalpha(ch) || isdigit(ch); }
    constexpr bool isxdigit(char ch) {
        return isdigit(ch) || (ch >= 'a' && ch <= 'f') ||
               (ch >= 'A' && ch <= 'F');
    }
    constexpr bool iscntrl(char ch) {
        return (ch >= 0x00 && ch <= 0x1F) || ch == 0x7F;
    }
    constexpr bool ispunct(char ch) {
        return (ch >= '!' && ch <= '/') || (ch >= ':' && ch <= '@') ||
               (ch >= '[' && ch <= '`') || (ch >= '{' && ch <= '~');
    }
    constexpr bool isblank(char ch) { return ch == ' ' || ch == '\t'; }
    constexpr bool isspace(char ch) {
        return isblank(ch) || ch == '\f' || ch == '\n' || ch == '\r' ||
               ch == '\v';
    }
    constexpr bool isgraph(char ch) { return isalnum(ch) || ispunct(ch); }
    constexpr bool isprint(char ch) { return isgraph(ch) || ch == ' '; }
    constexpr bool isident(char ch) { return isalnum(ch) || ch == '_'; }
    constexpr char toupper(char ch) {
        return ch >= 'a' && ch <= 'z' ? ch - 32 : ch;
    }
    constexpr char tolower(char ch) {
        return ch >= 'A' && ch <= 'Z' ? ch + 32 : ch;
    }
    template <char From, char To>
    constexpr char replace(char ch) {
        return ch == From ? To : ch;
    }
}  // namespace ascii

/**
 * @brief Some constexpr function for string operating.
 */
namespace strutils {
    /**
     * @brief C-style string wrapper type that can be used in constant context.
     * @tparam N string length with trailing '\0'.
     */
    template <size_t N>
    struct str_t : public std::array<char, N> {
        constexpr str_t() : std::array<char, N>{'\0'} {}
        constexpr str_t(const char (&in)[N]) : std::array<char, N>{} {
            for (size_t i = 0; i < N; ++i) (*this)[i] = in[i];
        }
        constexpr ~str_t() = default;
        constexpr char *data_end() { return std::array<char, N>::data() + N; }
        constexpr const char *data_end() const {
            return std::array<char, N>::data() + N;
        }

        /**
         * @brief Get substring.
         * @details
         * If `End` is not equals with `N`, an extra `'\0'` will be added.
         * @tparam Start index of substring start
         * @tparam End index of substring end
         */
        template <size_t Start, size_t End = N>
        constexpr auto substr() const
            requires(End <= N)
        {
            if constexpr (End == N) {
                str_t<End - Start> str{};
                for (size_t i = 0; i < End - Start; ++i)
                    str[i] = (*this)[Start + i];
                return str;
            } else {
                str_t<End - Start + 1> str{};
                for (size_t i = 0; i < End - Start; ++i)
                    str[i] = (*this)[Start + i];
                return str;
            }
        };
    };

    /**
     * @brief Concatenate multiple strings into one string at compile time.
     * @tparam sizes sizes of each input string
     * @param strs input strings
     * @return Flattenned string.
     */
    template <size_t... sizes>
    constexpr auto flatten(str_t<sizes>... strs) {
        constexpr size_t COUNT = sizeof...(strs);
        constexpr size_t lengths[COUNT] = {(strs.size() - 1)...};
        constexpr size_t FLAT_LENGTH = (sizes + ...) - COUNT + 1;

        char *datas[COUNT] = {strs.data()...};
        str_t<FLAT_LENGTH> flat{};
        size_t index = 0;
        for (size_t i = 0; i < COUNT; i++) {
            for (size_t j = 0; j < lengths[i]; j++) {
                flat[index] = datas[i][j];
                index++;
            }
        }

        return flat;
    }

    /**
     * @brief Make a string uppercase.
     * @tparam N size of string
     * @param str input string
     * @return New string with all letters in uppercase.
     */
    template <size_t N>
    constexpr str_t<N> uppercase(str_t<N> str) {
        std::transform(str.begin(), str.end(), str.begin(), ascii::toupper);
        return str;
    }

    /**
     * @brief Make a string lowercase.
     * @tparam N size of string
     * @param str input string
     * @return New string with all letters in lowercase.
     */
    template <size_t N>
    constexpr str_t<N> lowercase(str_t<N> str) {
        std::transform(str.begin(), str.end(), str.begin(), ascii::tolower);
        return str;
    }

    /**
     * @brief Replace hyphen(`-`) to underscore(`_`).
     * @tparam N size of string
     * @param str input string
     * @return New string with all hyphen(`-`) was replaced by underscore(`_`).
     */
    template <size_t N>
    constexpr str_t<N> hyphen_to_underscore(str_t<N> str) {
        std::transform(
            str.begin(), str.end(), str.begin(), ascii::replace<'-', '_'>);
        return str;
    }
}  // namespace strutils

/**
 * @brief Compile-time reflection from glaze and magic_get.
 * @see [glaze](https://github.com/stephenberry/glaze)
 * @see [magic_get](https://github.com/apolukhin/magic_get)
 */
namespace reflect {
    /**
     * @brief An object of `T` that can be used in constant context without really construct it.
     * @tparam T any type.
     */
    template <typename T>
    extern const T fake_obj;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

    /**
     * @brief Type that can be implicitly converted to any rvalue reference type.
     */
    struct any_rref_t {
        template <typename T>
        [[maybe_unused]] constexpr operator T &&() const & noexcept;
        template <typename T>
        [[maybe_unused]] constexpr operator T &&() const && noexcept;
    };

    /**
     * @brief Type that can be implicitly converted to any lvalue reference type.
     */
    struct any_lref_t {
        template <typename T>
        [[maybe_unused]] constexpr operator T &() const & noexcept;
        template <typename T>
        [[maybe_unused]] constexpr operator T &() const && noexcept;
    };

    /**
     * @brief Type that can be implicitly converted to any type.
     */
    struct any_t {
        template <typename T>
        [[maybe_unused]] constexpr operator T() const & noexcept;
        template <typename T>
        [[maybe_unused]] constexpr operator T() const && noexcept;
    };

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    /**
     * @brief Get the number of an aggregate type's member.
     * @details
     * Just call `get_number_of_member<T>()` and keep the template pack `Args ...` empty.
     * Basically, it first tests whether `T` can be constructed with one parameter,
     * and if so, then tries to construct it with two parameters,
     * and so on, until `T` cannot be constructed with `N` parameters,
     * then the number of members of `T` is `N -1`.
     * @tparam T must be an aggregate type.
     * @tparam Args just keep empty.
     * @return The number of T's member.
     */
    template <typename T, typename... Args>
    consteval size_t get_number_of_member()
        requires(std::is_aggregate_v<std::remove_cvref_t<T>>)
    {
        using T_ = std::remove_cvref_t<T>;
        if constexpr (requires { T_{{Args{}}..., {any_rref_t{}}}; })
            return get_number_of_member<T_, Args..., any_rref_t>();
        else if constexpr (requires { T_{{Args{}}..., {any_lref_t{}}}; })
            return get_number_of_member<T_, Args..., any_lref_t>();
        else if constexpr (requires { T_{{Args{}}..., {any_t{}}}; })
            return get_number_of_member<T_, Args..., any_t>();
        else
            return sizeof...(Args);
    }

    /**
     * @brief Simplification of interface `get_number_of_member<T>()`.
     */
    template <typename T>
    constexpr size_t number_of_member = get_number_of_member<T>();

    /**
     * @brief Convert a value of type `T` to a tuple containing references to all its members.
     * @details
     * The fundamental factor that can be implemented is structured binding declaration.
     * Then, just write a lot of repetitive code.
     * @tparam T any type.
     * @tparam N number of `T`'s member, automatically calculated if `T` is an aggregate type.
     */
    template <class T, size_t N = number_of_member<T>>
    constexpr decltype(auto) to_tuple(T &&t)
        requires(N <= 128)
    {
        if constexpr (N == 0) {
            return std::tuple{};
        } else if constexpr (N == 1) {
            auto &&[p] = t;
            return std::tie(p);
        } else if constexpr (N == 2) {
            auto &&[p0, p1] = t;
            return std::tie(p0, p1);
        } else if constexpr (N == 3) {
            auto &&[p0, p1, p2] = t;
            return std::tie(p0, p1, p2);
        } else if constexpr (N == 4) {
            auto &&[p0, p1, p2, p3] = t;
            return std::tie(p0, p1, p2, p3);
        } else if constexpr (N == 5) {
            auto &&[p0, p1, p2, p3, p4] = t;
            return std::tie(p0, p1, p2, p3, p4);
        } else if constexpr (N == 6) {
            auto &&[p0, p1, p2, p3, p4, p5] = t;
            return std::tie(p0, p1, p2, p3, p4, p5);
        } else if constexpr (N == 7) {
            auto &&[p0, p1, p2, p3, p4, p5, p6] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6);
        } else if constexpr (N == 8) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6, p7);
        } else if constexpr (N == 9) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7, p8] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6, p7, p8);
        } else if constexpr (N == 10) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
        } else if constexpr (N == 11) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
        } else if constexpr (N == 12) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
            return std::tie(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
        } else if constexpr (N == 13) {
            auto &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
        } else if constexpr (N == 14) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
        } else if constexpr (N == 15) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                p14);
        } else if constexpr (N == 16) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15);
        } else if constexpr (N == 17) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16);
        } else if constexpr (N == 18) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17);
        } else if constexpr (N == 19) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18);
        } else if constexpr (N == 20) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19);
        } else if constexpr (N == 21) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20);
        } else if constexpr (N == 22) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21);
        } else if constexpr (N == 23) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22);
        } else if constexpr (N == 24) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23);
        } else if constexpr (N == 25) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24);
        } else if constexpr (N == 26) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25);
        } else if constexpr (N == 27) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26);
        } else if constexpr (N == 28) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                p27);
        } else if constexpr (N == 29) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28);
        } else if constexpr (N == 30) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29);
        } else if constexpr (N == 31) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30);
        } else if constexpr (N == 32) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31);
        } else if constexpr (N == 33) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32);
        } else if constexpr (N == 34) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33);
        } else if constexpr (N == 35) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34);
        } else if constexpr (N == 36) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35);
        } else if constexpr (N == 37) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36);
        } else if constexpr (N == 38) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37);
        } else if constexpr (N == 39) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38);
        } else if constexpr (N == 40) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39);
        } else if constexpr (N == 41) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39,
                p40);
        } else if constexpr (N == 42) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41);
        } else if constexpr (N == 43) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42);
        } else if constexpr (N == 44) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43);
        } else if constexpr (N == 45) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44);
        } else if constexpr (N == 46) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45);
        } else if constexpr (N == 47) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46);
        } else if constexpr (N == 48) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47);
        } else if constexpr (N == 49) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48);
        } else if constexpr (N == 50) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49);
        } else if constexpr (N == 51) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50);
        } else if constexpr (N == 52) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51);
        } else if constexpr (N == 53) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52);
        } else if constexpr (N == 54) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52,
                p53);
        } else if constexpr (N == 55) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54);
        } else if constexpr (N == 56) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55);
        } else if constexpr (N == 57) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56);
        } else if constexpr (N == 58) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57);
        } else if constexpr (N == 59) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58);
        } else if constexpr (N == 60) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59);
        } else if constexpr (N == 61) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60);
        } else if constexpr (N == 62) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61);
        } else if constexpr (N == 63) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62);
        } else if constexpr (N == 64) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63);
        } else if constexpr (N == 65) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64);
        } else if constexpr (N == 66) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65);
        } else if constexpr (N == 67) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65,
                p66);
        } else if constexpr (N == 68) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67);
        } else if constexpr (N == 69) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68);
        } else if constexpr (N == 70) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69);
        } else if constexpr (N == 71) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70);
        } else if constexpr (N == 72) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71);
        } else if constexpr (N == 73) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72);
        } else if constexpr (N == 74) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73);
        } else if constexpr (N == 75) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74);
        } else if constexpr (N == 76) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75);
        } else if constexpr (N == 77) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76);
        } else if constexpr (N == 78) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77);
        } else if constexpr (N == 79) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78);
        } else if constexpr (N == 80) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78,
                p79);
        } else if constexpr (N == 81) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80);
        } else if constexpr (N == 82) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81);
        } else if constexpr (N == 83) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82);
        } else if constexpr (N == 84) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83);
        } else if constexpr (N == 85) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84);
        } else if constexpr (N == 86) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85);
        } else if constexpr (N == 87) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86);
        } else if constexpr (N == 88) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87);
        } else if constexpr (N == 89) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88);
        } else if constexpr (N == 90) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89);
        } else if constexpr (N == 91) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90);
        } else if constexpr (N == 92) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91);
        } else if constexpr (N == 93) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91,
                p92);
        } else if constexpr (N == 94) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93);
        } else if constexpr (N == 95) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94);
        } else if constexpr (N == 96) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95);
        } else if constexpr (N == 97) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96);
        } else if constexpr (N == 98) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97);
        } else if constexpr (N == 99) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98);
        } else if constexpr (N == 100) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99);
        } else if constexpr (N == 101) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100);
        } else if constexpr (N == 102) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101);
        } else if constexpr (N == 103) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102);
        } else if constexpr (N == 104) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103);
        } else if constexpr (N == 105) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103,
                p104);
        } else if constexpr (N == 106) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105);
        } else if constexpr (N == 107) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106);
        } else if constexpr (N == 108) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107);
        } else if constexpr (N == 109) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108);
        } else if constexpr (N == 110) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109);
        } else if constexpr (N == 111) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110);
        } else if constexpr (N == 112) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111);
        } else if constexpr (N == 113) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112);
        } else if constexpr (N == 114) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113);
        } else if constexpr (N == 115) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114);
        } else if constexpr (N == 116) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115);
        } else if constexpr (N == 117) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116);
        } else if constexpr (N == 118) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117);
        } else if constexpr (N == 119) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118);
        } else if constexpr (N == 120) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119);
        } else if constexpr (N == 121) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120);
        } else if constexpr (N == 122) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121);
        } else if constexpr (N == 123) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122);
        } else if constexpr (N == 124) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122, p123] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122, p123);
        } else if constexpr (N == 125) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122, p123, p124] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122, p123, p124);
        } else if constexpr (N == 126) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122, p123, p124, p125] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122, p123, p124,
                p125);
        } else if constexpr (N == 127) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122, p123, p124, p125, p126] = t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122, p123, p124,
                p125, p126);
        } else if constexpr (N == 128) {
            auto
                &&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                   p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25,
                   p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37,
                   p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
                   p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61,
                   p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73,
                   p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85,
                   p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97,
                   p98, p99, p100, p101, p102, p103, p104, p105, p106, p107,
                   p108, p109, p110, p111, p112, p113, p114, p115, p116, p117,
                   p118, p119, p120, p121, p122, p123, p124, p125, p126, p127] =
                    t;
            return std::tie(
                p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27,
                p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40,
                p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53,
                p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66,
                p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79,
                p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92,
                p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104,
                p105, p106, p107, p108, p109, p110, p111, p112, p113, p114,
                p115, p116, p117, p118, p119, p120, p121, p122, p123, p124,
                p125, p126, p127);
        }
    }

    /**
     * @brief Wrapper of pointer type.
     * @details
     * We cannot directly pass the pointer of a member of a constexpr struct as the
     * template auto parameter when we use Clang as the compiler.
     * Therefore, we wrapper the pointer to a constexpr cptr_t value and pass it
     * as the template auto paramter.
     * @tparam T any type.
     */
    template <typename T>
    struct cptr_t {
        const T *const ptr;
    };

    /**
     * @brief Get the pointer of the N-th member as a constexpr value.
     * @tparam T any type.
     * @tparam N index of the member.
     * @return `cptr_t` wrapped pointer.
     */
    template <typename T, size_t N>
    consteval auto cptr_of_member() {
        decltype(auto) member = std::get<N>(to_tuple(fake_obj<T>));
        return cptr_t<std::decay_t<decltype(member)>>{&member};
    }

    /**
     * @brief Core function to get name via compiler built-in macro.
     * @tparam Ptr pointer to the the variable or field you want to reflect.
     * @return A compile-time string containing the name of the variable or field.
     */
    template <auto Ptr>
    consteval const char *pretty_name() {
#if defined(__clang__) || defined(__GNUC__)
        return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        return __FUNCSIG__;
#endif
    }

    /**
     * @brief Get the base name of a full qualified name.
     * @details For examples, `basename_of("xxx::yyy::zzz.nnn")` returns `"nnn"`.
     * @param name the name that may be quailfied.
     * @return The base name.
     */
    constexpr std::string_view basename_of(std::string_view name) {
        auto end = std::find_if(name.crbegin(), name.crend(), ascii::isident);
        auto start = std::find_if_not(end, name.crend(), ascii::isident);
        return std::string_view(start.base(), end.base());
    }

    /**
     * @brief Extract the real name from the output of `pretty_name()`.
     * @tparam Ptr pointer to the the variable or field you want to reflect.
     * @return A compile-time string of name of the variable or field.
     */
    template <auto Ptr>
    consteval std::string_view name_of() {
        std::string_view name = pretty_name<Ptr>();
#if defined(__clang__) || defined(__GNUC__)
        std::string_view prefix = "[with auto Ptr = (& ";
        std::string_view suffix = ")]";
#elif defined(_MSC_VER)
        std::string_view prefix = "pretty_name<&";
        std::string_view suffix = ">";
#endif
        auto start = name.find(prefix) + prefix.length();
        auto len = name.rfind(suffix) - start;
        std::string_view path = name.substr(start, len);

        return basename_of(path);
    }

    /**
     * @brief Get the name of N-th member of type `T`.
     * @tparam T any type.
     * @tparam N index of member.
     * @return Name of N-th member.
     */
    template <typename T, size_t N>
    consteval std::string_view name_of() {
        return name_of<cptr_of_member<T, N>()>();
    }
}  // namespace reflect

namespace details {
    template <typename Tp>
    concept from_chars =
        requires(Tp t, const char *s) { std::from_chars(s, s, t); };

    template <typename Tp>
    concept integer = std::integral<Tp> && from_chars<Tp>;

    template <typename Tp>
    concept float_pointer = std::floating_point<Tp> && from_chars<Tp>;

    enum {
        ARGUMENT,
        ENDARG,
        SHORT,
        LONG,
    };

    inline size_t argtype(std::string_view arg) {
        if (arg == std::string_view("--")) return ENDARG;
        if (arg.starts_with("--")) return LONG;
        if (arg.starts_with("-")) return SHORT;
        return ARGUMENT;
    };

    inline std::string filename(std::string const &path) {
        return path.substr(path.find_last_of("/\\") + 1);
    }

    inline void remove_one_arg(int &argc, char **(&argv)) {
        --argc;
        ++argv;
    }

    template <typename Srch, typename IdxSeq, size_t CurIdx, typename... Ts>
    struct type_positions_s {};

    template <typename Srch, size_t CurIdx, size_t... Idx>
    struct type_positions_s<Srch, std::index_sequence<Idx...>, CurIdx> {
        using type = std::index_sequence<Idx...>;
    };

    template <
        typename Srch, typename Cur, typename... Ts, size_t CurIdx,
        size_t... Idx>
    struct type_positions_s<
        Srch, std::index_sequence<Idx...>, CurIdx, Cur, Ts...> {
        using type = std::conditional_t<
            std::is_same_v<
                std::remove_cvref_t<SrchT>, std::remove_cvref_t<CurT>>,
            typename type_positions_s<
                SrchT, std::index_sequence<Idx..., CurIdx>, CurIdx + 1,
                Ts...>::type,
            typename type_positions_s<
                SrchT, std::index_sequence<Idx...>, CurIdx + 1, Ts...>::type>;
    };

    /**
     * @brief Get the positions of a certain type in a series of types.
     * @details
     * For examples, when `Srch` is `T`, and `Ts...` is `T, U, V, T, N`, then
     * `type_potision<Srch, Ts...>` is `std::index_sequence<0, 3>`.
     * @tparam Srch type to search
     * @tparam Ts a series types may contains the `Srch` type
     */
    template <typename Srch, typename... Ts>
    using type_positions =
        typename type_positions_s<Srch, std::index_sequence<>, 0, Ts...>::type;

    template <typename Srch, typename Tuple>
    struct type_positions_in_tuple_s {};

    template <typename Srch, typename... Ts>
    struct type_positions_in_tuple_s<Srch, std::tuple<Ts...>> {
        using type = type_positions<Srch, Ts...>;
    };

    /**
     * @brief Similar to `type_positions`, but search type in a tuple.
     * @tparam Srch type to search
     * @tparam Tuple a tuple that may contains the `Srch` type
     */
    template <typename Srch, typename Tuple>
    using type_positions_in_tuple =
        typename type_positions_in_tuple_s<Srch, Tuple>::type;

    /**
     * @brief Get the number of times a certain type appears in a series of types.
     * For examples, when `Srch` is `T`, and `Ts...` is `T, U, V, T, N`, then
     * `type_count<Srch, Ts...>` is `2`.
     * @tparam Srch type to search
     * @tparam Ts a series types may contains the `Srch` type
     */
    template <typename Srch, typename... Ts>
    constexpr size_t type_count = type_positions<Srch, Ts...>::size();

    /**
     * @brief Similar to `type_count`, but search type in a tuple.
     * @tparam Srch type to search
     * @tparam Tuple a tuple that may contains the `Srch` type
     */
    template <typename Srch, typename Tuple>
    constexpr size_t type_count_in_tuple =
        type_positions_in_tuple<Srch, Tuple>::size();
}  // namespace details

/**
 * @brief A wrapper type to collect ignored arguments.
 * @details
 * Double-hyphen(`--`) without long flag means end of options, all subsequent
 * arguments are no longer parsed. If you have a member of type `greet::ignored`,
 * greet will collect the ignored arguments in it.
 */
class ignored : public std::vector<std::string> {
  public:
    using std::vector<std::string>::vector;
};

/**
 * @brief A counter type that can count how many times the option was used.
 * @details
 * For examples, a counter type option named `verbose`, when arguments are
 * `-vv --verbose`, then the value of this option is `3`.
 */
class counter {
  public:
    counter();
    counter(const counter &other) = default;
    counter(counter &&other) : _counter(other._counter) { other._counter = 0; }
    counter &operator=(const counter &other) = default;
    counter &operator=(counter &&other) {
        _counter = other._counter;
        other._counter = 0;
        return *this;
    }

    counter &operator++() {
        ++_counter;
        return *this;
    }
    counter operator++(int) {
        counter prev = *this;
        ++_counter;
        return prev;
    }
    operator size_t() { return _counter; }

  private:
    size_t _counter;
};

template <typename OptT>
struct string_converter;

template <typename OptT>
concept string_convertable = requires(OptT t, const char *str) {
    {
        string_converter<OptT>::from_str(str)
    } -> std::same_as<std::optional<OptT>>;
    { string_converter<OptT>::to_str(t) } -> std::same_as<std::string>;
};

template <details::integer OptT>
struct string_converter<OptT> {
    static auto from_str(const char *str) -> std::optional<OptT> {
        OptT v{};
        const char *end = _detail::strend(str);
        std::from_chars_result result;
        if (str[0] == '0')
            if (str[1] == 'x')
                result = std::from_chars(str + 2, end, v, 16);
            else
                result = std::from_chars(str + 1, end, v, 8);
        else
            result = std::from_chars(str, end, v);
        auto [ptr, ec] = result;
        if (ec == std::errc{})
            if (ptr == end)
                return v;
            else
                return std::nullopt;
        else
            return sstd::nullopt;
    }

    static std::string to_str(const OptT &value) {
        return std::to_string(value);
    }
};

template <details::float_pointer OptT>
struct string_converter<OptT> {
    static auto from_str(const char *str) -> std::optional<OptT> {
        OptT v{};
        const char *end = _detail::strend(str);
        auto [ptr, ec] = std::from_chars(str, end, v);
        if (ec == std::errc{})
            if (ptr == end)
                return v;
            else
                return std::nullopt;
        else
            return std::nullopt;
    }

    static std::string to_str(const OptT &value) {
        return std::to_string(value);
    }
};

template <>
struct string_converter<char> {
    static auto from_str(const char *str) -> std::optional<char> {
        if (std::char_traits<char>::length(str) != 1) return std::nullopt;
        return *str;
    }

    static std::string to_str(const char &value) {
        return std::string(1, value);
    }
};

template <>
struct string_converter<const char *> {
    static auto from_str(const char *str) -> std::optional<const char *> {
        return str;
    }

    static std::string to_str(const char *const &value) { return value; }
};

template <>
struct string_converter<std::string_view> {
    static auto from_str(const char *str) -> std::optional<std::string_view> {
        return std::string_view(str);
    }

    static std::string to_str(std::string_view &value) {
        return std::string(value);
    }
};

template <>
struct string_converter<std::string> {
    static auto from_str(const char *str) -> std::optional<std::string> {
        return std::string(str);
    }

    static std::string to_str(const std::string &value) { return value; }
};

/**
 * @brief Metadata of options.
 * @note
 * It can be automatically generated by greet.
 * If you want to customize metadata, please specialize `greet::gen_meta<T>(T &)`.
 */
class meta {
  public:
    meta() = default;
    void set_program_name(std::string_view name);
    std::string_view program_name() const;

  private:
    std::string_view _program_name;
};

/**
 * @brief Error code when parsing arguments failed.
 */
enum class parse_error {
    /**
     * @brief An option from your custom metadata that has neither short nor long flag.
     */
    INVALID_OPTION,

    /**
     * @brief Flag of an option from your custom metadata contains unprintable characters.
     */
    NOT_PRINTABLE,

    /**
     * @brief The short flag of an option from your custom metadata is hyphen(`-`).
     */
    SHORT_FLAG_IS_HYPHEN,

    /**
     * @brief The short flag of an option from your custom metadata is already used.
     */
    SHORT_FLAG_ALREADY_USED,

    /**
     * @brief The long flag of an option from your custom metadata is already used.
     */
    LONG_FLAG_ALREADY_USED,

    /**
     * @brief An unexpected argument.
     * @details
     * For examples, it is a flag that none of options use it,
     * or it is a value but the previous argument is not an option that requires value.
     */
    UNEXPECTED_ARGUMENT,

    /**
     * @brief Give an option a value, when this option doesn't require value.
     * @note
     * If `-a` and `-aaa` don't require value, only `--aaa=xyz` will trigger this error.
     * `-a xyz` and `--aaa xyz` will trigger `UNEXPECTED_ARGUMENT`;
     * `-axyz` will be treated as `-a -xyz` then further parse `-xyz`;
     * `-a=xyz` will be treated as `-a -=xyz`, remember that `=` is a legal short flag.
     */
    UNEXPECTED_VALUE,

    /**
     * @brief Option requires a value but none was supplied.
     */
    MISSING_VALUE,

    /**
     * @brief Failed to convert argument to an expected value of option.
     */
    INVALID_VALUE,

    /**
     * @brief An option can only be used once but is used multiple times.
     */
    MUTIPLE_OPTION,

    /**
     * @brief An option that must be used but is not used.
     */
    MSSING_OPTION,
};

struct default_handler {
    static int on_print_help(const meta &);
    static int on_print_version(const meta &);
    static int on_error(const meta &, parse_error, std::string_view);
};

/**
 * @brief Configuration of greet's behaviors when parsing options.
 * @details
 * The default behaviors are:
 * 1.  An option named `abcd` can accept short flag `-a` and long flag `--abcd`.
 * 2.  If multiple options start with the same letter, only the first declared option
 *     has the short flag.
 * 3.  An option named `abc_def` will generate a long flag `--abc-def`, which means
 *     that the symbol `_` will be replaced by the symbol `-`.
 * 4.  Option names are case-insensitive, an option named `AbcDefG` will generate
 *     a short flag `-a` and a long flag `--abcdefg`.
 * 4.  `-h`, `--help`, `-V` and `--version` are reserved for print help and version.
 * 5.  For options that need a value, `-a xxx`, `-axxx`, `-a=xxx`, `--aaa xxx` and
 *     `--aaa=xxx` are all acceptable.
 * 6.  The short flag must be a printable character (from `!` to `~`) and cannot be `-`.
 * 7.  For options that don't need a value, `-e -f -g`, `--eee --fff --ggg`,
 *     `--eee -f --ggg` and `-efg` are all acceptable.
 * 8.  When you mix options that need a value with options that don’t, such as `-faxxxg`,
 *     it will be parsed as `-f -a xxxg` but not `-f -a xxx -g`.
 * 9.  When the value of an option is start with a hyphen(`-`), `-a-b`, `-a=-b` and
 *     `--aaa=-b` are all acceptable. However, `-a -b` and `--aaa -b` are not acceptable
 *     and will be parsed to two options.
 * 10. Double-hyphen(`--`) without long flag means end of options, all subsequent
 *     arguments are no longer parsed. If you want to collect them, use `greet::ignored`.
 */
struct configuration {
    /**
     * @brief Allow values start with a hyphen(`-`).
     * @details
     * By default, if you want to pass a value start with hyphen(`-`), `-a-b`, `-a=-b` and
     * `--aaa=-b` are all acceptable but `-a -b` and `--aaa -b` are not acceptable.
     * if enabled, `-a -b` and `--aaa -b` are also acceptable.
     * Moreover, even `-a --` and `--aaa --` are acceptable.
     */
    bool allow_hyphen;

    /**
     * @brief Do not use short flag, only accept long flags.
     * @note This item is not valid for your custom metadata.
     */
    bool no_short_flag;

    /**
     * @brief Case sensitive when generating flags for option names.
     * @details
     * By default, if there are two options named `aBc` and `AbC`, greet will only generate
     * all-lowercase flags `-a` and `--abc` for `aBc`, and if `uppercase_second_short_flag`
     * is not enabled, trigger `INVALID_OPTION` for `AbC`. If `case_sensitive` enabled, the
     * option `aBc` will get `-a` and `--aBc`, and the option `AbC` will get `-A` and `--AbC`.
     * @note This item is not valid for your custom metadata.
     */
    bool case_sensitive;

    /**
     * @brief Generate uppercase short flags for options.
     * @note This item is not valid for your custom metadata.
     * @note This item is not valid when `case_sensitive` is enabled.
     */
    bool uppercase_short_flag;

    /**
     * @brief When options starting with the same letter, use uppercase for the second one.
     * @details
     * For examples, there are three options `afirst`, `asecond` and `athird`, if enabled,
     * the option `afirst` will get short flag `-a` and the option `asecond` will get `-A`.
     * @note If `uppercase_short_flag` is also enabled, the second option will get lowercase.
     * @note This item is not valid for your custom metadata.
     * @note This item is not valid when `case_sensitive` is enabled.
     */
    bool uppercase_second_short_flag;

    /**
     * @brief Use underscore(`_`) in long flags.
     * @details
     * By Default, an option named `abc_def` will generate a long flag `--abc-def`, which
     * means that the underscore(`_`) will be replaced by the hyphen(`-`). If enabled,
     * the underscore(`_`) will be kept, the long flag will become `--abc_def`.
     * @note This item is not valid for your custom metadata.
     */
    bool use_underscore;

    /**
     * @brief Do not generate option for "print help" behavior.
     */
    bool no_help;

    /**
     * @brief Do not generate option for "print version" behavior.
     */
    bool no_version;

    /**
     * @brief Short flag for the "print help" behavior, default to 'h'.
     */
    char help_short_flag = 'h';

    /**
     * @brief Long flag for the "print help" behavior, default to 'help`.
     */
    std::string_view help_long_flag = "help";

    /**
     * @brief Short flag for the "print version" behavior, default to 'V'.
     */
    char version_short_flag = 'V';

    /**
     * @brief Long flag for the "print version" behavior, default to `version`.
     */
    std::string_view version_long_flag = "version";

    /**
     * @brief Handling function for "print help" behavior, default to 
     * `greet::default_handler::on_print_help`.
     */
    std::function<int(const meta &)> on_print_help =
        default_handler::on_print_help;

    /**
     * @brief Handling function for "print version" behavior, default to 
     * `greet::default_handler::on_print_version`.
     */
    std::function<int(const meta &)> on_print_version =
        default_handler::on_print_version;

    /**
     * @brief Handling function for "error handling" behavior, default to 
     * `greet::default_handler::on_error`.
     */
    std::function<int(const meta &, parse_error, std::string_view)> on_error =
        default_handler::on_error;
};

}  // namespace greet

#endif
