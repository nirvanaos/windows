#include <iostream>

namespace TestAPI {

// This function is made extern outline to avoid it's optimization
// by the too smart compilers.
// For MSVC "Whole Program Optimization" option (/GL) must be turned off.
extern void test_exception (int* p)
{
	std::cout << *p;
}

}
