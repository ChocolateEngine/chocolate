#pragma once
#include <stdio.h>

#if NDEBUG
#define IDPF
#elif _WIN32
#define IDPF( ... ) { fprintf( stderr, "%s: ", __FUNCSIG__ );		\
		fprintf( stderr, __VA_ARGS__ ); }
#elif __linux__
#define IDPF( ... ) { fprintf( stderr, "%s: ", __PRETTY_FUNCTION__ );	\
		fprintf( stderr, __VA_ARGS__ ); }
#endif /* NDEBUG  */
