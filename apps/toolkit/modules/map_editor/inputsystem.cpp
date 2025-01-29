#include "inputsystem.h"
#include "iinput.h"
#include "main.h"
#include "game_shared.h"

extern IInputSystem* input;

LOG_CHANNEL_REGISTER( EditorInput, ELogColor_Default );


CONVAR_FLOAT( m_pitch, 0.022, CVARF_ARCHIVE, "Mouse Pitch" );
CONVAR_FLOAT( m_yaw, 0.022, CVARF_ARCHIVE, "Mouse Yaw" );
CONVAR_FLOAT( m_sensitivity, 1.0, CVARF_ARCHIVE, "Mouse Sensitivity" );


static glm::vec2                                    gMouseDelta{};
static glm::vec2                                    gMouseDeltaScale{ 1.f, 1.f };
static std::unordered_map< ButtonList_t, EBinding > gKeyBinds;
static KeyState                                     gBindingState[ EBinding_Count ];

static bool                                         gResetBindings = args_register( "Reset All Bindings", "--reset-binds" );


// TODO: buffered input
// https://www.youtube.com/watch?v=VQ0Amoqz4Lg


// bind "ctrl+c" "command ent_copy"
// bind "ctrl+c" "move_forward"


CONCMD_VA( in_dump_all_buttons, "Dump a List of All Button Names" )
{
	log_t group = Log_GroupBegin( gLC_EditorInput );

	Log_Group( group, "Buttons:\n\n" );
	for ( int i = 0; i < SDL_NUM_SCANCODES; i++ )
	{
		const char* name = input->GetKeyName( (EButton)i );

		if ( strlen( name ) == 0 )
			continue;

		Log_GroupF( group, "%d: %s\n", i, name );
	}

	Log_GroupEnd( group );
}


static const char* gInputBindingStr[] = {

	// General
	"General_Undo",
	"General_Redo",
	"General_Cut",
	"General_Copy",
	"General_Paste",

	// Viewport Bindings
	"Viewport_MouseLook",
	"Viewport_MoveForward",
	"Viewport_MoveBack",
	"Viewport_MoveLeft",
	"Viewport_MoveRight",
	"Viewport_MoveUp",
	"Viewport_MoveDown",
	"Viewport_Sprint",
	"Viewport_Slow",
	"Viewport_SelectSingle",
	"Viewport_SelectMulti",
	"Viewport_IncreaseMoveSpeed",
	"Viewport_DecreaseMoveSpeed",
	"Viewport_GizmoSnap",
	"Viewport_GizmoSnapIncrement",
	"Viewport_GizmoSnapIncrement",
};


static_assert( CH_ARR_SIZE( gInputBindingStr ) == EBinding_Count );


const char* Input_BindingToStr( EBinding sBinding )
{
	if ( sBinding < 0 || sBinding > EBinding_Count )
		return "INVALID";

	return gInputBindingStr[ sBinding ];
}


EBinding Input_StrToBinding( std::string_view sBindingName )
{
	std::string lower = str_lower2( sBindingName.data() );

	for ( u16 i = 0; i < EBinding_Count; i++ )
	{
		if ( lower == str_lower2( gInputBindingStr[ i ] ) )
			return (EBinding)i;
	}

	Log_ErrorF( gLC_EditorInput, "Unknown Binding: %s\n", sBindingName );
	return EBinding_Count;
}


//static void PrintBinding( EButton sScancode )
//{
//	// Find the command for this key and print it
//	auto it = gKeyBinds.find( sScancode );
//	if ( it == gKeyBinds.end() )
//	{
//		Log_MsgF( gLC_EditorInput, "Binding: \"%s\" :\n", input->GetKeyName( sScancode ) );
//	}
//	else
//	{
//		Log_MsgF( "Binding: \"%s\" - %s\n", input->GetKeyName( sScancode ), Input_BindingToStr( it->second ) );
//	}
//}


CONCMD( bind_reset_all )
{
	Input_ResetBindings();
}


