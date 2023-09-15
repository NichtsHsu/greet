# Greet

**English** | [简体中文](README.zh-cn.md)

A C++ Command Line Argument Parser, which want to provide more modern interfaces with C++23 features.

Inspired by [clap](https://github.com/clap-rs/clap) and [glaze](https://github.com/stephenberry/glaze), and heavily referenced [clap](https://github.com/clap-rs/clap) for behaviors.

Get the name from [clap's documentation](https://docs.rs/clap/latest/clap/#example):

> -n, --name <NAME>    Name of the person to **greet**
>
> -c, --count <COUNT>  Number of times to **greet** [default: 1]
>
> -h, --help           Print help
>
> -V, --version        Print version

## Bad news

* Greet uses C++23 features, so you need at least `gcc-13` or `clang-17` to compile it 😈
* I bet I haven't considered any performance optimizations 😱
* Your IDE/LSP may throw a lot of errors even though the code was compiled successfully 😭

## Good news

* Greet has only one header file, You just need to download the [`greet.hpp`](https://github.com/NichtsHsu/greet/blob/master/greet.hpp) file to your project and `#include` it 😙
* Greet does not rely on any third-party libraries other than the `std` 😍
* More modern interfaces might cheer you up 😆

## Build the [example](https://github.com/NichtsHsu/greet/blob/master/example.cpp)

Make sure you have `gcc-13` installed. Even if you plan to use `clang-17`, you still need to install `libstdc++-13-dev`.

The build command is simple:

```bash
g++ example.cpp -std=c++23 -o example # or
clang example.cpp -std=c++2b -stdlib=libc++ -o example
```

## Rule of greet

* For options that need an argument, `-a xxx`, `-axxx`, `-a=xxx`, `--aaa xxx` and `--aaa=xxx` are acceptable.
* For options that don't need an argument, `-e -f -g`, `--eee --fff --ggg` and `-efg` are acceptable.
* When you mix them, such as `-faxxxg`, it will be parsed as `-f -a xxxg` but not `-f -a xxx -g`.
* If `-a` doesn't need an argument, `-a-b` will be parsed as `-a -- -b` then an error will be reported because of `--`. But if `-a` need an argument, `-a-b` will be parsed to option `-a` with its value `-b`.
* When the value of a option is start with a hyphen(`-`), `-a-b`, `-a=-b` and `--aaa=-b` are acceptable. However, `-a -b` and `--aaa -b` is not acceptable by default, and will be parsed to two options.

## Guide to use

### 1. Prepare

Download the [`greet.hpp`](https://github.com/NichtsHsu/greet/blob/master/greet.hpp) file to your project and `#include` it:

```cpp
#include <greet.hpp>
```

### 2. Declare an argument group class

You should declare an argument group class type, which should be: derived from `greet::information`, [default initializable](https://en.cppreference.com/w/cpp/concepts/default_initializable) and [movable](https://en.cppreference.com/w/cpp/concepts/movable). Basically, you can declare no constructor and let the compiler handle it for you.

```cpp
struct Args: public greet::information {
    // don't declare any constructors

    // version of program, will be printed at '-V' or '--version'
    std::string version() override { return "greet v0.1.0"; }

    // description of program, will be printed at '-h' or '--help'
    std::string description() override { return "greet with a person"; }

    // we will learn about it soon
    greet::meta genmeta() override { /* TODO */}
}
```

### 3. Add options

Add any options you want to provide to the user in the `Args`:

```cpp
struct Args: public greet::information {
    std::string name;
    size_t age;
    bool greeted;
    greet::counter times;
    std::vector<std::string> places;

    // version of program, will be printed at '-V' or '--version'
    std::string version() override { return "greet v0.1.0"; }

    // description of program, will be printed at '-h' or '--help'
    std::string description() override { return "greet with a person"; }

    // we will learn about it soon
    greet::meta genmeta() override { /* TODO */}
}
```

There are four different option types available:

#### 3.1 NORMAL type

The NORMAL type options include the integer types, the float point types, `char`, `const char *` and `std::string`. If you want add your custom type, please refer to [EXT: build your own NORMAL type option](#ext-build-your-own-normal-type-option).

The NORMAL type options need an argument, that means, for an example, `-a xx`.

The NORMAL type option cannot be used multuple times.

#### 3.2 BOOL type

The BOOL type option can only be `bool`.

The BOOL type option doesn't need an argument, `true` if the user provides this option, `false` otherwise.

The BOOL type option cannot be used multuple times.

#### 3.3 COUNTER type

The COUNTER type option can only be `greet::counter`.

The COUNTER option doesn't need an argument, and can be used multiple times -- it's counting how many times the user has used this option. For an example, `-a -a -a -a` makes the `counter` to evaluate to `4`.

NOTE: value of `greet::counter` can be implicitly converted to `size_t`. You can use it as `size_t` anywhere, for examples, to compare or print.

#### 3.4 VECTOR type

The VECTOR type options are `std::vector`s when the template parameter is NORMAL type.

The VECTOR type options need an argument, and can be used multiple times. For an example, `-a 1 -a 2 -a 3` will get a vector of `1`, `2` and `3`.

### 4. Complete meta

Write meta informations for each of your options:

```cpp
struct Args: public greet::information {
    std::string name;
    size_t age;
    bool greeted;
    greet::counter times;
    std::vector<std::string> places;

    ...

    greet::meta genmeta() override {
        return {
            greet::opt(name)
                .shrt('n')
                .lng("name")
                .required()
                .about("name of the person to greet"),
            greet::opt(age)
                .lng("age")
                .def(18u)
                .about("age of the person to greet"),
            greet::opt(greeted)
                .shrt('g')
                .about("have greeted before"),
            greet::opt(times)
                .shrt('t')
                .about("how many times you want to greet"),
            greet::opt(places)
                .shrt('p')
                .lng("place")
                .allow_hyphen()
                .about("where to greet"),
        };
    }
}
```

#### 4.1 available meta informations of NORMAL types

```cpp
greet::opt(aaa)     // bind to the `aaa` option
    .shrt('a')      // the short flag: '-a'
    .lng("aaa")     // the long flag: '--aaa'
                    // You should provide at least one of the two
    .required()     // user must provide this option
    .def(0)         // default value, only valid when didn't set `required`
    .argname("aha") // the argument name when print help,
                    // default to the uppercase long flag.
                    // if no long flag, default to 'VALUE'
    .allow_hyphen() // allow value start with a hyphen(`-`),
                    // this only affects `-a -b` and `--aaa -b`
    .about("a NORMAL type option")  // about message
```

#### 4.2 available meta informations of BOOL types

```cpp
greet::opt(aaa)     // bind to the `aaa` option
    .shrt('a')      // the short flag: '-a'
    .lng("aaa")     // the long flag: '--aaa'
                    // You should provide at least one of the two
    .about("a BOOL type option")   // about message
```

#### 4.3 available meta informations of COUNTER types

```cpp
greet::opt(aaa)     // bind to the `aaa` option
    .shrt('a')      // the short flag: '-a'
    .lng("aaa")     // the long flag: '--aaa'
                    // You should provide at least one of the two
    .about("a COUNTER type option") // about message
```

#### 4.4 available meta informations of VECTOR types

```cpp
greet::opt(aaa)     // bind to the `aaa` option
    .shrt('a')      // the short flag: '-a'
    .lng("aaa")     // the long flag: '--aaa'
                    // You should provide at least one of the two
    .argname("aha") // the argument name when print help,
                    // default to the uppercase long flag.
                    // if no long flag, default to 'VALUE'
    .allow_hyphen() // allow value start with a hyphen(`-`),
                    // this only affects `-a -b` and `--aaa -b`
    .about("a VECTOR type option")  // about message
```

*NOTE: `-h`, `--help`, `-V` and `--version` are reserved for print help and version.*

### 5. Get arguments

Call `greet::greet()` with your argument group class as the template argument:

```cpp
int main(int argc, char* argv[]) {
    Args args = greet::greet<Args>(argc, argv);
}
```

### 6. Just do anything with it

```cpp
int main(int argc, char* argv[]) {
    Args args = greet::greet<Args>(argc, argv);

    std::cout << "I will greet " << args.name << std::endl;
    std::cout << "He/She is " << args.age << " years old" << std::endl;
    if (args.greeted)
        std::cout << "We have greeted before" << std::endl;
    else
        std::cout << "We never greeted before" << std::endl;
    std::cout << "We should greet " << args.times << " times" << std::endl;
    std::cout << "We may greet at " << args.places.size()
              << " places:" << std::endl;
    for (const auto& place : args.places) {
        std::cout << "\t" << place << std::endl;
    }
}
```

### 7. Try print help

```bash
$ example -h
greet with a person

Usage: example [OPTIONS] --name <NAME>

Options:
  -n, --name <NAME>    name of the person to greet [REQUIRED]
      --age <AGE>      age of the person to greet [default: 18]
  -g                   have greeted before
  -t                   how many times you want to greet
  -p, --place <PLACE>  where to greet
  -h, --help           Print help
  -V, --version        Print version
```

### EXT: build your own NORMAL type option

A NORMAL type option should be [semiregular](https://en.cppreference.com/w/cpp/concepts/semiregular) and string convertable.

To be semiregular, it basically means:

```cpp
class myoption {
   public:
    myoption();
    myoption(const myoption &);
    myoption(myoption &&);
    myoption &operator=(const myoption &);
    myoption &operator=(myoption &&);
};
```

To be string convertable, specialize the `greet::string_converter` with your type:

```cpp
template <>
struct greet::string_converter<myoption> {
    static auto from_str(const char *str)
        -> std::expected<myoption, std::errc> {
        /* TODO */
    }
    static std::string to_str(const myoption &value) {
        /* TODO */
    }
};
```

After that, you can use it as a NORMAL option type.
