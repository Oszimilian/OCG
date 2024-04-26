#include <iostream>
#include "CanGen.h"

int main(int argc, char **argv) {
    std::cout << "Halolo" << std::endl;

    CanGen canGen;
    canGen.importBaseConfig("../config/baseConfig.yaml");
    canGen.init();

    return 0;
}