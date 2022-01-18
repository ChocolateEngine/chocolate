#include "engine.h"
#include "core/platform.h"

Engine *engine = new Engine;

extern "C"
{
	DLL_EXPORT void* cengine_get(  )
	{
		return engine;
	}
}
