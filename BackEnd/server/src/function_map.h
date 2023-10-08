#include <unordered_map>
#include <cstdlib>
#include <functional>
#include <iostream>

#include "format.h"

namespace Capstone {
    /**
    * @brief Match the function ID given in the JSON to one of the matrix functions
    * 
    * @param funcID 
    */
   template <typename T>
   T mapIdToFunction(FunctionData& data) {
       switch (data.mFuncId) {
            case 0x10: std::cout << "Adding Matrices Together\n"; return {{2, 4}, {6, 8}};
            case 0x11: std::cout << "";
            case 0x12: std::cout << "Transposing a matrix\n"; break;
            case 0x13: std::cout << "";
            case 0x14: std::cout << "";
            case 0x15: std::cout << "";
            default: return {{}};
       }

       return {{}};
    }   
}   

