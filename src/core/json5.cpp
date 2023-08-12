#include <string.h>
#include <vector>
#include <forward_list>
#include <stdlib.h>

#include "core/json5.h"


// TODO: be able to have verbose json error printing with this
// CONVAR( json_verbose, 0 );


constexpr int STRING_MALLOC_SIZE = 256;


struct JsonParseState_t
{
};


static bool Json_IsWhitespace( char c )
{
	return c < '!' || c > '~';
}


// Ends on the first character after whitespace
static EJsonError Json_SkipWhitespace( const char*& str )
{
StartOfSkip:
	while ( *str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' )
		str++;

	// check for comment character
	if ( *str == '/' )
	{
		str++;

		// is this a multiline comment?
		if ( *str == '*' )
		{
			// read until */ character (probably slow)
			while ( strncmp( str, "*/", 2 ) != 0 )
				str++;

			// advance past */ character
			str++;

			// continue checking whitespace
			goto StartOfSkip;
		}
		// is this a line comment?
		else if ( *str == '/' )
		{
			// read until end of line
			while ( *str != '\n' && *str != '\r' )
				str++;

			// continue checking whitespace
			goto StartOfSkip;
		}

		// must be some invalid character
		return EJsonError_InvalidCharacter;
	}

	return EJsonError_None;
}


static bool Json_IsControlCharacter( char c )
{
	// return c == '"' || c == '\'' || c == '[' || c == ']' || c == '{' || c == '}' || c == ':' || c == ',';
	return c == ':' || c == ',';
}


static bool Json_IsNumber( char c )
{
	return c >= '0' && c <= '9';
}


static bool Json_ValidQuotelessKey( char c )
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '$';
}


static EJsonError Json_ParseQuote( const char*& str, char endChar, char*& output )
{
	str++;
	output = (char*)malloc( STRING_MALLOC_SIZE ); // preallocate chars

	if ( output == nullptr )
		return EJsonError_OutOfMemory;

	memset( output, 0, STRING_MALLOC_SIZE );

	// const char* base = str;
	int i = 0;
	for ( int j = 1 ; *str != endChar; i++, str++ )
	{
		// can't have new line in it
		if ( *str == '\n' || *str == '\r' )
			return EJsonError_NewLineInQuote;

		if ( i % STRING_MALLOC_SIZE == 0 )
		{
			auto* tmp = (char*)realloc( output, STRING_MALLOC_SIZE * ( j + 1 ) );
			if ( tmp == nullptr )
			{
				free( output );
				return EJsonError_OutOfMemory;
			}

			memset( tmp, 0, ( STRING_MALLOC_SIZE * ( j + 1 ) ) - ( STRING_MALLOC_SIZE * j ) );

			j++;
			output = tmp;
		}

		if ( *str == '\\' )
		{
			str++;

			if ( *str == 'n' )
				output[ i ] = '\n';
			else if ( *str == 't' )
				output[ i ] = '\t';
			else
				output[ i ] = *str;
		}
		else
		{
			output[ i ] = *str;
		}
	}

	str++;
	auto* tmp = (char*)realloc( output, i+1 );

	if ( tmp == nullptr )
	{
		free( output );
		return EJsonError_OutOfMemory;
	}

	output = tmp;
	output[ i ] = '\0';
	return EJsonError_None;
}


static EJsonError Json_ParseString( const char*& str, char*& output, int* len = nullptr )
{
	const char* base = str;

	int i = 0;
	for ( ; !Json_IsWhitespace( *str ) && *str != ':'; i++, str++ )
	{
		if ( !Json_ValidQuotelessKey( *str ) )
			return EJsonError_InvalidQuotelessKeyCharacter;
	}

	output = (char*)malloc( i + 1 );

	if ( output == nullptr )
		return EJsonError_OutOfMemory;

	memset( output, 0, i + 1 );

	// output = const_cast< char* >( base );
	strncpy( output, base, i );
	output[ i ] = '\0';

	if ( len )
		*len = i;

	return EJsonError_None;
}


static EJsonError Json_ParseStringNoCheck( const char*& str, char*& output, int* len = nullptr )
{
	const char* base = str;

	int i = 0;
	for ( ; !Json_IsWhitespace( *str ) && *str != ':' && *str != ',' && *str != ']' && *str != '}'; i++, str++ );

	output = (char*)malloc( i + 1 );

	if ( output == nullptr )
		return EJsonError_OutOfMemory;

	memset( output, 0, i + 1 );

	// output = const_cast< char* >( base );
	strncpy( output, base, i );
	output[ i ] = '\0';

	if ( len )
		*len = i;

	return EJsonError_None;
}


