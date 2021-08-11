#include "../../inc/core/engine.h"
#include "../../inc/shared/platform.h"

extern "C"
{
	void DLL_EXPORT engine_start
		(  )
	{
		engine_c engine;

		engine.engine_main(  );

		return;
	}
}
