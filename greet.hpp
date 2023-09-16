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

#include <charconv>
#include <compare>
#include <concepts>
#include <cstring>
#include <expected>
#include <format>
#include <functional>
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
    requires(Tp t, const char* s) { std::from_chars(s, s, t); };

template <typename Tp>
concept integer = std::integral<Tp> && from_chars<Tp>;

template <typename Tp>
concept float_pointer = std::floating_point<Tp> && from_chars<Tp>;

enum {
    ARGUMENT,
    SHORT,
    LONG,
};

inline size_t argtype(const char* arg) {
    if (std::strlen(arg) >= 2)
        if (arg[0] == '-') {
            if (arg[1] == '-')
                return LONG;
            else
                return SHORT;
        }

    return ARGUMENT;
};

inline const char* strend(const char* str) {
    while (*str != '\0') ++str;
    return str;
}

inline std::string uppercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

inline std::string filename(std::string const& path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

inline void remove_one_arg(int& argc, char**(&argv)) {
    --argc;
    ++argv;
}
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
    counter(const counter& other);
    counter(counter&& other);
    counter& operator=(const counter& other);
    counter& operator=(counter&& other);

    counter& operator++();
    counter operator++(int);
    operator size_t();

   private:
    size_t _counter;
};

template <typename OptT>
struct string_converter;

template <typename OptT>
concept string_convertable = requires(OptT t, const char* str) {
    {
        string_converter<OptT>::from_str(str)
    } -> std::same_as<std::expected<OptT, std::errc>>;
    { string_converter<OptT>::to_str(t) } -> std::same_as<std::string>;
};

template <_detail::integer OptT>
struct string_converter<OptT> {
    static auto from_str(const char* str) -> std::expected<OptT, std::errc>;
    static std::string to_str(const OptT& value);
};

template <_detail::float_pointer OptT>
struct string_converter<OptT> {
    static auto from_str(const char* str) -> std::expected<OptT, std::errc>;
    static std::string to_str(const OptT& value);
};

template <>
struct string_converter<char> {
    static auto from_str(const char* str) -> std::expected<char, std::errc>;
    static std::string to_str(const char& value);
};

template <>
struct string_converter<const char*> {
    static auto from_str(const char* str)
        -> std::expected<const char*, std::errc>;
    static std::string to_str(const char* const& value);
};

template <>
struct string_converter<std::string> {
    static auto from_str(const char* str)
        -> std::expected<std::string, std::errc>;
    static std::string to_str(const std::string& value);
};

template <typename ArgsGroupT>
concept args_group =
    std::derived_from<ArgsGroupT, information> &&
    std::default_initializable<ArgsGroupT> && std::movable<ArgsGroupT>;

template <typename OptT>
concept option = std::semiregular<OptT> && string_convertable<OptT>;

namespace _detail {
class anyopt;

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
    using realopt = OptT;
};

template <typename OptT>
constexpr size_t opt_type_v = opt_type<OptT>::type;

template <typename OptT>
using realopt_t = typename opt_type<OptT>::realopt;

template <typename OptT>
class opt_wrapper {
    opt_wrapper() = delete;
};

template <option OptT>
class opt_wrapper<OptT> {
   public:
    opt_wrapper(OptT& optref);
    opt_wrapper(const opt_wrapper&) = default;
    opt_wrapper(opt_wrapper&& other);
    ~opt_wrapper() = default;
    opt_wrapper& operator=(const opt_wrapper&) = default;
    opt_wrapper& operator=(opt_wrapper&& other);

