#pragma once
#include <stdio.h>

#if NDEBUG
#define IDPF
#else
#define IDPF( ... ) { fprintf( stderr, "%s: ", __PRETTY_FUNCTION__ );	\
		fprintf( stderr, __VA_ARGS__ ); }
#endif /* NDEBUG  */
