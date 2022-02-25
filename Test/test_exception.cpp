#include <iostream>

namespace TestAPI {

// This function is made extern outline to avoid it's optimization
// of the too smart compilers like CLang.
extern void test_exception (int* p)
{
	std::cout << *p;
}

}