    opt_wrapper& shrt(char value) &;
    opt_wrapper&& shrt(char value) &&;
    opt_wrapper& lng(const std::string& value) &;
    opt_wrapper&& lng(const std::string& value) &&;
    opt_wrapper& about(const std::string& value) &;
    opt_wrapper&& about(const std::string& value) &&;
    opt_wrapper& argname(const std::string& value) &;
    opt_wrapper&& argname(const std::string& value) &&;
    opt_wrapper& required() &;
    opt_wrapper&& required() &&;
    opt_wrapper& allow_hyphen() &;
    opt_wrapper&& allow_hyphen() &&;
    template <typename DefT>
        requires std::constructible_from<OptT, DefT>
    opt_wrapper& def(DefT&& value) &;
    template <typename DefT>
        requires std::constructible_from<OptT, DefT>
    opt_wrapper&& def(DefT&& value) &&;

   private:
    friend anyopt;

    std::reference_wrapper<OptT> _optref;
    bool _required;
    bool _allow_hyphen;
    char _shrt;
    std::string _lng;
    std::string _about;
    std::string _argname;
};

template <>
class opt_wrapper<bool> {
   public:
    opt_wrapper(bool& optref);
    opt_wrapper(const opt_wrapper&) = default;
    opt_wrapper(opt_wrapper&& other);
    ~opt_wrapper() = default;
    opt_wrapper& operator=(const opt_wrapper&) = default;
    opt_wrapper& operator=(opt_wrapper&& other);

    opt_wrapper& shrt(char value) &;
    opt_wrapper&& shrt(char value) &&;
    opt_wrapper& lng(const std::string& value) &;
    opt_wrapper&& lng(const std::string& value) &&;
    opt_wrapper& about(const std::string& value) &;
    opt_wrapper&& about(const std::string& value) &&;

   private:
    friend anyopt;

    std::reference_wrapper<bool> _optref;
    char _shrt;
    std::string _lng;
    std::string _about;
};

template <>
class opt_wrapper<counter> {
   public:
    opt_wrapper(counter& optref);
    opt_wrapper(const opt_wrapper&) = default;
    opt_wrapper(opt_wrapper&& other);
    ~opt_wrapper() = default;
    opt_wrapper& operator=(const opt_wrapper&) = default;
    opt_wrapper& operator=(opt_wrapper&& other);

    opt_wrapper& shrt(char value) &;
    opt_wrapper&& shrt(char value) &&;
    opt_wrapper& lng(const std::string& value) &;
    opt_wrapper&& lng(const std::string& value) &&;
    opt_wrapper& about(const std::string& value) &;
    opt_wrapper&& about(const std::string& value) &&;

   private:
    friend anyopt;

    std::reference_wrapper<counter> _optref;
    char _shrt;
    std::string _lng;
    std::string _about;
};

template <option OptT>
class opt_wrapper<std::vector<OptT>> {
   public:
    opt_wrapper(std::vector<OptT>& optref);
    opt_wrapper(const opt_wrapper&) = default;
    opt_wrapper(opt_wrapper&& other);
    ~opt_wrapper() = default;
    opt_wrapper& operator=(const opt_wrapper&) = default;
    opt_wrapper& operator=(opt_wrapper&& other);

    opt_wrapper& shrt(char value) &;
    opt_wrapper&& shrt(char value) &&;
    opt_wrapper& lng(const std::string& value) &;
    opt_wrapper&& lng(const std::string& value) &&;
    opt_wrapper& about(const std::string& value) &;
    opt_wrapper&& about(const std::string& value) &&;
    opt_wrapper& argname(const std::string& value) &;
    opt_wrapper&& argname(const std::string& value) &&;
    opt_wrapper& allow_hyphen() &;
    opt_wrapper&& allow_hyphen() &&;

   private:
    friend anyopt;

    std::reference_wrapper<std::vector<OptT>> _optref;
    bool _allow_hyphen;
    char _shrt;
    std::string _lng;
    std::string _about;
    std::string _argname;
};

class anyopt {
   public:
    template <typename OptT>
    anyopt(const opt_wrapper<OptT>& origin);
    template <typename OptT>
    anyopt(opt_wrapper<OptT>&& origin);
    anyopt(const anyopt& other) = delete;
    anyopt(anyopt&& other);

    const size_t opttype;

