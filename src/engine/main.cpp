#include "engine.h"
#include "core/platform.h"

extern "C"
{
	void DLL_EXPORT engine_start
		(  )
	{
	        Engine 	engine;

		engine.EngineMain(  );

		return;
	}
}
