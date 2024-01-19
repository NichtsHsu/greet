#include <iostream>
#include <string>
#include <bit>

#include "new-greet.hpp"

struct MyStruct {
    int alacaca;
    double blabla;
    std::string wtf;
};

int main() {
    std::cout << greet::reflect::name_of<MyStruct, 0>() << std::endl;
    std::cout << greet::reflect::name_of<MyStruct, 1>() << std::endl;
    std::cout << greet::reflect::name_of<MyStruct, 2>() << std::endl;

    return 0;
}