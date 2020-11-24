#ifndef NIRVANA_CORE_WINDOWS_INITIALIZE_H_
#define NIRVANA_CORE_WINDOWS_INITIALIZE_H_

#include <Heap.h>
#include "Thread.inl"
#include "Console.h"
#include <exception>

namespace Nirvana {
namespace Core {
namespace Windows {

inline
bool initialize (void)
{
  try {
    Heap::initialize ();
    Port::Thread::initialize ();
  } catch (const std::exception& ex) {
    Console::write (ex.what ());
    Console::write ("\n");
    return false;
  }
  return true;
}

}
}
}

#endif
