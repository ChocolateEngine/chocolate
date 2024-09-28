#include "core/asserts.h"

#if USE_ASSERTS

static bool gNoAsserts = false;
static bool gNoAssertBox = false;


CONVAR_BOOL_CMD( assert_show, !gNoAsserts, 0, "Enable/Disable Assertions" )
{
    gNoAsserts = !assert_show;
}

CONVAR_BOOL_CMD( assert_show_box, !gNoAssertBox, 0, "Enable/Disable the Assertions Window" )
{
    gNoAssertBox = !assert_show_box;
}


void Assert_Init()
{
	gNoAsserts   = Args_Register( false, "Disable Asserts", "-no-asserts" );
	gNoAssertBox = Args_Register( false, "Disable The Assert Window", "-no-assert-window" );

    if ( gNoAsserts )
		Con_SetConVarValue( "assert_show", false );

    if ( gNoAssertBox )
		Con_SetConVarValue( "assert_show_box", false );
}


struct AssertDisabled
{
    const char* file;
    u32         line;
};

std::vector< AssertDisabled > gDisabledAsserts;


CONCMD( assert_clear_disabled )
{
    gDisabledAsserts.clear();
}


constexpr int BTN_DEBUG_BREAK = 0;
constexpr int BTN_IGNORE = 1;
constexpr int BTN_IGNORE_ALWAYS = 2;
constexpr int BTN_IGNORE_ALL = 3;


constexpr SDL_MessageBoxButtonData buttons[] = {
    /* .flags, .buttonid, .text */
    { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Debug Break" },
    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Ignore" },
    {                                       0, 2, "Always Ignore" },
    {                                       0, 3, "Ignore All" },
};


int ShowAssert_SDL2( const char* file, u32 line, const char* cond, const char* title, const char* msg )
{
    std::string fullMsg;
    // vstring( fullMsg, "File: %s\nLine: %d\nCond: %s\n\n%s", file, line, cond, msg );
    vstring( fullMsg, "File:    %s\nLine:   %d\n\nCondition:\n%s\n\n%s", file, line, cond, msg );

    const SDL_MessageBoxData data = {
        .flags =        SDL_MESSAGEBOX_WARNING,
        .window =       NULL,
        .title =        title,
        .message =      fullMsg.c_str(),
        .numbuttons =   SDL_arraysize(buttons),
        .buttons =      buttons,
        .colorScheme =  0
    };

    int buttonId;

    static char errmsg[1024];
    std::memset( errmsg, 0, 1024 );

    if ( SDL_ShowMessageBox( &data, &buttonId ) < 0 )
    {
        Log_ErrorF( "Error Displaying Assert Message Box: %s\n", SDL_GetErrorMsg( errmsg, 1024 ) );
        gNoAssertBox = true;
        return -1;
    }

    return buttonId;
}


void ShowAssert( const char* file, u32 line, const char* cond, const char* title, const char* msg )
{
    if ( gNoAssertBox )
        return;

    // TODO: make a better looking win32 assert dialog
    int ret = ShowAssert_SDL2( file, line, cond, title, msg );

    switch ( ret )
    {
        case -1:
        case BTN_IGNORE:
        default:
            break;

        case BTN_DEBUG_BREAK:
            sys_wait_for_debugger();
            break;

        case BTN_IGNORE_ALL:
            gNoAsserts = true;
            break;

        case BTN_IGNORE_ALWAYS:
            gDisabledAsserts.emplace_back( file, line );
            break;
    }
}


void HandleAssert( const char* file, u32 line, const char* cond, const char* title, const char* msg )
{
	if ( gNoAsserts )
		return;

	// check if assert is disabled
    for ( AssertDisabled& _assert : gDisabledAsserts )
    {
        if ( line != _assert.line )
            continue;

        // Assert is disabled, ignore it
		if ( ch_str_equals( file, _assert.file ) )
            return;
    }

    // All clear to show this assert
    Log_ErrorF( "%s\n  File: %s: %d\n  Cond: %s\n  %s\n", title, file, line, cond, msg );
	ShowAssert( file, line, cond, title, msg );
}


void HandleAssert( const char* file, u32 line, const char* cond, const char* msg )
{
    if ( gNoAsserts )
        return;

    // check if assert is disabled
    for ( AssertDisabled& _assert : gDisabledAsserts )
    {
        if ( line != _assert.line )
            continue;

        // Assert is disabled, ignore it
		if ( ch_str_equals( file, _assert.file ) == 0 )
            return;
    }

    // All clear to show this assert
    Log_ErrorF( "Assertion Failed\n  File: %s: %d\n  Cond: %s\n  %s\n", file, line, cond, msg );
    ShowAssert( file, line, cond, "Assertion Failed", msg );
}


bool IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* title, const char* msg )
{
	if ( !sResult )
		return false;

    HandleAssert( file, line, cond, title, msg );
	return sResult;
}


bool IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* msg )
{
	if ( !sResult )
		return false;

	HandleAssert( file, line, cond, msg );
	return sResult;
}


#else

void Assert_Init()
{
}

void HandleAssert( const char* file, u32 line, const char* cond, const char* title, const char* msg )
{
}

void HandleAssert( const char* file, u32 line, const char* cond, const char* msg )
{
}

bool IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* title, const char* msg )
{
	if ( sResult )
		Log_Error( msg );

	return sResult;
}

bool IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* msg )
{
	if ( sResult )
		Log_Error( msg );

	return sResult;
}

#endif