// TODO: add hex support, and other types
static EJsonError Json_ParseNumber( const char*& str, JsonObject_t& srObj )
{
	const char* base = str;

	srObj.aType = EJsonType_Int;

	int i = 0;
	for ( ; !Json_IsWhitespace( *str ) && *str != ':' && *str != ',' && *str != ']'; i++ )
	{
		if ( *str == '.' )
			srObj.aType = EJsonType_Double;

		if ( *str == '+' )
			base = ++str;
		else
			str++;
	}

	char* output = (char*)malloc( i + 1 );

	if ( output == nullptr )
		return EJsonError_OutOfMemory;

	memset( output, 0, i + 1 );

	// output = const_cast< char* >( base );
	strncpy( output, base, i );
	output[ i ] = '\0';

	if ( srObj.aType == EJsonType_Double )
	{
		char *end;
		srObj.aDouble = strtod( output, &end );

		if ( end == output )
		{
			free( output );
			return EJsonError_InvalidDouble;
		}
	}
	else  // int
	{
		char *end;
		srObj.aInt = strtol( output, &end, 10 );

		if ( end == output )
		{
			free( output );
			return EJsonError_InvalidInt;
		}
	}

	free( output );
	return EJsonError_None;
}


// --------------------------------------------------------------------------------------
// Public Functions


EJsonError Json_Parse( JsonObject_t* spRoot, const char* spStr )
{
	if ( spRoot == nullptr )
		return EJsonError_RootIsNullptr;

	if ( spStr == nullptr )
		return EJsonError_DataIsNullptr;

	// stack for types
	// EJsonType typeStack[128] = {};
	// int typeIndex = 0;

	// std::vector< JsonObject_t* > objStack;
	// objStack.push_back( spRoot );

	std::forward_list< JsonObject_t* > objStack;
	objStack.push_front( spRoot );
	// int stackCount = 1;
	// JsonObject_t* objStack = (JsonObject_t*)malloc( sizeof( JsonObject_t ) );
	// objStack[0] = *spRoot;
	// JsonObject_t* objStack = spRoot;
	// JsonObject_t* cur = nullptr;

	char c;
	for (; *spStr;)
	{
		if ( EJsonError err = Json_SkipWhitespace( spStr ) )
			return err;

		c = *spStr;

		// ------------------------------------------------------
		// Find Root if needed
		if ( spRoot->aType == EJsonType_Invalid )
		{
			switch ( c )
			{
				case '{':
				{
					spStr++;
					spRoot->aType = EJsonType_Object;

					break;
				}
				case '[':
				{
					spStr++;
					spRoot->aType = EJsonType_Array;

					break;
				}
				default:
				{
					return EJsonError_UnexpectedRootType;
				}
			}

			if ( EJsonError err = Json_SkipWhitespace( spStr ) )
				return err;

			c = *spStr;
		}
		else if ( objStack.empty() )
		{
			break;
		}

		// ------------------------------------------------------
		// First, check the key if in an object

		// also add a new JsonObject_t
		// JsonObject_t* next = (JsonObject_t*)malloc( cur->apObjects);
		// cur->apObjects = 

		// parent->apObjects = ( JsonObject_t* )malloc( sizeof( JsonObject_t ) );
		// 
		// if ( parent->apObjects == nullptr )
		// 	return EJsonError_OutOfMemory;

		// JsonObject_t cur{};

		// if ( objStack[ objStack.front() - 1 ]->aType == EJsonType_Object )
		if ( objStack.front()->aType == EJsonType_Object )
		{
			if ( c == '}' )
			{
				spStr++;
				
				if ( EJsonError err = Json_SkipWhitespace( spStr ) )
					return err;

				// ------------------------------------------------------
				// Check for the ',' character

				// TODO: this doesn't require a comma
				if ( *spStr == ',' )
					spStr++;

				objStack.pop_front();

				continue;
			}
		}
		else if ( objStack.front()->aType == EJsonType_Array )
		{
			if ( c == ']' )
			{
				spStr++;

				if ( EJsonError err = Json_SkipWhitespace( spStr ) )
					return err;

				// ------------------------------------------------------
				// Check for the ',' character

				// TODO: this doesn't require a comma
				if ( *spStr == ',' )
					spStr++;

				objStack.pop_front();

				continue;
			}
		}


		JsonObject_t& cur = objStack.front()->aObjects.emplace_back();

		if ( objStack.front()->aType == EJsonType_Object )
		{
			switch ( c )
			{
				case '"':
				case '\'':
				{
					if ( EJsonError err = Json_ParseQuote( spStr, c, cur.apName ) )
						return err;
					break;
				}
				// Has to be a quoteless string
				default:
				{
					if ( !Json_ValidQuotelessKey( c ) )
					{
						// if it's a number, warn about that
						if ( Json_IsNumber( c ) )
							return EJsonError_KeyStartsWithNumber;

						return EJsonError_InvalidQuotelessKeyCharacter;
					}

					if ( EJsonError err = Json_ParseString( spStr, cur.apName ) )
						return err;

					break;
				}
			}
		}

		// ------------------------------------------------------
		// Verify we are at the ':' character

		if ( EJsonError err = Json_SkipWhitespace( spStr ) )
			return err;

		// only if we're not in an array

		if ( objStack.front()->aType != EJsonType_Array )
		{
			if ( *spStr != ':' )
				return EJsonError_ExpectedColonCharacter;

			spStr++;

			if ( EJsonError err = Json_SkipWhitespace( spStr ) )
				return err;
		}

		// ------------------------------------------------------
		// Now, check the value

		c = *spStr;

		switch ( c )
		{
			case '{':
			{
				spStr++;
				cur.aType = EJsonType_Object;

				objStack.push_front( &cur );

				break;
			}
			case '[':
			{
				spStr++;
				cur.aType = EJsonType_Array;

				objStack.push_front( &cur );

				break;
			}
			case '"':
			case '\'':
			{
				if ( EJsonError err = Json_ParseQuote( spStr, c, cur.apString ) )
					return err;

				cur.aType = EJsonType_String;
				break;
			}
			default:
			{
				if ( Json_IsNumber( c ) || c == '.' || c == '+' || c == '-' )
				{
					if ( EJsonError err = Json_ParseNumber( spStr, cur ) )
						return err;
				}
				
				else
				{
					char* string;
					int strLen = 0;
					if ( EJsonError err = Json_ParseStringNoCheck( spStr, string, &strLen ) )
						return err;

					if ( strLen == 4 && strncmp( string, "true", 4 ) == 0 )
					{
						cur.aType = EJsonType_True;
					}
					else if ( strLen == 5 && strncmp( string, "false", 5 ) == 0 )
					{
						cur.aType = EJsonType_False;
					}
					else if ( strLen == 4 && strncmp( string, "null", 4 ) == 0 )
					{
						cur.aType = EJsonType_Null;
					}
					else
					{
						return EJsonError_InvalidCharacter;
					}
				}
			}
		}

		if ( EJsonError err = Json_SkipWhitespace( spStr ) )
			return err;

		c = *spStr;

		// ------------------------------------------------------
		// Check for the ',' character

		// TODO: this doesn't require a comma
		if ( c == ',' )
			spStr++;

		// if ( c != ':' )
		// 	return EJsonError_ExpectedColonCharacter;
	}

	return EJsonError_None;
}