static void bind_dropdown_keys(
  const std::vector< std::string >& args,  // arguments currently typed in by the user
  std::vector< std::string >&       results )    // results to populate the dropdown list with
{
	std::vector< std::string > buttonNames;

	size_t                     buttonNameIndex = 0;
	buttonNames.push_back( "" );

	if ( args.size() )
	{
		for ( size_t i = 0; i < args[ 0 ].size(); i++ )
		{
			if ( args[ 0 ][ i ] == '+' )
			{
				buttonNameIndex++;
				buttonNames.push_back( "" );
			}
			else
			{
				buttonNames[ buttonNameIndex ] += args[ 0 ][ i ];
			}
		}
	}

	// all button results before "+", used only if multiple buttons are present
	std::vector< std::string > firstButtonResults;

	if ( buttonNames.size() > 1 )
	{
		// leave the last button name for the 2nd search section
		for ( size_t btnI = 0; btnI < buttonNames.size() - 1; btnI++ )
		{
			std::string_view buttonName = buttonNames[ btnI ];

			for ( int i = 0; i < SDL_NUM_SCANCODES; i++ )
			{
				const char* name = input->GetKeyName( (EButton)i );

				if ( name == nullptr )
					continue;

				size_t nameLen = strlen( name );

				// Check if they are both equal
				if ( nameLen != buttonName.size() )
					continue;

#ifdef _WIN32
				// if ( _strnicmp( name, currentButton.data(), strlen( name ) ) != 0 )
				if ( _strnicmp( name, buttonName.data(), nameLen ) != 0 )
					continue;
#else
				if ( strncasecmp( name, buttonName.data(), nameLen ) != 0 )
					continue;
#endif
				firstButtonResults.push_back( name );
				break;
			}
		}
	}

	if ( firstButtonResults.size() != buttonNames.size() - 1 )
		return;

	// Match Last button name
	// all results after the last "+", used even if only one button is present
	std::vector< std::string > lastButtonResults;

	std::string_view lastButtonName = buttonNames[ buttonNames.size() - 1 ];
	for ( int i = 0; i < SDL_NUM_SCANCODES; i++ )
	{
		const char* name = input->GetKeyName( (EButton)i );

		if ( name == nullptr )
			continue;

		size_t nameLen = strlen( name );

		if ( nameLen == 0 )
			continue;

		if ( lastButtonName.size() )
		{
			// Check if this string is inside this other string
			const char* find = strcasestr( name, lastButtonName.data() );

			if ( !find )
				continue;

			lastButtonResults.push_back( name );
		}
		else
		{
			lastButtonResults.push_back( name );
		}
	}

	if ( lastButtonResults.empty() && firstButtonResults.empty() )
		return;

	// populate first button results
	std::string buttonPrefix;

	if ( firstButtonResults.size() )
	{
		buttonPrefix = firstButtonResults[ 0 ];

		for ( size_t i = 1; i < firstButtonResults.size(); i++ )
		{
			buttonPrefix += "+" + firstButtonResults[ i ];
		}
	}

	// are we also typing in a binding?
	if ( args.size() > 1 && lastButtonResults.size() )
	{
		// look if we have any exact matches for the lastButtonResults
		for ( size_t btnI = 0; btnI < lastButtonResults.size(); btnI++ )
		{
			std::string_view buttonName = lastButtonResults[ btnI ];

			bool foundMatch = false;
			for ( int i = 0; i < SDL_NUM_SCANCODES; i++ )
			{
				const char* name = input->GetKeyName( (EButton)i );

				if ( name == nullptr )
					continue;

				size_t nameLen = strlen( name );

				// Check if they are both equal
				if ( nameLen != buttonName.size() )
					continue;

#ifdef _WIN32
				// if ( _strnicmp( name, currentButton.data(), strlen( name ) ) != 0 )
				if ( _strnicmp( name, buttonName.data(), nameLen ) != 0 )
					continue;
#else
				if ( strncasecmp( name, buttonName.data(), nameLen ) != 0 )
					continue;
#endif
				if ( buttonPrefix.size() )
					buttonPrefix += "+";

				buttonPrefix += name;
				foundMatch = true;
				break;
			}

			if ( foundMatch )
				break;
		}

		lastButtonResults.clear();
	}
	
	// populate last button results

	if ( lastButtonResults.empty() )
	{
		std::string& result = results.emplace_back();
		result              = "\"" + buttonPrefix + "\"";
		return;
	}

	for ( size_t i = 0; i < lastButtonResults.size(); i++ )
	{
		std::string& result = results.emplace_back();

		result += "\"";

		if ( buttonPrefix.size() )
		{
			result += buttonPrefix + "+" + lastButtonResults[ i ];
		}
		else
		{
			result += lastButtonResults[ i ];
		}

		result += "\"";
	}
}