    inline char shrt() const;
    inline const std::string& lng() const;
    inline const std::string& about() const;
    inline const std::string& argname() const;
    inline bool required() const;
    inline bool allow_hyphen() const;
    inline std::string def() const;
    inline std::errc set(const char* value = nullptr);
    inline bool already_set() const;
    inline bool need_argument() const;

   private:
    template <typename OptT>
    inline void _initialize();

    template <typename OptT>
    char _shrt_inner() const;
    template <typename OptT>
    const std::string& _lng_inner() const;
    template <typename OptT>
    const std::string& _about_inner() const;
    template <typename OptT>
    const std::string& _argname_inner() const;
    template <typename OptT>
    bool _required_inner() const;
    template <typename OptT>
    bool _allow_hyphen_inner() const;
    template <typename OptT>
    std::string _def_inner() const;
    template <typename OptT>
    std::errc _set_inner(const char* value);

    char (anyopt::*_shrt)() const;
    const std::string& (anyopt::*_lng)() const;
    const std::string& (anyopt::*_about)() const;
    const std::string& (anyopt::*_argname)() const;
    bool (anyopt::*_required)() const;
    bool (anyopt::*_allow_hyphen)() const;
    std::string (anyopt::*_def)() const;
    std::errc (anyopt::*_set)(const char*);
    std::unique_ptr<void, void (*)(void*)> _origin;

    bool _set_flag;
    const std::string _empty_string;
};

template <option OptT>
opt_wrapper<OptT>::opt_wrapper(OptT& optref)
    : _optref{optref},
      _required{false},
      _allow_hyphen{false},
      _shrt{'\0'},
      _lng{},
      _about{},
      _argname{} {}

