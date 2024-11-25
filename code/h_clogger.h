#pragma once
#include <iostream>


#define LINFO(stream)    std::cout << "\033[1;44m"  <<"[INFO ]: "<< stream << "\033[0m" << std::endl;
#define LTRACE(stream)   std::cout << "\033[1;100m" <<"[TRACE]: "<< stream << "\033[0m" << std::endl;
#define LTIME(stream)    std::cout << "\033[1;42m"  <<"[TIME ]: "<< stream << "\033[0m" << std::endl;
#define LERR(stream)     std::cout << "\033[1;41m"  <<"[ERROR]:" <<\
																__FILE__<<" "<<__func__<<" "<<__LINE__<<": "<<\
																stream << "\033[0m" << std::endl;

#define LWARN(stream)    std::cout << "\033[1;45m"  <<"[WARN ]: "<< stream << "\033[0m" << std::endl;