static void bind_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	bind_dropdown_keys( args, results );
	
	if ( results.size() > 1 )
		return;
	
	if ( args.size() < 2 )
		return;

	if ( results.empty() )
	{
		results.push_back( "INVALID BUTTON LIST" );
		return;
	}
	
	std::string key = results[ 0 ] + " ";
	results.clear();

	std::string command = args[ 1 ];
	std::vector< std::string > bindingResults;

	for ( u16 i = 0; i < EBinding_Count; i++ )
	{
		const char* binding = Input_BindingToStr( (EBinding)i );

		if ( binding == nullptr )
			continue;

		size_t bindingLen = strlen( binding );

		if ( bindingLen == 0 )
			continue;

		if ( command.size() )
		{
			// Check if this string is inside this other string
			const char* find = strcasestr( binding, command.data() );

			if ( !find )
				continue;
		}

		bindingResults.push_back( binding );
	}

	for ( std::string_view binding : bindingResults )
	{
		results.push_back( key + binding.data() );
	}
	
	//// Search Through ConVars
	//std::vector< std::string > searchResults;
	//Con_BuildAutoCompleteList( command, searchResults );
	//
	//for ( auto& cvar : searchResults )
	//{
	//	results.push_back( key + cvar );
	//}
}


CONCMD_DROP_VA( bind, bind_dropdown, 0, "Bind a key to a command" )
{
	if ( args.empty() )
		return;

	if ( args.size() == 1 )
	{
		Log_Error( "Please add a command after the keys for the binding command\n" );
		return;
	}

	// Split Buttons to a list
	std::vector< std::string > buttonNames;

	size_t buttonNameIndex = 0;
	buttonNames.push_back( "" );
	for ( size_t i = 0; i < args[ 0 ].size(); i++ )
	{
		if ( args[ 0 ][ i ] == '+' )
		{
			buttonNameIndex++;
			buttonNames.push_back( "" );
		}
		else
		{
			buttonNames[ buttonNameIndex ] += args[ 0 ][ i ];
		}
	}

	// Convert to Button Enums
	ChVector< EButton > buttons;
	for ( std::string_view buttonName : buttonNames )
	{
		EButton button = input->GetKeyFromName( buttonName );

		if ( button == SDL_SCANCODE_UNKNOWN )
		{
			Log_ErrorF( gLC_EditorInput, "Unknown Key: %s\n", buttonName );
			return;
		}

		buttons.push_back( button );
	}

	// Convert the command to an enum
	EBinding binding = Input_StrToBinding( args[ 1 ] );

	if ( binding == EBinding_Count )
		return;

	Input_BindKeys( buttons.data(), buttons.size(), binding );
}


static void CmdBindArchive( std::string& srOutput )
{
	srOutput += "// Bindings\n\n";

	for ( auto& [ buttonList, binding ] : gKeyBinds )
	{
		std::string buttonStr = Input_ButtonListToStr( &buttonList );

		if ( buttonStr.empty() )
			continue;
		
		srOutput += "bind \"" + buttonStr + "\" \"";
		srOutput += Input_BindingToStr( binding );
		srOutput += "\"\n";
	}
}


void Input_Init()
{
	Input_CalcMouseDelta();

	Con_AddArchiveCallback( CmdBindArchive );

	if ( gResetBindings )
	{
		Input_ResetBindings();
	}
}


EModMask Input_CalcModMask()
{
	EModMask modMask = EModMask_None;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_LCTRL ) )
		modMask |= EModMask_CtrlL;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_RCTRL ) )
		modMask |= EModMask_CtrlR;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_LSHIFT ) )
		modMask |= EModMask_ShiftL;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_RSHIFT ) )
		modMask |= EModMask_ShiftR;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_LALT ) )
		modMask |= EModMask_AltL;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_RALT ) )
		modMask |= EModMask_AltR;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_LGUI ) )
		modMask |= EModMask_GuiL;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_RGUI ) )
		modMask |= EModMask_GuiR;

	return modMask;
}


