#include "core/profiler.h"


void profile_end_frame()
{
#ifdef TRACY_ENABLE
	FrameMark;
#endif
}

