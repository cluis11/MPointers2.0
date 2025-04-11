#include <iostream>
#include "MPointer.h"  // Asegurate de incluir tu clase MPointer

int main() {
    try {
        // MPointer de tipo int
        MPointer<int> ptrInt = MPointer<int>::New();
        *ptrInt = 42;
        std::cout << "INT: " << *ptrInt << std::endl;

        // MPointer de tipo float
        MPointer<float> ptrFloat = MPointer<float>::New();
        *ptrFloat = 3.14f;
        std::cout << "FLOAT: " << *ptrFloat << std::endl;

        // MPointer de tipo char
        MPointer<char> ptrChar = MPointer<char>::New();
        *ptrChar = 'A';
        std::cout << "CHAR: " << *ptrChar << std::endl;

        // MPointer de tipo string
        MPointer<std::string> ptrString = MPointer<std::string>::New();
        *ptrString = "Hola Mundo";
        std::cout << "STRING: " << std::string(*ptrString) << std::endl;


    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