EModMask Input_ConvertKeyToModMask( EButton spKey )
{
	switch ( spKey )
	{
		default:
			return EModMask_None;

		case SDL_SCANCODE_LCTRL:
			return EModMask_CtrlL;

		case SDL_SCANCODE_RCTRL:
			return EModMask_CtrlR;

		case SDL_SCANCODE_LSHIFT:
			return EModMask_ShiftL;

		case SDL_SCANCODE_RSHIFT:
			return EModMask_ShiftR;

		case SDL_SCANCODE_LALT:
			return EModMask_AltL;

		case SDL_SCANCODE_RALT:
			return EModMask_AltR;

		case SDL_SCANCODE_LGUI:
			return EModMask_GuiL;

		case SDL_SCANCODE_RGUI:
			return EModMask_GuiR;
	}

	return EModMask_None;
}


// Extract Modifier Keys from the Key List
EModMask Input_GetModMask( EButton* spKeys, u8 sKeyCount, ChVector< EButton >& srNewKeys )
{
	EModMask modMask = EModMask_None;

	for ( u8 i = 0; i < sKeyCount; i++ )
	{
		EModMask newMask = Input_ConvertKeyToModMask( spKeys[ i ] );

		if ( newMask )
		{
			modMask |= newMask;
		}
		else
		{
			srNewKeys.push_back( spKeys[ i ] );
		}
	}

	return modMask;
}


void Input_Update()
{
	// update mouse inputs
	gMouseDelta = {};
	Input_CalcMouseDelta();

	// Find current ModMask
	EModMask modMaskPressed = Input_CalcModMask();

	// Update Binding Key States
	for ( auto& [ key, value ] : gKeyBinds )
	{
		bool pressed = true;
		for ( u8 i = 0; i < key.aCount; i++ )
		{
			if ( !input->KeyPressed( key.apButtons[ i ] ) )
			{
				pressed = false;
				break;
			}
		}

		// If we have a key modifier mask on this key
		// make sure it matches the modifier mask currently being pressed
		if ( key.aModMask && key.aModMask != modMaskPressed )
			pressed = false;	

		KeyState state = gBindingState[ value ];

		// Is this currently pressed
		if ( state & KeyState_Pressed )
		{
			// Make sure we remove the just pressed state
			state &= ~KeyState_JustPressed;

			// If we released the key, remove both press types,
			// and add both release types (released and just released)
			if ( !pressed )
			{
				state |= KeyState_Released | KeyState_JustReleased;
				state &= ~( KeyState_Pressed | KeyState_JustPressed );
			}
		}
		// Is this key currently released?
		else if ( state & KeyState_Released )
		{
			// Make sure we remove the just released state
			state &= ~KeyState_JustReleased;

			// If we pressed the key, remove both release types,
			// and add both press types (pressed and just pressed)
			if ( pressed )
			{
				state |= KeyState_Pressed | KeyState_JustPressed;
				state &= ~( KeyState_Released | KeyState_JustReleased );
			}
		}
		else
		{
			// Mark this key as just released
			state |= KeyState_Released | KeyState_JustReleased;
			state &= ~( KeyState_Pressed | KeyState_JustPressed );
		}

		gBindingState[ value ] = state;
	}
}


