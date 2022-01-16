#include "engine.h"
#include "core/platform.h"

Engine *engine = new Engine;

extern "C"
{
	void* DLL_EXPORT cengine_get(  )
	{
		return engine;
	}
}
