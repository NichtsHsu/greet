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
#include <charconv>
#include <concepts>
#include <cstring>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace greet {
namespace _detail {
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

    inline size_t argtype(const char *arg) {
        std::string_view str(arg);
        if (str == std::string_view("--")) return ENDARG;
        if (str.starts_with("--")) return LONG;
        if (str.starts_with("-")) return SHORT;
        return ARGUMENT;
    };

    inline const char *strend(const char *str) {
        while (*str != '\0') ++str;
        return str;
    }

    inline std::string uppercase(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    inline std::string filename(std::string const &path) {
        return path.substr(path.find_last_of("/\\") + 1);
    }

    inline void remove_one_arg(int &argc, char **(&argv)) {
        --argc;
        ++argv;
    }

    template <typename SrchT, typename IdxSeqT, size_t CurIdx, typename... Ts>
    struct type_positions {};

    template <typename SrchT, size_t CurIdx, size_t... Idx>
    struct type_positions<SrchT, std::index_sequence<Idx...>, CurIdx> {
        using type = std::index_sequence<Idx...>;
    };

    template <
        typename SrchT, typename CurT, typename... Ts, size_t CurIdx,
        size_t... Idx>
    struct type_positions<
        SrchT, std::index_sequence<Idx...>, CurIdx, CurT, Ts...> {
        using type = std::conditional_t<
            std::is_same_v<std::decay_t<SrchT>, std::decay_t<CurT>>,
            typename type_positions<
                SrchT, std::index_sequence<Idx..., CurIdx>, CurIdx + 1,
                Ts...>::type,
            typename type_positions<
                SrchT, std::index_sequence<Idx...>, CurIdx + 1, Ts...>::type>;
    };

    template <typename SrchT, typename... Ts>
    using type_positions_t =
        typename type_positions<SrchT, std::index_sequence<>, 0, Ts...>::type;
}  // namespace _detail

class meta;
struct information {
    virtual std::string version() = 0;
    virtual std::string description() = 0;
    virtual meta genmeta() = 0;
};

class counter {
  public:
    counter();
    counter(const counter &other) = default;
    counter(counter &&other);
    counter &operator=(const counter &other) = default;
    counter &operator=(counter &&other);

    counter &operator++();
    counter operator++(int);
    operator size_t();

  private:
    size_t _counter;
};

class ignored : public std::vector<std::string> {
  public:
    ignored() = default;
    ignored(const ignored &) = default;
    ignored(ignored &&) = default;
    ignored &operator=(const ignored &) = default;
    ignored &operator=(ignored &&) = default;
};

template <typename OptT>
struct string_converter;

template <typename OptT>
concept string_convertable = requires(OptT t, const char *str) {
    {
        string_converter<OptT>::from_str(str)
    } -> std::same_as<std::expected<OptT, std::errc>>;
    { string_converter<OptT>::to_str(t) } -> std::same_as<std::string>;
};

template <_detail::integer OptT>
struct string_converter<OptT> {
    static auto from_str(const char *str) -> std::expected<OptT, std::errc> {
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
                return std::unexpected(std::errc::invalid_argument);
        else
            return std::unexpected(ec);
    }

    static std::string to_str(const OptT &value) {
        return std::to_string(value);
    }
};

template <_detail::float_pointer OptT>
struct string_converter<OptT> {
    static auto from_str(const char *str) -> std::expected<OptT, std::errc> {
        OptT v{};
        const char *end = _detail::strend(str);
        auto [ptr, ec] = std::from_chars(str, end, v);
        if (ec == std::errc{})
            if (ptr == end)
                return v;
            else
                return std::unexpected(std::errc::invalid_argument);
        else
            return std::unexpected(ec);
    }

    static std::string to_str(const OptT &value) {
        return std::to_string(value);
    }
};

template <>
struct string_converter<char> {
    static auto from_str(const char *str) -> std::expected<char, std::errc> {
        if (std::strlen(str) != 1)
            return std::unexpected(std::errc::invalid_argument);
        return *str;
    }

    static std::string to_str(const char &value) {
        return std::string(1, value);
    }
};

template <>
struct string_converter<const char *> {
    static auto from_str(const char *str)
        -> std::expected<const char *, std::errc> {
        return str;
    }

    static std::string to_str(const char *const &value) { return value; }
};

template <>
struct string_converter<std::string> {
    static auto from_str(const char *str)
        -> std::expected<std::string, std::errc> {
        return std::string(str);
    }

