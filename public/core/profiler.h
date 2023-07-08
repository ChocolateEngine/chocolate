#pragma once

// Macros for tracy because the constexpr ones dont want to work

#ifndef TRACY_ENABLE

#define PROF_SCOPE_BASE( varname, active )
#define PROF_SCOPE_NAMED_BASE( varname, name, active )

#define PROF_SCOPE_NAMED( name )
#define PROF_SCOPE()

#define CH_PROF_ZONE_TEXT( name, len )
#define CH_PROF_ZONE_NAME( name, len )

#define TracyAlloc( ptr, size )
#define TracyFree( ptr )
#define TracyMessageC( txt, size, color )

#else

#include <Tracy.hpp>

#define PROF_SCOPE_BASE( varname, active ) static tracy::SourceLocationData TracyConcat(__tracy_source_location,__LINE__) { nullptr, __FUNCTION__,  __FILE__, (uint32_t)__LINE__, 0 }; tracy::ScopedZone varname( &TracyConcat(__tracy_source_location,__LINE__), active );
#define PROF_SCOPE_NAMED_BASE( varname, name, active ) static tracy::SourceLocationData TracyConcat(__tracy_source_location,__LINE__) { name, __FUNCTION__,  __FILE__, (uint32_t)__LINE__, 0 }; tracy::ScopedZone varname( &TracyConcat(__tracy_source_location,__LINE__), active );

#define PROF_SCOPE_NAMED( name ) PROF_SCOPE_NAMED_BASE( __tracy_scope, name, true )
#define PROF_SCOPE() PROF_SCOPE_BASE( __tracy_scope, true )

#define CH_PROF_ZONE_TEXT( name, len ) __tracy_scope.Text( name, len )
#define CH_PROF_ZONE_NAME( name, len ) __tracy_scope.Name( name, len )

#endif

void CORE_API profile_end_frame();