void Input_ResetBindings()
{
	Input_ClearBindings();

	Input_BindKeys( { SDL_SCANCODE_LCTRL, SDL_SCANCODE_Z }, EBinding_General_Undo );
	Input_BindKeys( { SDL_SCANCODE_LCTRL, SDL_SCANCODE_Y }, EBinding_General_Redo );
	
	Input_BindKeys( { SDL_SCANCODE_LCTRL, SDL_SCANCODE_X }, EBinding_General_Cut );
	Input_BindKeys( { SDL_SCANCODE_LCTRL, SDL_SCANCODE_C }, EBinding_General_Copy );
	Input_BindKeys( { SDL_SCANCODE_LCTRL, SDL_SCANCODE_V }, EBinding_General_Paste );
	
	Input_BindKey( SDL_SCANCODE_SPACE, EBinding_Viewport_MouseLook );
	
	Input_BindKey( SDL_SCANCODE_W, EBinding_Viewport_MoveForward );
	Input_BindKey( SDL_SCANCODE_S, EBinding_Viewport_MoveBack );
	Input_BindKey( SDL_SCANCODE_A, EBinding_Viewport_MoveLeft );
	Input_BindKey( SDL_SCANCODE_D, EBinding_Viewport_MoveRight );
	Input_BindKey( SDL_SCANCODE_Q, EBinding_Viewport_MoveUp );
	Input_BindKey( SDL_SCANCODE_E, EBinding_Viewport_MoveDown );

	Input_BindKey( SDL_SCANCODE_LSHIFT, EBinding_Viewport_Sprint );
	Input_BindKey( SDL_SCANCODE_LCTRL, EBinding_Viewport_Slow );

	Input_BindKey( EButton_MouseLeft, EBinding_Viewport_SelectSingle );
	Input_BindKey( EButton_MouseRight, EBinding_Viewport_SelectMulti );
	// Input_BindKeys( { SDL_SCANCODE_LCTRL, EButton_MouseLeft }, EBinding_Viewport_SelectMulti );

	Input_BindKey( SDL_SCANCODE_LSHIFT, EBinding_Viewport_GizmoSnap );
	Input_BindKey( SDL_SCANCODE_LEFTBRACKET, EBinding_Viewport_GizmoSnapDecrement );
	Input_BindKey( SDL_SCANCODE_RIGHTBRACKET, EBinding_Viewport_GizmoSnapIncrement );
}


void Input_ClearBindings()
{
	// Free Button Memory
	for ( auto& [ buttonList, binding ] : gKeyBinds )
	{
		if ( buttonList.apButtons )
			free( buttonList.apButtons );
	}
	
	gKeyBinds.clear();
}


const std::unordered_map< ButtonList_t, EBinding >& Input_GetBindings()
{
	return gKeyBinds;
}


const ButtonList_t* Input_GetKeyBinding( EBinding sBinding )
{
	for ( auto& [ buttonList, binding ] : gKeyBinds )
	{
		if ( binding == sBinding )
			return &buttonList;
	}

	return nullptr;
}


std::string Input_ButtonListToStr( const ButtonList_t* spButtonList )
{
	if ( !spButtonList )
		return "";

	std::vector< std::string > buttons;

	if ( spButtonList->aModMask & EModMask_CtrlL )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_LCTRL ) );

	if ( spButtonList->aModMask & EModMask_CtrlR )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_RCTRL ) );

	if ( spButtonList->aModMask & EModMask_ShiftL )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_LSHIFT ) );

	if ( spButtonList->aModMask & EModMask_ShiftR )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_RSHIFT ) );

	if ( spButtonList->aModMask & EModMask_AltL )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_LALT ) );

	if ( spButtonList->aModMask & EModMask_AltR )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_RALT ) );

	if ( spButtonList->aModMask & EModMask_GuiL )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_LGUI ) );

	if ( spButtonList->aModMask & EModMask_GuiR )
		buttons.push_back( input->GetKeyName( (EButton)SDL_SCANCODE_RGUI ) );

	for ( u8 i = 0; i < spButtonList->aCount; i++ )
	{
		buttons.push_back( input->GetKeyName( spButtonList->apButtons[ i ] ) );
	}

	if ( buttons.empty() )
		return "";

	std::string output = buttons[ 0 ];

	for ( size_t i = 1; i < buttons.size(); i++ )
	{
		output += "+" + buttons[ i ];
	}

	return output;
}


void Input_CalcMouseDelta()
{
	const glm::ivec2& baseDelta = input->GetMouseDelta();

	gMouseDelta.x               = baseDelta.x * m_sensitivity;
	gMouseDelta.y               = baseDelta.y * m_sensitivity;
}


glm::vec2 Input_GetMouseDelta()
{
	return gMouseDelta * gMouseDeltaScale;
}


void Input_SetMouseDeltaScale( const glm::vec2& scale )
{
	gMouseDeltaScale = scale;
}


const glm::vec2& Input_GetMouseDeltaScale()
{
	return gMouseDeltaScale;
}


