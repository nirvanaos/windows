#ifndef NIRVANA_CORE_PORT_BACKOFF_H_
#define NIRVANA_CORE_PORT_BACKOFF_H_

extern "C" __declspec (dllimport)
void __stdcall Sleep (unsigned long dwMilliseconds);

namespace Nirvana {
namespace Core {
namespace Port {

class BackOff
{
protected:
	static void sleep (unsigned hint)
	{
		::Sleep (hint);
	}
};

}
}
}

#endif