    static std::string to_str(const std::string &value) { return value; }
};

template <typename ArgsGroupT>
concept args_group =
    std::derived_from<ArgsGroupT, information> &&
    std::default_initializable<ArgsGroupT> && std::movable<ArgsGroupT>;

template <typename OptT>
concept option = std::semiregular<OptT> && string_convertable<OptT>;

namespace _detail {
    enum {
        NORMAL,
        BOOLEAN,
        COUNTER,
        VECTOR,
    };

    template <typename OptT>
    struct opt_type {};

    template <option OptT>
    struct opt_type<OptT> {
        static constexpr size_t type = NORMAL;
    };

    template <>
    struct opt_type<bool> {
        static constexpr size_t type = BOOLEAN;
    };

    template <>
    struct opt_type<counter> {
        static constexpr size_t type = COUNTER;
    };

    template <option OptT>
    struct opt_type<std::vector<OptT>> {
        static constexpr size_t type = VECTOR;
    };

    template <typename OptT>
    constexpr size_t opt_type_v = opt_type<OptT>::type;

    class opt_base {
      public:
        opt_base();
        opt_base(const opt_base &) = default;
        opt_base(opt_base &&other);
        virtual ~opt_base() = default;
        opt_base &operator=(const opt_base &) = default;
        opt_base &operator=(opt_base &&other);

        inline char get_shrt() const;
        inline const std::string &get_lng() const;
        inline const std::string &get_about() const;
        inline bool already_set() const;
        virtual const std::string &get_argname() const;
        virtual bool get_required() const;
        virtual bool get_allow_hyphen() const;
        virtual std::string get_def() const;
        virtual std::errc set(const char *value = nullptr) = 0;
        virtual bool need_argument() const;

      protected:
        char _shrt;
        std::string _lng;
        std::string _about;
        bool _set_flag;
    };

    template <typename OptT>
    class opt_wrapper {
        opt_wrapper() = delete;
    };

    template <option OptT>
    class opt_wrapper<OptT> : public opt_base {
      public:
        opt_wrapper(OptT &optref);
        opt_wrapper(const opt_wrapper &) = default;
        opt_wrapper(opt_wrapper &&other);
        ~opt_wrapper() = default;
        opt_wrapper &operator=(const opt_wrapper &) = default;
        opt_wrapper &operator=(opt_wrapper &&other);

        opt_wrapper &shrt(char value) &;
        opt_wrapper &&shrt(char value) &&;
        opt_wrapper &lng(const std::string &value) &;
        opt_wrapper &&lng(const std::string &value) &&;
        opt_wrapper &about(const std::string &value) &;
        opt_wrapper &&about(const std::string &value) &&;
        opt_wrapper &argname(const std::string &value) &;
        opt_wrapper &&argname(const std::string &value) &&;
        opt_wrapper &required() &;
        opt_wrapper &&required() &&;
        opt_wrapper &allow_hyphen() &;
        opt_wrapper &&allow_hyphen() &&;
        template <typename... DefT>
        opt_wrapper &def(DefT &&...value) &
            requires std::constructible_from<OptT, DefT...>;
        template <typename... DefT>
        opt_wrapper &&def(DefT &&...value) &&
            requires std::constructible_from<OptT, DefT...>;

      private:
        const std::string &get_argname() const override;
        bool get_required() const override;
        bool get_allow_hyphen() const override;
        std::string get_def() const override;
        std::errc set(const char *value = nullptr) override;
        bool need_argument() const override;

        std::reference_wrapper<OptT> _optref;
        bool _required;
        bool _allow_hyphen;
        std::string _argname;
    };

    template <>
    class opt_wrapper<bool> : public opt_base {
      public:
        opt_wrapper(bool &optref);
        opt_wrapper(const opt_wrapper &) = default;
        opt_wrapper(opt_wrapper &&other);
        ~opt_wrapper() = default;
        opt_wrapper &operator=(const opt_wrapper &) = default;
        opt_wrapper &operator=(opt_wrapper &&other);

        opt_wrapper &shrt(char value) &;
        opt_wrapper &&shrt(char value) &&;
        opt_wrapper &lng(const std::string &value) &;
        opt_wrapper &&lng(const std::string &value) &&;
        opt_wrapper &about(const std::string &value) &;
        opt_wrapper &&about(const std::string &value) &&;

      private:
        std::errc set(const char *value = nullptr) override;

        std::reference_wrapper<bool> _optref;
    };

    template <>
    class opt_wrapper<counter> : public opt_base {
      public:
        opt_wrapper(counter &optref);
        opt_wrapper(const opt_wrapper &) = default;
        opt_wrapper(opt_wrapper &&other);
        ~opt_wrapper() = default;
        opt_wrapper &operator=(const opt_wrapper &) = default;
        opt_wrapper &operator=(opt_wrapper &&other);

