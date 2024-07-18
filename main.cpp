#include <iostream>
#include "CanGen.h"
#include <string>

int main(int argc, char **argv) {
    std::cout << "CanGen v0.1" << std::endl;

    CanGen canGen;
    if(argc > 1) {
        std::cout << "Input: " << argv[1] << "\n";
        if(argv[1] == std::string("pi")) {
            canGen.importBaseConfig("../config/baseConfigPi.yaml");
        } else if(argv[1] == std::string("pc")) {
            canGen.importBaseConfig("../config/baseConfigPc.yaml");
        } else {
	    std::cout << "No valid Input" << "\n";
	    return 0;
        }
        canGen.init();
    } else {
        std::cout << "no input provided! \n";
        std::cout << "Decide between: pi or pc \n";
    }
    
    return 0;
}