template <option OptT>
opt_wrapper<OptT>::opt_wrapper(opt_wrapper&& other) : _optref(other._optref) {
    _required = other._required;
    _allow_hyphen = other._allow_hyphen;
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    _argname = std::move(other._argname);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::operator=(opt_wrapper&& other) {
    _optref = other.optref;
    _required = other._required;
    _allow_hyphen = other._allow_hyphen;
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    _argname = std::move(other._argname);
    return *this;
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::shrt(char value) & {
    _shrt = value;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::shrt(char value) && {
    _shrt = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::lng(const std::string& value) & {
    _lng = value;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::lng(const std::string& value) && {
    _lng = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::about(const std::string& value) & {
    _about = value;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::about(const std::string& value) && {
    _about = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::required() & {
    _required = true;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::required() && {
    _required = true;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::allow_hyphen() & {
    _allow_hyphen = true;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::allow_hyphen() && {
    _allow_hyphen = true;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<OptT>& opt_wrapper<OptT>::argname(const std::string& value) & {
    _argname = value;
    return *this;
}

template <option OptT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::argname(const std::string& value) && {
    _argname = value;
    return std::move(*this);
}

template <option OptT>
template <typename DefT>
    requires std::constructible_from<OptT, DefT>
opt_wrapper<OptT>& opt_wrapper<OptT>::def(DefT&& value) & {
    _optref.get() = OptT{std::forward<DefT>(value)};
    return *this;
}

template <option OptT>
template <typename DefT>
    requires std::constructible_from<OptT, DefT>
opt_wrapper<OptT>&& opt_wrapper<OptT>::def(DefT&& value) && {
    _optref.get() = OptT{std::forward<DefT>(value)};
    return std::move(*this);
}

opt_wrapper<bool>::opt_wrapper(bool& optref)
    : _optref{optref}, _shrt{'\0'}, _lng{}, _about{} {}

opt_wrapper<bool>::opt_wrapper(opt_wrapper&& other) : _optref{other._optref} {
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
}

opt_wrapper<bool>& opt_wrapper<bool>::operator=(opt_wrapper&& other) {
    _optref = other._optref;
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    return *this;
}

opt_wrapper<bool>& opt_wrapper<bool>::shrt(char value) & {
    _shrt = value;
    return *this;
}

opt_wrapper<bool>&& opt_wrapper<bool>::shrt(char value) && {
    _shrt = value;
    return std::move(*this);
}

opt_wrapper<bool>& opt_wrapper<bool>::lng(const std::string& value) & {
    _lng = value;
    return *this;
}

opt_wrapper<bool>&& opt_wrapper<bool>::lng(const std::string& value) && {
    _lng = value;
    return std::move(*this);
}

opt_wrapper<bool>& opt_wrapper<bool>::about(const std::string& value) & {
    _about = value;
    return *this;
}

opt_wrapper<bool>&& opt_wrapper<bool>::about(const std::string& value) && {
    _about = value;
    return std::move(*this);
}

opt_wrapper<counter>::opt_wrapper(counter& optref)
    : _optref{optref}, _shrt{'\0'}, _lng{}, _about{} {}

opt_wrapper<counter>::opt_wrapper(opt_wrapper&& other)
    : _optref{other._optref} {
    _shrt = other._shrt;
    other._shrt = '\0';
    _lng = std::move(other._lng);
    _about = std::move(other._about);
}

opt_wrapper<counter>& opt_wrapper<counter>::operator=(opt_wrapper&& other) {
    _optref = other._optref;
    _shrt = other._shrt;
    other._shrt = '\0';
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    return *this;
}

opt_wrapper<counter>& opt_wrapper<counter>::shrt(char value) & {
    _shrt = value;
    return *this;
}

opt_wrapper<counter>&& opt_wrapper<counter>::shrt(char value) && {
    _shrt = value;
    return std::move(*this);
}

opt_wrapper<counter>& opt_wrapper<counter>::lng(const std::string& value) & {
    _lng = value;
    return *this;
}

opt_wrapper<counter>&& opt_wrapper<counter>::lng(const std::string& value) && {
    _lng = value;
    return std::move(*this);
}

opt_wrapper<counter>& opt_wrapper<counter>::about(const std::string& value) & {
    _about = value;
    return *this;
}

opt_wrapper<counter>&& opt_wrapper<counter>::about(
    const std::string& value) && {
    _about = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>::opt_wrapper(std::vector<OptT>& optref)
    : _optref{optref},
      _allow_hyphen{false},
      _shrt{'\0'},
      _lng{},
      _about{},
      _argname{} {}

template <option OptT>
opt_wrapper<std::vector<OptT>>::opt_wrapper(opt_wrapper&& other)
    : _optref{other._optref} {
    _allow_hyphen = other._allow_hyphen;
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    _argname = std::move(other._argname);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>& opt_wrapper<std::vector<OptT>>::operator=(
    opt_wrapper&& other) {
    _optref = other._optref;
    _allow_hyphen = other._allow_hyphen;
    _shrt = other._shrt;
    _lng = std::move(other._lng);
    _about = std::move(other._about);
    _argname = std::move(other._argname);
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>& opt_wrapper<std::vector<OptT>>::shrt(
    char value) & {
    _shrt = value;
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&& opt_wrapper<std::vector<OptT>>::shrt(
    char value) && {
    _shrt = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>& opt_wrapper<std::vector<OptT>>::lng(
    const std::string& value) & {
    _lng = value;
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&& opt_wrapper<std::vector<OptT>>::lng(
    const std::string& value) && {
    _lng = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>& opt_wrapper<std::vector<OptT>>::about(
    const std::string& value) & {
    _about = value;
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&& opt_wrapper<std::vector<OptT>>::about(
    const std::string& value) && {
    _about = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>& opt_wrapper<std::vector<OptT>>::argname(
    const std::string& value) & {
    _argname = value;
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&& opt_wrapper<std::vector<OptT>>::argname(
    const std::string& value) && {
    _argname = value;
    return std::move(*this);
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&
opt_wrapper<std::vector<OptT>>::allow_hyphen() & {
    _allow_hyphen = true;
    return *this;
}

template <option OptT>
opt_wrapper<std::vector<OptT>>&&
opt_wrapper<std::vector<OptT>>::allow_hyphen() && {
    _allow_hyphen = true;
    return std::move(*this);
}

template <typename OptT>
anyopt::anyopt(const opt_wrapper<OptT>& origin)
    : opttype{opt_type_v<OptT>},
      _origin{new opt_wrapper<OptT>{origin}, [](void* ptr) {
                  delete reinterpret_cast<opt_wrapper<OptT>*>(ptr);
              }} {
    _initialize<OptT>();
}

template <typename OptT>
anyopt::anyopt(opt_wrapper<OptT>&& origin)
    : opttype{opt_type_v<OptT>},
      _origin{new opt_wrapper<OptT>{std::move(origin)}, [](void* ptr) {
                  delete reinterpret_cast<opt_wrapper<OptT>*>(ptr);
              }} {
    _initialize<OptT>();
}

template <typename OptT>
void anyopt::_initialize() {
    _shrt = &anyopt::_shrt_inner<OptT>;
    _lng = &anyopt::_lng_inner<OptT>;
    _about = &anyopt::_about_inner<OptT>;
    _argname = &anyopt::_argname_inner<OptT>;
    _required = &anyopt::_required_inner<OptT>;
    _allow_hyphen = &anyopt::_allow_hyphen_inner<OptT>;
    _def = &anyopt::_def_inner<OptT>;
    _set = &anyopt::_set_inner<OptT>;
}

anyopt::anyopt(anyopt&& other)
    : opttype{other.opttype}, _origin{std::move(other._origin)} {
    _shrt = other._shrt;
    _lng = other._lng;
    _about = other._about;
    _argname = other._argname;
    _required = other._required;
    _allow_hyphen = other._allow_hyphen;
    _def = other._def;
    _set = other._set;
};

char anyopt::shrt() const { return (this->*_shrt)(); }

const std::string& anyopt::lng() const { return (this->*_lng)(); }

const std::string& anyopt::about() const { return (this->*_about)(); }

const std::string& anyopt::argname() const { return (this->*_argname)(); }

bool anyopt::required() const { return (this->*_required)(); }

bool anyopt::allow_hyphen() const { return (this->*_allow_hyphen)(); }

std::string anyopt::def() const { return (this->*_def)(); }

std::errc anyopt::set(const char* value) { return (this->*_set)(value); }

bool anyopt::already_set() const { return _set_flag; }

inline bool anyopt::need_argument() const {
    return opttype == NORMAL || opttype == VECTOR;
}

template <typename OptT>
char anyopt::_shrt_inner() const {
    return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_shrt;
}

template <typename OptT>
const std::string& anyopt::_lng_inner() const {
    return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_lng;
}

template <typename OptT>
const std::string& anyopt::_about_inner() const {
    return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_about;
}

template <typename OptT>
const std::string& anyopt::_argname_inner() const {
    constexpr size_t type = opt_type_v<OptT>;

    if constexpr (type == NORMAL || type == VECTOR)
        return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_argname;
    else
        return _empty_string;
}

template <typename OptT>
bool anyopt::_required_inner() const {
    if constexpr (opt_type_v<OptT> == NORMAL)
        return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_required;
    else
        return false;
}

template <typename OptT>
bool anyopt::_allow_hyphen_inner() const {
    constexpr size_t type = opt_type_v<OptT>;

    if constexpr (type == NORMAL || type == VECTOR)
        return reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())
            ->_allow_hyphen;
    else
        return false;
}

template <typename OptT>
std::string anyopt::_def_inner() const {
    if constexpr (opt_type_v<OptT> == NORMAL)
        return string_converter<OptT>::to_str(
            reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_optref.get());
    else
        return {};
}

template <typename OptT>
std::errc anyopt::_set_inner(const char* value) {
    constexpr size_t type = opt_type_v<OptT>;

    if constexpr (type == NORMAL) {
        std::expected<OptT, std::errc> expt =
            string_converter<OptT>::from_str(value);
        if (expt) {
            reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_optref.get() =
                std::move(expt.value());
            _set_flag = true;
        } else {
            return expt.error();
        }
    } else if constexpr (type == BOOLEAN) {
        reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_optref.get() =
            true;
        _set_flag = true;
    } else if constexpr (type == COUNTER) {
        ++(reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())->_optref.get());
    } else if constexpr (type == VECTOR) {
        std::expected<realopt_t<OptT>, std::errc> expt =
            string_converter<realopt_t<OptT>>::from_str(value);
        if (expt)
            reinterpret_cast<opt_wrapper<OptT>*>(_origin.get())
                ->_optref.get()
                .emplace_back(std::move(expt.value()));
        else
            return expt.error();
    }

    return std::errc{};
}

std::string get_argname(const greet::_detail::anyopt& optref) {
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
    meta(OptionTs&&... options);
    meta(const meta&) = delete;
    meta(meta&&) = default;

    auto opts() -> std::vector<_detail::anyopt>&;
    auto required_opts()
        -> const std::vector<std::reference_wrapper<_detail::anyopt>>&;
    auto query(const std::string& flag)
        -> std::optional<std::reference_wrapper<_detail::anyopt>>;
    inline bool help() const;
    inline bool version() const;

   private:
    bool _help, _version;
    std::vector<_detail::anyopt> _opts;
    std::vector<std::reference_wrapper<_detail::anyopt>> _required_opts;
    std::unordered_map<std::string, std::reference_wrapper<_detail::anyopt>>
        _sorted_by_flag;
};

namespace _detail {
class print_helper {
   public:
    print_helper(std::string&& program_name, meta& m);
    print_helper(const print_helper&) = delete;
    print_helper(print_helper&&) = delete;

    void print_usage() const;
    void print_options() const;
    [[noreturn]] static void internal_error(const std::string& msg);
    [[noreturn]] void unexpected_argument(const std::string& arg) const;
    [[noreturn]] void missing_value(const std::string& flag,
                                    const anyopt& optref) const;
    [[noreturn]] void unexpected_value(const std::string& flag,
                                       const std::string& value) const;
    [[noreturn]] void invalid_value(const std::string& flag,
                                    const std::string& value,
                                    const anyopt& optref, std::errc ec) const;
    [[noreturn]] void used_mutiple(const std::string& flag,
                                   const anyopt& optref) const;
    [[noreturn]] void missing_options(
        const std::vector<std::reference_wrapper<_detail::anyopt>>& opts) const;

   private:
    std::string _program_name;
    meta& _metaref;
};

print_helper::print_helper(std::string&& program_name, meta& m)
    : _program_name{std::move(program_name)}, _metaref{m} {}

void print_helper::print_usage() const {
    std::cout << "Usage: " << _program_name << " [OPTIONS]";
    for (const auto& optref : _metaref.required_opts()) {
        if (optref.get().lng().empty())
            std::cout << std::format(" -{} <{}>", optref.get().shrt(),
                                     get_argname(optref.get()));
        else
            std::cout << std::format(" --{} <{}>", optref.get().lng(),
                                     get_argname(optref.get()));
    }
    std::cout << std::endl;
}

void print_helper::print_options() const {
    std::cout << "Options:" << std::endl;

    size_t fixed_width = 0;
    for (const auto& optref : _metaref.opts()) {
        size_t width = 8;
        if (!optref.lng().empty()) width += 2 + optref.lng().size();
        if (optref.need_argument()) width += 3 + get_argname(optref).size();
        fixed_width = std::max(fixed_width, width);
    }

    for (const auto& optref : _metaref.opts()) {
        std::string item = "  ";
        if (optref.shrt() == '\0')
            item += "   ";
        else if (optref.lng().empty())
            item += std::format("-{}", optref.shrt());
        else
            item += std::format("-{},", optref.shrt());

        if (!optref.lng().empty()) item += std::format(" --{}", optref.lng());

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

[[noreturn]] void print_helper::internal_error(const std::string& msg) {
    std::cerr << "[internal error]: " << msg << std::endl;
    std::exit(1);
}

[[noreturn]] void print_helper::unexpected_argument(
    const std::string& arg) const {
    std::cerr << std::format("error: unexpected argument '{}' found", arg)
              << std::endl;
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl;
    std::cout << "For more information, try '--help'." << std::endl;
    std::exit(2);
}

[[noreturn]] void print_helper::missing_value(const std::string& flag,
                                              const anyopt& optref) const {
    std::cerr
        << std::format(
               "error: a value is required for '{} <{}>' but none was supplied",
               flag, get_argname(optref))
        << std::endl;
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl;
    std::cout << "For more information, try '--help'." << std::endl;
    std::exit(2);
}

[[noreturn]] void print_helper::unexpected_value(
    const std::string& flag, const std::string& value) const {
    std::cerr << std::format(
                     "error: unexpected value '{}' for '{}' found; no more "
                     "were expected",
                     value, flag)
              << std::endl;
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl;
    std::cout << "For more information, try '--help'." << std::endl;
    std::exit(2);
}

[[noreturn]] void print_helper::invalid_value(const std::string& flag,
                                              const std::string& value,
                                              const anyopt& optref,
                                              std::errc ec) const {
    std::cerr << std::format("error: invalid value '{}' for '{} <{}>': {}",
                             value, flag, get_argname(optref),
                             std::make_error_code(ec).message())
              << std::endl;
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl;
    std::cout << "For more information, try '--help'." << std::endl;
    std::exit(2);
}

[[noreturn]] void print_helper::used_mutiple(const std::string& flag,
                                             const anyopt& optref) const {
    if (optref.need_argument())
        std::cerr << std::format(
                         "error: the argument '{} <{}>' cannot be used "
                         "multiple times",
                         flag, get_argname(optref))
                  << std::endl;
    else
        std::cerr
            << std::format(
                   "error: the argument '{}' cannot be used multiple times",
                   flag)
            << std::endl;
    std::cout << std::endl;
    print_usage();
    std::cout << std::endl;
    std::cout << "For more information, try '--help'." << std::endl;
    std::exit(2);
}

[[noreturn]] void print_helper::missing_options(
    const std::vector<std::reference_wrapper<_detail::anyopt>>& opts) const {
    std::cerr << "error: the following required arguments were not provided:"
              << std::endl;
    for (const auto& optref : opts)
        if (optref.get().lng().empty())
            std::cout << std::format("  -{} <{}>", optref.get().shrt(),
                                     get_argname(optref.get()))
                      << std::endl;
        else
            std::cout << std::format("  --{} <{}>", optref.get().lng(),
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
_detail::opt_wrapper<OptT> opt(OptT& optref) {
    return {optref};
};

counter::counter() : _counter{0} {}

counter::counter(const counter& other) : _counter{other._counter} {}

counter::counter(counter&& other) {
    _counter = other._counter;
    other._counter = 0;
}

counter& counter::operator=(const counter& other) {
    _counter = other._counter;
    return *this;
}

counter& counter::operator=(counter&& other) {
    _counter = other._counter;
    other._counter = 0;
    return *this;
};

counter& counter::operator++() {
    ++_counter;
    return *this;
}

counter counter::operator++(int) {
    counter prev = *this;
    ++_counter;
    return prev;
}

counter::operator size_t() { return _counter; }

template <_detail::integer OptT>
auto string_converter<OptT>::from_str(const char* str)
    -> std::expected<OptT, std::errc> {
    OptT v{};
    const char* end = _detail::strend(str);
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

template <_detail::integer OptT>
std::string string_converter<OptT>::to_str(const OptT& value) {
    return std::to_string(value);
}

template <_detail::float_pointer OptT>
auto string_converter<OptT>::from_str(const char* str)
    -> std::expected<OptT, std::errc> {
    OptT v{};
    const char* end = _detail::strend(str);
    auto [ptr, ec] = std::from_chars(str, end, v);
    if (ec == std::errc{})
        if (ptr == end)
            return v;
        else
            return std::unexpected(std::errc::invalid_argument);
    else
        return std::unexpected(ec);
}

template <_detail::float_pointer OptT>
std::string string_converter<OptT>::to_str(const OptT& value) {
    return std::to_string(value);
}

auto string_converter<char>::from_str(const char* str)
    -> std::expected<char, std::errc> {
    if (std::strlen(str) != 1)
        return std::unexpected(std::errc::invalid_argument);
    return *str;
}

std::string string_converter<char>::to_str(const char& value) {
    return std::string{1, value};
}

auto string_converter<const char*>::from_str(const char* str)
    -> std::expected<const char*, std::errc> {
    return str;
}

std::string string_converter<const char*>::to_str(const char* const& value) {
    return value;
}

auto string_converter<std::string>::from_str(const char* str)
    -> std::expected<std::string, std::errc> {
    return std::string(str);
}

std::string string_converter<std::string>::to_str(const std::string& value) {
    return value;
}

template <typename... OptionTs>
meta::meta(OptionTs&&... options)
    : _help{false},
      _version{false},
      _opts{},
      _required_opts{},
      _sorted_by_flag{} {
    (void)(int[]){
        0,
        (_opts.emplace_back(_detail::anyopt(std::forward<OptionTs>(options))),
         0)...};

    _opts.emplace_back(
        _detail::anyopt(opt(_help).shrt('h').lng("help").about("Print help")));
    _opts.emplace_back(_detail::anyopt(
        opt(_version).shrt('V').lng("version").about("Print version")));

    _sorted_by_flag.reserve(sizeof...(OptionTs) * 2);

    for (auto& optref : _opts) {
        if (optref.shrt() == '\0' && optref.lng().empty())
            _detail::print_helper::internal_error(
                "there is an option that specifies neither short nor long "
                "flags.");

        bool inserted;
        if (optref.shrt() != '\0') {
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

auto meta::opts() -> std::vector<_detail::anyopt>& { return _opts; }

auto meta::required_opts()
    -> const std::vector<std::reference_wrapper<_detail::anyopt>>& {
    return _required_opts;
}

auto meta::query(const std::string& flag)
    -> std::optional<std::reference_wrapper<_detail::anyopt>> {
    if (_sorted_by_flag.contains(flag))
        return _sorted_by_flag.at(flag);
    else
        return std::nullopt;
};

inline bool meta::help() const { return _help; }

inline bool meta::version() const { return _version; }

template <args_group ArgsGroupT>
ArgsGroupT greet(int argc, char* argv[]) {
    ArgsGroupT args{};
    meta m = args.genmeta();
    _detail::print_helper printer{_detail::filename(argv[0]), m};

    _detail::remove_one_arg(argc, argv);

    while (argc) {
        size_t argtype = _detail::argtype(argv[0]);

        auto parse_helper = [&](const std::string& flag, bool newarg) -> bool {
            auto result = m.query(flag);
            if (!result) printer.unexpected_argument(flag);
            auto& optref = result.value();

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
                char* split_pos = std::strchr(argv[0], '=');
                if (split_pos) {
                    std::string flag{argv[0], split_pos};
                    argv[0] = split_pos;
                    parse_helper(flag, false);
                } else {
                    std::string flag{argv[0]};
                    _detail::remove_one_arg(argc, argv);
                    parse_helper(flag, true);
                }
            } break;
            case _detail::ARGUMENT:
                printer.unexpected_argument(argv[0]);
                break;
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
    for (const auto& optref : m.required_opts())
        if (!optref.get().already_set()) missing_opts.emplace_back(optref);
    if (missing_opts.size()) printer.missing_options(missing_opts);

    return args;
};

}  // namespace greet