        opt_wrapper &shrt(char value) &;
        opt_wrapper &&shrt(char value) &&;
        opt_wrapper &lng(const std::string &value) &;
        opt_wrapper &&lng(const std::string &value) &&;
        opt_wrapper &about(const std::string &value) &;
        opt_wrapper &&about(const std::string &value) &&;

      private:
        std::errc set(const char *value = nullptr) override;

        std::reference_wrapper<counter> _optref;
    };

    template <option OptT>
    class opt_wrapper<std::vector<OptT>> : public opt_base {
      public:
        opt_wrapper(std::vector<OptT> &optref);
        opt_wrapper(const opt_wrapper &) = default;
        opt_wrapper(opt_wrapper &&other);
        ~opt_wrapper() = default;
        opt_wrapper &operator=(const opt_wrapper &) = default;
        opt_wrapper &operator=(opt_wrapper &&other);

        opt_wrapper &shrt(char value) &;
        opt_wrapper &&shrt(char value) &&;
        opt_wrapper &lng(const std::string &value) &;
        opt_wrapper &&lng(const std::string &value) &&;
        opt_wrapper &about(const std::string &value) &;
        opt_wrapper &&about(const std::string &value) &&;
        opt_wrapper &argname(const std::string &value) &;
        opt_wrapper &&argname(const std::string &value) &&;
        opt_wrapper &allow_hyphen() &;
        opt_wrapper &&allow_hyphen() &&;

      private:
        const std::string &get_argname() const override;
        bool get_allow_hyphen() const override;
        std::errc set(const char *value = nullptr) override;
        bool need_argument() const override;

        std::reference_wrapper<std::vector<OptT>> _optref;
        bool _allow_hyphen;
        std::string _argname;
    };

    class anyopt {
      public:
        template <typename OptT>
        anyopt(const opt_wrapper<OptT> &origin);
        template <typename OptT>
        anyopt(opt_wrapper<OptT> &&origin);
        anyopt(const anyopt &other) = delete;
        anyopt(anyopt &&other) = default;

        const size_t opttype;

        inline char shrt() const;
        inline const std::string &lng() const;
        inline const std::string &about() const;
        inline const std::string &argname() const;
        inline bool required() const;
        inline bool allow_hyphen() const;
        inline std::string def() const;
        inline std::errc set(const char *value = nullptr);
        inline bool already_set() const;
        inline bool need_argument() const;

      private:
        std::unique_ptr<opt_base> _origin;
    };

    const std::string &opt_base::get_argname() const {
        static const std::string empty;
        return empty;
    }

    opt_base::opt_base() : _shrt{'\0'}, _lng{}, _about{}, _set_flag{false} {}

    opt_base::opt_base(opt_base &&other) :
        _shrt{other._shrt},
        _lng(std::move(other._lng)),
        _about(std::move(other._about)),
        _set_flag(other._set_flag) {
        other._shrt = '\0';
        other._set_flag = false;
    }

    opt_base &opt_base::operator=(opt_base &&other) {
        _shrt = other._shrt;
        other._shrt = '\0';
        _lng = std::move(other._lng);
        _about = std::move(other._about);
        _set_flag = other._set_flag;
        other._set_flag = false;
        return *this;
    }

    char opt_base::get_shrt() const { return _shrt; }

    const std::string &opt_base::get_lng() const { return _lng; }

    const std::string &opt_base::get_about() const { return _about; }

    bool opt_base::already_set() const { return _set_flag; }

    bool opt_base::get_required() const { return false; }

    bool opt_base::get_allow_hyphen() const { return false; }

    std::string opt_base::get_def() const { return {}; }

    bool opt_base::need_argument() const { return false; }

    template <option OptT>
    opt_wrapper<OptT>::opt_wrapper(OptT &optref) :
        opt_base{},
        _optref{optref},
        _required{false},
        _allow_hyphen{false},
        _argname{} {}

