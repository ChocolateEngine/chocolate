#include "../../inc/core/engine.h"
#include "../../inc/shared/platform.h"

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