void Json_Free( JsonObject_t* spRoot )
{
	std::vector< JsonObject_t* > objStack;
	objStack.push_back( spRoot );

	while ( !objStack.empty() )
	{
		if ( objStack[ objStack.size() - 1 ]->aType == EJsonType_Object && objStack[ objStack.size() - 1 ]->aObjects.size() )
		{
			size_t j = objStack.size() - 1;
			for ( size_t i = 0; i < objStack[j]->aObjects.size(); i++ )
			{
				objStack.push_back( &objStack[j]->aObjects[i] );
			}

			continue;
		}

		if ( objStack[objStack.size() - 1]->aType == EJsonType_String )
		{
			free( objStack[objStack.size() - 1]->apString );
		}

		if ( objStack[objStack.size() - 1]->apName )
		{
			free( objStack[objStack.size() - 1]->apName );
		}

		objStack.pop_back();

		if ( objStack.size() && objStack[ objStack.size() - 1 ]->aType == EJsonType_Object && objStack[ objStack.size() - 1 ]->aObjects.size() )
		{
			objStack[objStack.size() - 1]->aObjects.clear();
		}
	}
}


void Json_ToString( JsonObject_t* spRoot )
{
}


// TODO: IMPLEMENT ME !!!!!
const char* Json_ErrorToStr( EJsonError sErr )
{
	switch ( sErr )
	{
		default:
			return "None";
	}
}


const char* Json_TypeToStr( EJsonType sType )
{
	switch ( sType )
	{
		default:
			return "None";

		case EJsonType_Invalid:
			return "Invalid";

		case EJsonType_Object:
			return "Object";

		case EJsonType_String:
			return "String";

		case EJsonType_Int:
			return "Int";

		case EJsonType_Double:
			return "Double";

		case EJsonType_False:
			return "False";

		case EJsonType_True:
			return "True";

		case EJsonType_Null:
			return "Null";

		case EJsonType_Array:
			return "Array";
	}
}


