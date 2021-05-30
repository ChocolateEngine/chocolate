#include "../../inc/core/engine.h"

extern "C"
{
	void engine_start
		(  )
	{
		engine_c engine;

		engine.engine_main(  );

		return;
	}
}