    template <option OptT>
    opt_wrapper<OptT>::opt_wrapper(opt_wrapper &&other) :
        opt_base(std::move(other)), _optref(other._optref) {
        _required = other._required;
        _allow_hyphen = other._allow_hyphen;
        _argname = std::move(other._argname);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::operator=(opt_wrapper &&other) {
        opt_base::operator=(std::move(other));
        _optref = other.optref;
        _required = other._required;
        _allow_hyphen = other._allow_hyphen;
        _argname = std::move(other._argname);
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::shrt(char value) & {
        _shrt = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::shrt(char value) && {
        _shrt = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::lng(const std::string &value) & {
        _lng = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::lng(const std::string &value) && {
        _lng = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::about(const std::string &value) & {
        _about = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::about(const std::string &value) && {
        _about = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::required() & {
        _required = true;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::required() && {
        _required = true;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::allow_hyphen() & {
        _allow_hyphen = true;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::allow_hyphen() && {
        _allow_hyphen = true;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::argname(const std::string &value) & {
        _argname = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::argname(
        const std::string &value) && {
        _argname = value;
        return std::move(*this);
    }

    template <option OptT>
    template <typename... DefT>
    opt_wrapper<OptT> &opt_wrapper<OptT>::def(DefT &&...value) &
        requires std::constructible_from<OptT, DefT...>
    {
        _optref.get() = OptT(std::forward<DefT>(value)...);
        return *this;
    }

    template <option OptT>
    template <typename... DefT>
    opt_wrapper<OptT> &&opt_wrapper<OptT>::def(DefT &&...value) &&
        requires std::constructible_from<OptT, DefT...>
    {
        _optref.get() = OptT(std::forward<DefT>(value)...);
        return std::move(*this);
    }

    template <option OptT>
    const std::string &opt_wrapper<OptT>::get_argname() const {
        return _argname;
    }

    template <option OptT>
    bool opt_wrapper<OptT>::get_required() const {
        return _required;
    }

    template <option OptT>
    bool opt_wrapper<OptT>::get_allow_hyphen() const {
        return _allow_hyphen;
    }

    template <option OptT>
    std::string opt_wrapper<OptT>::get_def() const {
        return string_converter<OptT>::to_str(_optref.get());
    }

    template <option OptT>
    std::errc opt_wrapper<OptT>::set(const char *value) {
        std::expected<OptT, std::errc> expt =
            string_converter<OptT>::from_str(value);
        if (expt) {
            _optref.get() = std::move(expt.value());
            _set_flag = true;
        } else {
            return expt.error();
        }

        return {};
    }

    template <option OptT>
    bool opt_wrapper<OptT>::need_argument() const {
        return true;
    }

    opt_wrapper<bool>::opt_wrapper(bool &optref) :
        opt_base{}, _optref(optref) {}

    opt_wrapper<bool>::opt_wrapper(opt_wrapper &&other) :
        opt_base{std::move(other)}, _optref(other._optref) {}

    opt_wrapper<bool> &opt_wrapper<bool>::operator=(opt_wrapper &&other) {
        opt_base::operator=(std::move(other));
        _optref = other._optref;
        return *this;
    }

    opt_wrapper<bool> &opt_wrapper<bool>::shrt(char value) & {
        _shrt = value;
        return *this;
    }

    opt_wrapper<bool> &&opt_wrapper<bool>::shrt(char value) && {
        _shrt = value;
        return std::move(*this);
    }

    opt_wrapper<bool> &opt_wrapper<bool>::lng(const std::string &value) & {
        _lng = value;
        return *this;
    }

    opt_wrapper<bool> &&opt_wrapper<bool>::lng(const std::string &value) && {
        _lng = value;
        return std::move(*this);
    }

    opt_wrapper<bool> &opt_wrapper<bool>::about(const std::string &value) & {
        _about = value;
        return *this;
    }

    opt_wrapper<bool> &&opt_wrapper<bool>::about(const std::string &value) && {
        _about = value;
        return std::move(*this);
    }

    std::errc opt_wrapper<bool>::set(const char *value) {
        (void)value;
        _optref.get() = true;
        _set_flag = true;
        return {};
    }

    opt_wrapper<counter>::opt_wrapper(counter &optref) :
        opt_base{}, _optref(optref) {}

    opt_wrapper<counter>::opt_wrapper(opt_wrapper &&other) :
        opt_base{std::move(other)}, _optref(other._optref) {}

    opt_wrapper<counter> &opt_wrapper<counter>::operator=(opt_wrapper &&other) {
        opt_base::operator=(std::move(other));
        _optref = other._optref;
        return *this;
    }

    opt_wrapper<counter> &opt_wrapper<counter>::shrt(char value) & {
        _shrt = value;
        return *this;
    }

    opt_wrapper<counter> &&opt_wrapper<counter>::shrt(char value) && {
        _shrt = value;
        return std::move(*this);
    }

    opt_wrapper<counter> &opt_wrapper<counter>::lng(
        const std::string &value) & {
        _lng = value;
        return *this;
    }

    opt_wrapper<counter> &&opt_wrapper<counter>::lng(
        const std::string &value) && {
        _lng = value;
        return std::move(*this);
    }

    opt_wrapper<counter> &opt_wrapper<counter>::about(
        const std::string &value) & {
        _about = value;
        return *this;
    }

    opt_wrapper<counter> &&opt_wrapper<counter>::about(
        const std::string &value) && {
        _about = value;
        return std::move(*this);
    }

    std::errc opt_wrapper<counter>::set(const char *value) {
        (void)value;
        ++_optref.get();
        return {};
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>>::opt_wrapper(std::vector<OptT> &optref) :
        opt_base{}, _optref(optref), _allow_hyphen(false), _argname{} {}

    template <option OptT>
    opt_wrapper<std::vector<OptT>>::opt_wrapper(opt_wrapper &&other) :
        opt_base(std::move(other)), _optref(other._optref) {
        _allow_hyphen = other._allow_hyphen;
        _argname = std::move(other._argname);
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &opt_wrapper<std::vector<OptT>>::operator=(
        opt_wrapper &&other) {
        opt_base::operator=(std::move(other));
        _optref = other._optref;
        _allow_hyphen = other._allow_hyphen;
        _argname = std::move(other._argname);
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &opt_wrapper<std::vector<OptT>>::shrt(
        char value) & {
        _shrt = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &&opt_wrapper<std::vector<OptT>>::shrt(
        char value) && {
        _shrt = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &opt_wrapper<std::vector<OptT>>::lng(
        const std::string &value) & {
        _lng = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &&opt_wrapper<std::vector<OptT>>::lng(
        const std::string &value) && {
        _lng = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &opt_wrapper<std::vector<OptT>>::about(
        const std::string &value) & {
        _about = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &&opt_wrapper<std::vector<OptT>>::about(
        const std::string &value) && {
        _about = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &opt_wrapper<std::vector<OptT>>::argname(
        const std::string &value) & {
        _argname = value;
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &&opt_wrapper<std::vector<OptT>>::argname(
        const std::string &value) && {
        _argname = value;
        return std::move(*this);
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &
    opt_wrapper<std::vector<OptT>>::allow_hyphen() & {
        _allow_hyphen = true;
        return *this;
    }

    template <option OptT>
    opt_wrapper<std::vector<OptT>> &&
    opt_wrapper<std::vector<OptT>>::allow_hyphen() && {
        _allow_hyphen = true;
        return std::move(*this);
    }

    template <option OptT>
    const std::string &opt_wrapper<std::vector<OptT>>::get_argname() const {
        return _argname;
    }

    template <option OptT>
    bool opt_wrapper<std::vector<OptT>>::get_allow_hyphen() const {
        return _allow_hyphen;
    }

    template <option OptT>
    std::errc opt_wrapper<std::vector<OptT>>::set(const char *value) {
        std::expected<OptT, std::errc> expt =
            string_converter<OptT>::from_str(value);

        if (expt)
            _optref.get().emplace_back(std::move(expt.value()));
        else
            return expt.error();

        return {};
    }

    template <option OptT>
    bool opt_wrapper<std::vector<OptT>>::need_argument() const {
        return true;
    }

    template <typename OptT>
    anyopt::anyopt(const opt_wrapper<OptT> &origin) :
        opttype(opt_type_v<OptT>), _origin(new opt_wrapper<OptT>(origin)) {}

    template <typename OptT>
    anyopt::anyopt(opt_wrapper<OptT> &&origin) :
        opttype(opt_type_v<OptT>),
        _origin(new opt_wrapper<OptT>(std::move(origin))) {}

    char anyopt::shrt() const { return _origin.get()->get_shrt(); }

    const std::string &anyopt::lng() const { return _origin.get()->get_lng(); }

    const std::string &anyopt::about() const {
        return _origin.get()->get_about();
    }

    const std::string &anyopt::argname() const {
        return _origin.get()->get_argname();
    }

    bool anyopt::required() const { return _origin.get()->get_required(); }

    bool anyopt::allow_hyphen() const {
        return _origin.get()->get_allow_hyphen();
    }

    std::string anyopt::def() const { return _origin.get()->get_def(); }

    std::errc anyopt::set(const char *value) {
        return _origin.get()->set(value);
    }

    bool anyopt::already_set() const { return _origin.get()->already_set(); }

    inline bool anyopt::need_argument() const {
        return _origin.get()->need_argument();
    }

    std::string get_argname(const greet::_detail::anyopt &optref) {
        if (!optref.argname().empty())
            return optref.argname();
        else if (!optref.lng().empty())
            return uppercase(optref.lng());
        else
            return "VALUE";
    }
}  // namespace _detail

class meta {
  public:
    template <typename... OptionTs>
    meta(OptionTs &&...options);
    meta(const meta &) = delete;
    meta(meta &&) = delete;

    auto opts() -> std::vector<_detail::anyopt> &;
    auto ignored_args() -> std::optional<std::reference_wrapper<ignored>>;
    auto required_opts()
        -> const std::vector<std::reference_wrapper<_detail::anyopt>> &;
    auto query(const std::string &flag)
        -> std::optional<std::reference_wrapper<_detail::anyopt>>;
    inline bool help() const;
    inline bool version() const;

  private:
    template <typename FirstOptionT, typename... OptionTs>
    inline void _unpack_opts(FirstOptionT &&first, OptionTs &&...options);

    bool _help, _version;
    std::vector<_detail::anyopt> _opts;
    std::optional<std::reference_wrapper<ignored>> _ignored_args;
    std::vector<std::reference_wrapper<_detail::anyopt>> _required_opts;
    std::unordered_map<std::string, std::reference_wrapper<_detail::anyopt>>
        _sorted_by_flag;
};

namespace _detail {
    class print_helper {
      public:
        print_helper(std::string &&program_name, meta &m);
        print_helper(const print_helper &) = delete;
        print_helper(print_helper &&) = delete;

        void print_usage() const;
        void print_options() const;
        [[noreturn]] static void internal_error(const std::string &msg);
        [[noreturn]] void unexpected_argument(const std::string &arg) const;
        [[noreturn]] void missing_value(
            const std::string &flag, const anyopt &optref) const;
        [[noreturn]] void unexpected_value(
            const std::string &flag, const std::string &value) const;
        [[noreturn]] void invalid_value(
            const std::string &flag, const std::string &value,
            const anyopt &optref, std::errc ec) const;
        [[noreturn]] void used_mutiple(
            const std::string &flag, const anyopt &optref) const;
        [[noreturn]] void missing_options(
            const std::vector<std::reference_wrapper<_detail::anyopt>> &opts)
            const;

      private:
        std::string _program_name;
        meta &_metaref;
    };

    print_helper::print_helper(std::string &&program_name, meta &m) :
        _program_name(std::move(program_name)), _metaref(m) {}

    void print_helper::print_usage() const {
        std::cout << "Usage: " << _program_name << " [OPTIONS]";
        for (const auto &optref : _metaref.required_opts()) {
            if (optref.get().lng().empty())
                std::cout << std::format(
                    " -{} <{}>",
                    optref.get().shrt(),
                    get_argname(optref.get()));
            else
                std::cout << std::format(
                    " --{} <{}>",
                    optref.get().lng(),
                    get_argname(optref.get()));
        }
        std::cout << std::endl;
    }

    void print_helper::print_options() const {
        std::cout << "Options:" << std::endl;

        size_t fixed_width = 0;
        for (const auto &optref : _metaref.opts()) {
            size_t width = 8;
            if (!optref.lng().empty()) width += 2 + optref.lng().size();
            if (optref.need_argument()) width += 3 + get_argname(optref).size();
            fixed_width = std::max(fixed_width, width);
        }

        for (const auto &optref : _metaref.opts()) {
            std::string item = "  ";
            if (optref.shrt() == '\0')
                item += "   ";
            else if (optref.lng().empty())
                item += std::format("-{}", optref.shrt());
            else
                item += std::format("-{},", optref.shrt());

            if (!optref.lng().empty())
                item += std::format(" --{}", optref.lng());

            if (optref.need_argument())
                item += std::format(" <{}>", get_argname(optref));
            item.resize(fixed_width, ' ');
            std::cout << item << optref.about();
            if (optref.opttype == NORMAL) {
                if (optref.required())
                    std::cout << " [REQUIRED]";
                else
                    std::cout << std::format(" [default: {}]", optref.def());
            }
            std::cout << std::endl;
        }
    }

    [[noreturn]] void print_helper::internal_error(const std::string &msg) {
        std::cerr << "[internal error]: " << msg << std::endl;
        std::exit(1);
    }

    [[noreturn]] void print_helper::unexpected_argument(
        const std::string &arg) const {
        std::cerr << std::format("error: unexpected argument '{}' found", arg)
                  << std::endl;
        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }

    [[noreturn]] void print_helper::missing_value(
        const std::string &flag, const anyopt &optref) const {
        std::cerr << std::format(
                         "error: a value is required for '{} <{}>' but "
                         "none was supplied",
                         flag,
                         get_argname(optref))
                  << std::endl;
        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }

    [[noreturn]] void print_helper::unexpected_value(
        const std::string &flag, const std::string &value) const {
        std::cerr << std::format(
                         "error: unexpected value '{}' for '{}' found; no more "
                         "were expected",
                         value,
                         flag)
                  << std::endl;
        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }

    [[noreturn]] void print_helper::invalid_value(
        const std::string &flag, const std::string &value, const anyopt &optref,
        std::errc ec) const {
        std::cerr << std::format(
                         "error: invalid value '{}' for '{} <{}>': {}",
                         value,
                         flag,
                         get_argname(optref),
                         std::make_error_code(ec).message())
                  << std::endl;
        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }

    [[noreturn]] void print_helper::used_mutiple(
        const std::string &flag, const anyopt &optref) const {
        if (optref.need_argument())
            std::cerr << std::format(
                             "error: the argument '{} <{}>' cannot be used "
                             "multiple times",
                             flag,
                             get_argname(optref))
                      << std::endl;
        else
            std::cerr << std::format(
                             "error: the argument '{}' cannot be used "
                             "multiple times",
                             flag)
                      << std::endl;
        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }

    [[noreturn]] void print_helper::missing_options(
        const std::vector<std::reference_wrapper<_detail::anyopt>> &opts)
        const {
        std::cerr
            << "error: the following required arguments were not provided:"
            << std::endl;
        for (const auto &optref : opts)
            if (optref.get().lng().empty())
                std::cout << std::format(
                                 "  -{} <{}>",
                                 optref.get().shrt(),
                                 get_argname(optref.get()))
                          << std::endl;
            else
                std::cout << std::format(
                                 "  --{} <{}>",
                                 optref.get().lng(),
                                 get_argname(optref.get()))
                          << std::endl;

        std::cout << std::endl;
        print_usage();
        std::cout << std::endl;
        std::cout << "For more information, try '--help'." << std::endl;
        std::exit(2);
    }
}  // namespace _detail

template <typename OptT>
auto opt(OptT &optref) {
    if constexpr (std::is_same_v<std::decay_t<OptT>, ignored>)
        return std::ref(optref);
    else
        return _detail::opt_wrapper<OptT>(optref);
};

counter::counter() : _counter{0} {}

counter::counter(counter &&other) {
    _counter = other._counter;
    other._counter = 0;
}

counter &counter::operator=(counter &&other) {
    _counter = other._counter;
    other._counter = 0;
    return *this;
};

counter &counter::operator++() {
    ++_counter;
    return *this;
}

counter counter::operator++(int) {
    counter prev = *this;
    ++_counter;
    return prev;
}

counter::operator size_t() { return _counter; }

template <typename... OptionTs>
meta::meta(OptionTs &&...options) :
    _help{false},
    _version{false},
    _opts{},
    _ignored_args(std::nullopt),
    _required_opts{},
    _sorted_by_flag{} {
    constexpr size_t ignored_opt_nums = _detail::
        type_positions_t<std::reference_wrapper<ignored>, OptionTs...>::size();
    static_assert(
        ignored_opt_nums <= 1,
        "can only provide 0 or 1 `greet::ignored` option!");

    _opts.reserve(sizeof...(OptionTs) + 2 - ignored_opt_nums);
    _unpack_opts(std::forward<OptionTs>(options)...);

    _opts.emplace_back(
        _detail::anyopt(opt(_help).shrt('h').lng("help").about("Print help")));
    _opts.emplace_back(_detail::anyopt(
        opt(_version).shrt('V').lng("version").about("Print version")));

    _sorted_by_flag.reserve(sizeof...(OptionTs) * 2);

    for (auto &optref : _opts) {
        if (optref.shrt() == '\0' && optref.lng().empty())
            _detail::print_helper::internal_error(
                "there is an option that specifies neither short nor long "
                "flags.");

        bool inserted;
        if (optref.shrt() != '\0') {
            if (optref.shrt() < '!' || optref.shrt() > '~')
                _detail::print_helper::internal_error(
                    "the short flag must be a printable character.");
            if (optref.shrt() == '-')
                _detail::print_helper::internal_error(
                    "the short flag cannot be '-'.");

            std::tie(std::ignore, inserted) = _sorted_by_flag.emplace(
                std::string("-") + optref.shrt(), std::ref(optref));
            if (!inserted)
                _detail::print_helper::internal_error(std::format(
                    "the flag '-{}' is already be used.", optref.shrt()));
        }
        if (!optref.lng().empty()) {
            std::tie(std::ignore, inserted) = _sorted_by_flag.emplace(
                std::string("--") + optref.lng(), std::ref(optref));
            if (!inserted)
                _detail::print_helper::internal_error(std::format(
                    "the flag '--{}' is already be used.", optref.lng()));
        }

        if (optref.required()) _required_opts.emplace_back(std::ref(optref));
    };
}

auto meta::opts() -> std::vector<_detail::anyopt> & { return _opts; }

auto meta::ignored_args() -> std::optional<std::reference_wrapper<ignored>> {
    return _ignored_args;
}

auto meta::required_opts()
    -> const std::vector<std::reference_wrapper<_detail::anyopt>> & {
    return _required_opts;
}

auto meta::query(const std::string &flag)
    -> std::optional<std::reference_wrapper<_detail::anyopt>> {
    if (_sorted_by_flag.contains(flag))
        return _sorted_by_flag.at(flag);
    else
        return std::nullopt;
};

inline bool meta::help() const { return _help; }

inline bool meta::version() const { return _version; }

template <typename FirstOptionT, typename... OptionTs>
inline void meta::_unpack_opts(FirstOptionT &&first, OptionTs &&...options) {
    if constexpr (std::is_same_v<
                      std::decay_t<FirstOptionT>,
                      std::reference_wrapper<ignored>>)
        _ignored_args = first;
    else
        _opts.emplace_back(_detail::anyopt(std::forward<FirstOptionT>(first)));

    if constexpr (sizeof...(OptionTs))
        _unpack_opts(std::forward<OptionTs>(options)...);
}

template <args_group ArgsGroupT>
ArgsGroupT greet(int argc, char *argv[]) {
    ArgsGroupT args{};
    meta m = args.genmeta();
    _detail::print_helper printer(_detail::filename(argv[0]), m);

    _detail::remove_one_arg(argc, argv);

    while (argc) {
        size_t argtype = _detail::argtype(argv[0]);

        auto parse_helper = [&](const std::string &flag, bool newarg) -> bool {
            auto result = m.query(flag);
            if (!result) printer.unexpected_argument(flag);
            auto &optref = result.value();

            if (optref.get().need_argument()) {
                if (!argc) printer.missing_value(flag, optref.get());
                if (newarg) {
                    if (argv[0][0] == '-' && !optref.get().allow_hyphen())
                        printer.missing_value(flag, optref.get());
                } else if (argv[0][0] == '=')
                    argv[0] += 1;

                if (optref.get().already_set())
                    printer.used_mutiple(flag, optref.get());
                std::errc ec = optref.get().set(argv[0]);
                if (ec != std::errc{})
                    printer.invalid_value(flag, argv[0], optref.get(), ec);
                _detail::remove_one_arg(argc, argv);
                return true;
            } else {
                if (argtype == _detail::LONG && !newarg && argv[0][0] == '=')
                    printer.unexpected_value(flag, argv[0]);
                if (optref.get().already_set())
                    printer.used_mutiple(flag, optref.get());
                optref.get().set();
                return false;
            }
        };

        switch (argtype) {
            case _detail::SHORT: {
                ++argv[0];
                while (true) {
                    std::string flag = "-";
                    flag += argv[0][0];
                    ++argv[0];
                    if (argv[0][0] == '\0') {
                        _detail::remove_one_arg(argc, argv);
                        parse_helper(flag, true);
                        break;
                    }
                    if (parse_helper(flag, false)) break;
                };
            } break;
            case _detail::LONG: {
                char *split_pos = std::strchr(argv[0], '=');
                if (split_pos) {
                    std::string flag(argv[0], split_pos);
                    argv[0] = split_pos;
                    parse_helper(flag, false);
                } else {
                    std::string flag(argv[0]);
                    _detail::remove_one_arg(argc, argv);
                    parse_helper(flag, true);
                }
            } break;
            case _detail::ARGUMENT:
                printer.unexpected_argument(argv[0]);
                break;
            case _detail::ENDARG: {
                _detail::remove_one_arg(argc, argv);
                auto ignored_args = m.ignored_args();
                if (ignored_args) ignored_args.value().get().reserve(argc);
                while (argc) {
                    if (ignored_args)
                        ignored_args.value().get().emplace_back(argv[0]);
                    _detail::remove_one_arg(argc, argv);
                }
            } break;
            default:
                std::unreachable();
        }

        if (m.help()) {
            std::cout << args.description() << std::endl;
            std::cout << std::endl;
            printer.print_usage();
            std::cout << std::endl;
            printer.print_options();
            std::exit(0);
        }
        if (m.version()) {
            std::cout << args.version() << std::endl;
            std::exit(0);
        }
    }

    std::vector<std::reference_wrapper<_detail::anyopt>> missing_opts{};
    for (const auto &optref : m.required_opts())
        if (!optref.get().already_set()) missing_opts.emplace_back(optref);
    if (missing_opts.size()) printer.missing_options(missing_opts);

    return args;
};

}  // namespace greet

#endif
