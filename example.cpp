#include "greet.hpp"

struct Args : public greet::information {
    std::string name;
    size_t age;
    bool greeted;
    greet::counter times;
    std::vector<std::string> places;

    std::string version() override { return "greet v0.1.0"; }
    std::string description() override { return "greet with a person"; }
    greet::meta genmeta() override {
        return {
            greet::opt(name)
                .shrt('n')
                .lng("name")
                .required()
                .about("Name of the person to greet"),
            greet::opt(age)
                .lng("age")
                .def(18u)
                .about("Age of the person to greet"),
            greet::opt(greeted)
                .shrt('g')
                .about("Have greeted before"),
            greet::opt(times)
                .shrt('t')
                .about("How many times you want to greet"),
            greet::opt(places)
                .shrt('p')
                .lng("place")
                .allow_hyphen()
                .about("Where to greet"),
        };
    }
};

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