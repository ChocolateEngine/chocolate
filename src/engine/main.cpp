#include "engine.h"
#include "core/platform.h"

extern "C"
{
	void DLL_EXPORT engine_start
		( const char* gamePath )
	{
	        Engine 	engine;

		engine.EngineMain( gamePath );

		return;
	}
}
