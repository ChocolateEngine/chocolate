#include "gui.h"
#include "rmlui.h"


LOG_REGISTER_CHANNEL2( RmlUI, LogColor::Default );


RmlUI  gRmlUI;


double RmlUI::GetElapsedTime()
{
	return gRealTime;
}


/// Log the specified message.
/// @param[in] type Type of log message, ERROR, WARNING, etc.
/// @param[in] message Message to log.
/// @return True to continue execution, false to break into the debugger.
bool RmlUI::LogMessage( Rml::Log::Type sType, const Rml::String& srMessage )
{
	switch ( sType )
	{
		case Rml::Log::Type::LT_ALWAYS:
			Log_MsgF( gLC_RmlUI, "%s\n", srMessage.c_str() );
			break;

		case Rml::Log::Type::LT_ERROR:
			Log_ErrorF( gLC_RmlUI, "%s\n", srMessage.c_str() );
			break;

		case Rml::Log::Type::LT_ASSERT:
			Log_WarnF( gLC_RmlUI, "Assert - %s\n", srMessage.c_str() );
			break;

		case Rml::Log::Type::LT_WARNING:
			Log_WarnF( gLC_RmlUI, "%s\n", srMessage.c_str() );
			break;

		default:
		case Rml::Log::Type::LT_INFO:
			Log_MsgF( gLC_RmlUI, "Info - %s\n", srMessage.c_str() );
			break;

		case Rml::Log::Type::LT_DEBUG:
			Log_MsgF( gLC_RmlUI, "Debug - %s\n", srMessage.c_str() );
			break;

		case Rml::Log::Type::LT_MAX:
			break;
	}

	return true;
}
