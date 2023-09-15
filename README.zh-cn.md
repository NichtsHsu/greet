# Greet

[English](README.md) | **简体中文**

一个 C++ 的命令行参数解析器，想要通过 C++23 的特性提供更加现代化的接口。

受到 [clap](https://github.com/clap-rs/clap) 和 [glaze](https://github.com/stephenberry/glaze) 的启发，行为上很大程度地参考了 [clap](https://github.com/clap-rs/clap)。

名字源于 [clap 的文档](https://docs.rs/clap/latest/clap/#example):

> -n, --name <NAME>    Name of the person to **greet**
>
> -c, --count <COUNT>  Number of times to **greet** [default: 1]
>
> -h, --help           Print help
>
> -V, --version        Print version

## 坏消息

* Greet 使用 C++23 特性, 所以你需要最低 `gcc-13` 或者 `clang-17` 来编译 😈
* 我从来没有考虑过任何性能优化 😱
* 你的 IDE/LSP 可能会报很多错，即使代码可以正常编译 😭

## 好消息

* Greet 只有一个头文件, 你只需要下载 [`greet.hpp`](https://github.com/NichtsHsu/greet/blob/master/greet.hpp) 到你的项目中并且 `#include` 它 😙
* Greet 不依赖 `std` 以外的任何第三方库 😍
* 更现代化的接口可能会使你心情愉悦 😆

## 构建[示例](https://github.com/NichtsHsu/greet/blob/master/example.cpp)

确保你已安装 `gcc-13`。即使你打算使用 `clang-17`，你仍然需要安装 `libstdc++-13-dev`。

编译命令异常简单：

```bash
g++ example.cpp -std=c++23 -o example # or
clang example.cpp -std=c++2b -stdlib=libc++ -o example
```

## Greet 规则

* 对于需要参数的选项而言, `-a xxx`, `-axxx`, `-a=xxx`, `--aaa xxx` 和 `--aaa=xxx` 都是可接受的。
* 对于不需要参数的选项而言, `-e -f -g`, `--eee --fff --ggg` 和 `-efg` 都是可接受的。
* 当混合二者时，例如 `-faxxxg` 会被解析为 `-f -a xxxg` 而不是 `-f -a xxx -g`。
* 如果 `-a` 不需要参数，`-a-b` 会被解析为 `-a -- -b` 然后因为 `--` 报错。但是如果 `-a` 需要一个参数，`-a-b` 则会被解析为 `-a` 选项的值为 `-b`。
* 如果一个选项的值以连字符（`-`）开头，`-a-b`, `-a=-b` 和 `--aaa=-b` 都是可接受的。但是 `-a -b` 和 `--aaa -b` 默认不可接受，会被解析为两个选项。

## 使用指南

### 1. 准备

下载 [`greet.hpp`](https://github.com/NichtsHsu/greet/blob/master/greet.hpp) 到你的项目中并 `#include` 它：

```cpp
#include <greet.hpp>
```

### 2. 声明一个参数组类型

你需要声明一个满足以下条件的参数组类型：继承 `greet::information`，[可默认初始化](https://zh.cppreference.com/w/cpp/concepts/default_initializable)并且[可移动](https://zh.cppreference.com/w/cpp/concepts/movable)。基本上，你可以不声明任何构造函数然后让编译器帮你处理。

```cpp
struct Args: public greet::information {
    // 不声明任何构造函数

    // 程序版本号，会在 '-V' 或 '--version' 时打印
    std::string version() override { return "greet v0.1.0"; }

    // 程序简介, 会在 '-h' 或 '--help' 时打印
    std::string description() override { return "greet with a person"; }

    // 我们马上就会知道这是什么
    greet::meta genmeta() override { /* TODO */}
}
```

### 3. 添加选项

在 `Args` 中添加你想提供给用户的选项:

```cpp
struct Args: public greet::information {
    std::string name;
    size_t age;
    bool greeted;
    greet::counter times;
    std::vector<std::string> places;

    // 程序版本号，会在 '-V' 或 '--version' 时打印
    std::string version() override { return "greet v0.1.0"; }

    // 程序简介, 会在 '-h' 或 '--help' 时打印
    std::string description() override { return "greet with a person"; }

    // 我们马上就会知道这是什么
    greet::meta genmeta() override { /* TODO */}
}
```

一共有四种选项类型：

#### 3.1 NORMAL 类型

NORMAL 类型的选项包含整型类型，浮点类型，`char`, `const char *` 以及 `std::string`。如果你想要自定义一个类型，请参考[附加：构建你自己的 NORMAL 类型选项](#附加构建你自己的-normal-类型选项)。

NORMAL 类型的选项需要一个参数，这意味着，例如：`-a xx`。

NORMAL 类型的选项不能多次指定。

#### 3.2 BOOL 类型

BOOL 类型的选项必须是 `bool`。

BOOL 类型的选项不需要参数，当用户指定该选项时为 `true`，否则为 `false`。

BOOL 类型的选项不能多次指定。

#### 3.3 COUNTER 类型

COUNTER 类型的选项必须是 `greet::counter`。

COUNTER 类型的选项不需要参数，并且可以被多次指定 —— 换句话说它对用户指定该选项的次数进行计数。例如，`-a -a -a -a` 将导致 `counter` 求值为 `4`。

注意：`greet::counter` 类型的值可以隐式转换到 `size_t` 类型，你可以把它当做 `size_t` 用在任何地方，例如比较或者打印。

#### 3.4 VECTOR 类型

VECTOR 类型的选项是当模板参数为 NORMAL 类型时的 `std::vector`。

VECTOR 类型的选项需要一个值，并且可以被多次指定。例如，`-a 1 -a 2 -a 3` 将会得到一个包含 `1`, `2` 和 `3` 的数组。

### 4. 完善元数据

为你的每一个选项写下元信息：

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

#### 4.1 NORMAL 类型可用的元信息

```cpp
greet::opt(aaa)     // 绑定到 `aaa` 选项
    .shrt('a')      // 短标志：'-a'
    .lng("aaa")     // 长标志：'--aaa'
                    // 至少要提供二者之一
    .required()     // 用户必须提供该选项
    .def(0)         // 默认值，仅当未设置 `required` 时有效
    .argname("aha") // 打印帮助时显示的参数名，
                    // 默认是大写的长标志。
                    // 如果没有长标志则是 `VALUE`
    .allow_hyphen() // 允许值以连字符（`-`）开头，
                    // 这只影响 `-a -b` 和 `--aaa -b`
    .about("a NORMAL type option")  // 选项相关信息
```

#### 4.2 BOOL 类型可用的元信息

```cpp
greet::opt(aaa)     // 绑定到 `aaa` 选项
    .shrt('a')      // 短标志：'-a'
    .lng("aaa")     /// 长标志：'--aaa'
                    // 至少要提供二者之一
    .about("a BOOL type option")   // 选项相关信息
```

#### 4.3 COUNTER 类型可用的元信息

```cpp
greet::opt(aaa)     // 绑定到 `aaa` 选项
    .shrt('a')      // 短标志：'-a'
    .lng("aaa")     /// 长标志：'--aaa'
                    // 至少要提供二者之一
    .about("a COUNTER type option") // 选项相关信息
```

#### 4.4 VECTOR 类型可用的元信息

```cpp
greet::opt(aaa)     // 绑定到 `aaa` 选项
    .shrt('a')      // 短标志：'-a'
    .lng("aaa")     // 长标志：'--aaa'
                    // 至少要提供二者之一
    .argname("aha") // 打印帮助时显示的参数名，
                    // 默认是大写的长标志。
                    // 如果没有长标志则是 `VALUE`
    .allow_hyphen() // 允许值以连字符（`-`）开头，
                    // 这只影响 `-a -b` 和 `--aaa -b`
    .about("a VECTOR type option")   // 选项相关信息
```

*注意：`-h`, `--help`, `-V` 以及 `--version` 被预留为打印帮助和版本号。*

### 5. 获得参数

以你的参数组类型作为模板参数调用 `greet::greet()`：

```cpp
int main(int argc, char* argv[]) {
    Args args = greet::greet<Args>(argc, argv);
}
```

### 6. 随便做任何事情

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

### 附加：构建你自己的 NORMAL 类型选项

一个 NORMAL 类型的选项必须是[半正则](https://zh.cppreference.com/w/cpp/concepts/semiregular)并且与字符串可转换。

若要半正则，这基本上意味着：

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

若要与字符串可转换，用你的类型模板特化 `greet::string_converter`：

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

之后，你可以将它当做一个 NORMAL 类型使用。