void Input_BindKey( EButton sKey, EBinding sBinding )
{
	Input_BindKeys( &sKey, 1, sBinding );

#if 0
	if ( sBinding > EBinding_Count )
	{
		Log_ErrorF( gLC_EditorInput, "Trying to bind key \"%s\" to Invalid Binding: \"%d\"\n", input->GetKeyName( sKey ), sBinding );
		return;
	}

	input->RegisterKey( sKey );

	bool foundKey = false;
	for ( auto& [ key, value ] : gKeyBinds )
	{
		if ( key.aCount != 1 )
			continue;

		if ( key.spButtons[ 0 ] == sKey )
		{
			foundKey = true;
		}

		for ( u8 i = 0; i < key.aCount; i++ )
	}

	auto it = gKeyBinds.find( sKey );
	if ( it == gKeyBinds.end() )
	{
		// bind it
		gKeyBinds[ sKey ]         = sBinding;
		gBindingToKey[ sBinding ] = sKey;
	}
	else
	{
		// update this bind (does this work?)
		it->second                = sBinding;
		gBindingToKey[ sBinding ] = sKey;
	}

	Log_DevF( gLC_EditorInput, 1, "Bound Key: \"%s\" \"%s\"\n", input->GetKeyName( sKey ), Input_BindingToStr( sBinding ) );
#endif
}


void Input_ClearBinding( EBinding sBinding )
{
	// auto it = gBindingToKey.find( sBinding );
	// if ( it == gBindingToKey.end() )
	// 	return;
	// 
	// gBindingToKey.erase( it );

	for ( auto& [ key, value ] : gKeyBinds )
	{
		if ( value != sBinding )
			continue;

		// Free Memory
		free( key.apButtons );

		gKeyBinds.erase( key );
		break;
	}
}


void Input_BindKey( SDL_Scancode sKey, EBinding sBinding )
{
	Input_BindKeys( (EButton*)&sKey, 1, sBinding );
	// Input_BindKey( (EButton)sKey, sBinding );
}


std::string Input_GetKeyComboName( EButton* spKeys, u8 sKeyCount )
{
	std::string keyList = input->GetKeyName( spKeys[ 0 ] );

	for ( u8 i = 1; i < sKeyCount; i++ )
	{
		keyList += "+";
		keyList += input->GetKeyName( spKeys[ i ] );
	}

	return keyList;
}


void Input_PrintNewBinding( EButton* spKeys, u8 sKeyCount, EBinding sBinding )
{
	Log_DevF( gLC_EditorInput, 1, "Bound Keys: \"%s\" - \"%s\"\n", Input_GetKeyComboName( spKeys, sKeyCount ).c_str(), Input_BindingToStr( sBinding ) );
}


void Input_BindKeys( EButton* spKeys, u8 sKeyCount, EBinding sBinding )
{
	if ( sKeyCount == 0 )
	{
		Log_Error( gLC_EditorInput, "Trying to bind 0 keys?" );
		return;
	}

	if ( sBinding >= EBinding_Count )
	{
		Log_ErrorF( gLC_EditorInput, "Trying to bind key \"%s\" to Invalid Binding: \"%d\"\n", Input_GetKeyComboName( spKeys, sKeyCount ).c_str(), sBinding );
		return;
	}

	for ( u8 i = 0; i < sKeyCount; i++ )
		input->RegisterKey( spKeys[ i ] );

	ChVector< EButton > newKeyList;
	EModMask            modMask = Input_GetModMask( spKeys, sKeyCount, newKeyList );

	// Check if the key is already bound to the exact same binding
	for ( auto& [ key, value ] : gKeyBinds )
	{
		if ( newKeyList.size() != key.aCount )
			continue;

		if ( modMask != key.aModMask )
			continue;

		bool found = true;
		for ( u8 i = 0; i < key.aCount; i++ )
		{
			if ( spKeys[ i ] != key.apButtons[ i ] )
			{
				found = false;
				break;
			}
		}

		if ( found )
		{
			if ( value == sBinding )
			{
				Log_DevF( gLC_EditorInput, 1, "Key Binding Already Exists: \"%s\" - \"%s\"\n", Input_GetKeyComboName( spKeys, sKeyCount ).c_str(), Input_BindingToStr( sBinding ) );
				return;
			}

			// Clearing Old Bindings of this key, disabled so you can have a key do multiple things depending on the context
			// For example, shift to sprint move, and shift to snap the gizmos in increments

#if 0
			Input_ClearBinding( value );

			//value = sBinding;

			//Input_ClearBinding( sBinding );
			// gBindingToKey[ sBinding ] = key;

			//Input_PrintNewBinding( spKeys, sKeyCount, sBinding );
			//return;
#endif

			break;
		}
	}

	// Did not find key, allocate new one

	ButtonList_t buttonList{};
	buttonList.aModMask       = modMask;
	buttonList.aCount         = newKeyList.size();

	if ( buttonList.aCount )
		buttonList.apButtons = ch_malloc< EButton >( buttonList.aCount );

	for ( u8 i = 0; i < buttonList.aCount; i++ )
	{
		buttonList.apButtons[ i ] = newKeyList[ i ];
	}

	gKeyBinds[ buttonList ]   = sBinding;
	// gBindingToKey[ sBinding ] = buttonList;

	Input_PrintNewBinding( spKeys, sKeyCount, sBinding );
}


void Input_BindKeys( SDL_Scancode* spKeys, u8 sKeyCount, EBinding sBinding )
{
	return Input_BindKeys( (EButton*)spKeys, sKeyCount, sBinding );
}


void Input_BindKeys( std::vector< EButton > sKeys, EBinding sKeyBind )
{
	Input_BindKeys( sKeys.data(), sKeys.size(), sKeyBind );
}


void Input_BindKeys( std::vector< SDL_Scancode > sKeys, EBinding sKeyBind )
{
	Input_BindKeys( (EButton*)sKeys.data(), sKeys.size(), sKeyBind );
}


#if 1
bool Input_KeyPressed( EBinding sKeyBind )
{
	if ( sKeyBind > EBinding_Count )
		return false;

	return gBindingState[ sKeyBind ] & KeyState_Pressed;
}


bool Input_KeyReleased( EBinding sKeyBind )
{
	if ( sKeyBind > EBinding_Count )
		return false;

	return gBindingState[ sKeyBind ] & KeyState_Released;
}


bool Input_KeyJustPressed( EBinding sKeyBind )
{
	if ( sKeyBind > EBinding_Count )
		return false;

	return gBindingState[ sKeyBind ] & KeyState_JustPressed;
}


bool Input_KeyJustReleased( EBinding sKeyBind )
{
	if ( sKeyBind > EBinding_Count )
		return false;

	return gBindingState[ sKeyBind ] & KeyState_JustReleased;
}


KeyState Input_KeyState( EBinding sKeyBind )
{
	if ( sKeyBind > EBinding_Count )
		return KeyState_Invalid;

	return gBindingState[ sKeyBind ];
}

#else

bool Input_KeyPressed( EBinding sKeyBind )
{
	auto it = gBindingToKey.find( sKeyBind );
	if ( it == gBindingToKey.end() )
		return false;

	for ( u8 i = 0; i < it->second.aCount; i++ )
	{
		if ( !input->KeyPressed( it->second.spButtons[ i ] ) )
			return false;
	}

	return true;
}


bool Input_KeyReleased( EBinding sKeyBind )
{
	auto it = gBindingToKey.find( sKeyBind );
	if ( it == gBindingToKey.end() )
		return false;

	for ( u8 i = 0; i < it->second.aCount; i++ )
	{
		if ( !input->KeyReleased( it->second.spButtons[ i ] ) )
			return false;
	}

	return true;
}


bool Input_KeyJustPressed( EBinding sKeyBind )
{
	auto it = gBindingToKey.find( sKeyBind );
	if ( it == gBindingToKey.end() )
		return false;

	bool oneJustPressed = false;

	for ( u8 i = 0; i < it->second.aCount; i++ )
	{
		if ( !input->KeyPressed( it->second.spButtons[ i ] ) )
			return false;

		oneJustPressed |= input->KeyJustPressed( it->second.spButtons[ i ] );
	}

	return oneJustPressed;
}


bool Input_KeyJustReleased( EBinding sKeyBind )
{
	auto it = gBindingToKey.find( sKeyBind );
	if ( it == gBindingToKey.end() )
		return false;

	bool oneJustPressed = false;

	for ( u8 i = 0; i < it->second.aCount; i++ )
	{
		if ( !input->KeyReleased( it->second.spButtons[ i ] ) )
			return false;

		oneJustPressed |= input->KeyJustReleased( it->second.spButtons[ i ] );
	}

	return oneJustPressed;
}

#endif