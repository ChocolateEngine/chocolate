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
	{
		str++;

		if ( *str == '\0' )
			return EJsonError_None;
	}

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

			if ( *str == '\0' )
				return EJsonError_None;

			// advance past */ character
			str++;

			if ( *str == '\0' )
				return EJsonError_None;

			str++;

			if ( *str == '\0' )
				return EJsonError_None;

			// continue checking whitespace
			goto StartOfSkip;
		}
		// is this a line comment?
		else if ( *str == '/' )
		{
			// read until end of line
			while ( *str != '\n' && *str != '\r' )
				str++;

			if ( *str == '\0' )
				return EJsonError_None;

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


static EJsonError Json_ParseQuote( const char*& str, char endChar, ch_string& output )
{
	str++;
	output.data = (char*)malloc( STRING_MALLOC_SIZE ); // preallocate chars
	output.size = 0;

	if ( output.data == nullptr )
		return EJsonError_OutOfMemory;

	memset( output.data, 0, STRING_MALLOC_SIZE );

	// const char* base = str;
	int i = 0;
	for ( int j = 1 ; *str != endChar; i++, str++ )
	{
		// can't have new line in it
		if ( *str == '\n' || *str == '\r' )
			return EJsonError_NewLineInQuote;

		if ( i % STRING_MALLOC_SIZE == 0 )
		{
			auto* tmp = (char*)realloc( output.data, STRING_MALLOC_SIZE * ( j + 1 ) );
			if ( tmp == nullptr )
			{
				free( output.data );
				return EJsonError_OutOfMemory;
			}

			memset( tmp, 0, ( STRING_MALLOC_SIZE * ( j + 1 ) ) - ( STRING_MALLOC_SIZE * j ) );

			j++;
			output.data = tmp;
		}

		if ( *str == '\\' )
		{
			str++;

			if ( *str == 'n' )
				output.data[ i ] = '\n';
			else if ( *str == 't' )
				output.data[ i ] = '\t';
			else
				output.data[ i ] = *str;
		}
		else
		{
			output.data[ i ] = *str;
		}
	}

	str++;
	auto* tmp = (char*)realloc( output.data, i + 1 );

	if ( tmp == nullptr )
	{
		free( output.data );
		return EJsonError_OutOfMemory;
	}

	output.data = tmp;
	output.size = i;
	output.data[ i ] = '\0';

	// add this string to the string list
	ch_str_add( output );

	return EJsonError_None;
}


static EJsonError Json_ParseString( const char*& str, ch_string& output )
{
	const char* base = str;

	int i = 0;
	for ( ; !Json_IsWhitespace( *str ) && *str != ':'; i++, str++ )
	{
		if ( !Json_ValidQuotelessKey( *str ) )
			return EJsonError_InvalidQuotelessKeyCharacter;
	}

	output.data = (char*)malloc( i + 1 );

	if ( output.data == nullptr )
		return EJsonError_OutOfMemory;

	memset( output.data, 0, i + 1 );

	// output = const_cast< char* >( base );
	strncpy( output.data, base, i );
	output.data[ i ] = '\0';
	output.size      = i;

	// add this string to the string list
	ch_str_add( output );

	return EJsonError_None;
}


static EJsonError Json_ParseStringNoCheck( const char*& str, ch_string& output )
{
	const char* base = str;

	int i = 0;
	for ( ; !Json_IsWhitespace( *str ) && *str != ':' && *str != ',' && *str != ']' && *str != '}'; i++, str++ );

	output.data = (char*)malloc( i + 1 );

	if ( output.data == nullptr )
		return EJsonError_OutOfMemory;

	memset( output.data, 0, i + 1 );

	// output = const_cast< char* >( base );
	strncpy( output.data, base, i );
	output.data[ i ] = '\0';
	output.size      = i;

	// add this string to the string list
	ch_str_add( output );

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
		srObj.aInt = strtoll( output, &end, 0 );

		if ( end == output )
		{
			free( output );
			return EJsonError_InvalidInt;
		}
	}

	free( output );
	return EJsonError_None;
}


static JsonObject_t* Json_IncrementObjectList( JsonArray_t& data )
{
	JsonObject_t* tmp = (JsonObject_t*)realloc( data.apData, sizeof( JsonObject_t ) * ( data.aCount + 1 ) );

	if ( tmp == nullptr )
	{
		free( data.apData );
		return nullptr;
	}

	memset( tmp + data.aCount, 0, sizeof( JsonObject_t ) * 1 );

	u64 index = data.aCount;

	data.apData = tmp;
	data.aCount++;

	return &data.apData[ index ];
}


// --------------------------------------------------------------------------------------
// Public Functions


// TODO: try using a stack allocator so this can be all just one big allocation?
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

	memset( spRoot, 0, sizeof( JsonObject_t ) );

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


		JsonObject_t* cur = Json_IncrementObjectList( objStack.front()->aObjects );

		if ( objStack.front()->aType == EJsonType_Object )
		{
			switch ( c )
			{
				case '"':
				case '\'':
				{
					if ( EJsonError err = Json_ParseQuote( spStr, c, cur->name ) )
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

					if ( EJsonError err = Json_ParseString( spStr, cur->name ) )
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
				cur->aType = EJsonType_Object;

				objStack.push_front( cur );

				break;
			}
			case '[':
			{
				spStr++;
				cur->aType = EJsonType_Array;

				objStack.push_front( cur );

				break;
			}
			case '"':
			case '\'':
			{
				if ( EJsonError err = Json_ParseQuote( spStr, c, cur->aString ) )
					return err;

				cur->aType = EJsonType_String;
				break;
			}
			default:
			{
				if ( Json_IsNumber( c ) || c == '.' || c == '+' || c == '-' )
				{
					if ( EJsonError err = Json_ParseNumber( spStr, *cur ) )  // ---------------------------------------------------------- why is this a pointer?
						return err;
				}
				
				else
				{
					ch_string string;
					if ( EJsonError err = Json_ParseStringNoCheck( spStr, string ) )
						return err;

					if ( ch_str_equals( string, "true", 4 ) )
					{
						cur->aType = EJsonType_True;
					}
					else if ( ch_str_equals( string, "false", 5 ) )
					{
						cur->aType = EJsonType_False;
					}
					else if ( ch_str_equals( string, "null", 4 ) )
					{
						cur->aType = EJsonType_Null;
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
	std::vector< JsonObject_t* > objList;
	objList.push_back( spRoot );

	// First, get all objects into one huge list
	for ( size_t objIndex = 0; objIndex < objList.size(); objIndex++ )
	{
		// Does the current object have children?
		if ( !( objList[ objIndex ]->aType == EJsonType_Object || objList[ objIndex ]->aType == EJsonType_Array ) )
			continue;

		if ( objList[ objIndex ]->aObjects.aCount == 0 )
			continue;

		// put all children in the list and reserve memory for it
		size_t j = objIndex;
		objList.reserve( objList.size() + objList[j]->aObjects.aCount );
		for ( size_t i = 0; i < objList[j]->aObjects.aCount; i++ )
		{
			objList.push_back( &objList[j]->aObjects.apData[ i ] );
		}
	}

	// Now free all the objects
	while ( !objList.empty() )
	{
		// If this is a string type, free it
		if ( objList[objList.size() - 1]->aType == EJsonType_String )
		{
			ch_str_free( objList[objList.size() - 1]->aString.data );
			objList[ objList.size() - 1 ]->aString.data = nullptr;
			objList[ objList.size() - 1 ]->aString.size = 0;
		}
		else if ( objList[objList.size() - 1]->aType == EJsonType_Object || objList[objList.size() - 1]->aType == EJsonType_Array )
		{
			// Free the object list
			free( objList[objList.size() - 1]->aObjects.apData );
			objList[ objList.size() - 1 ]->aObjects.apData = nullptr;
			objList[ objList.size() - 1 ]->aObjects.aCount = 0;
		}

		// Free the name if it exists
		if ( objList[objList.size() - 1]->name.data )
		{
			ch_str_free( objList[ objList.size() - 1 ]->name.data );
			objList[ objList.size() - 1 ]->name.data = nullptr;
			objList[ objList.size() - 1 ]->name.size = 0;
		}

		// pop the object off the stack
		objList.pop_back();
	}
}


void Json_ToString( JsonObject_t* spRoot )
{
}


const char* Json_ErrorToStr( EJsonError sErr )
{
	switch ( sErr )
	{
		default:
		case EJsonError_None:
			return "None";

		case EJsonError_RootIsNullptr:
			return "Root is nullptr";

		case EJsonError_DataIsNullptr:
			return "Data is nullptr";

		case EJsonError_InvalidCharacter:
			return "Invalid Character";

		case EJsonError_OutOfMemory:
			return "Out of Memory";

		case EJsonError_UnexpectedRootType:
			return "Unexpected Root Type";

		case EJsonError_ExpectedEndOfBlock:
			return "Expected end of block";

		case EJsonError_ExpectedEndOfArray:
			return "Expected end of array";

		case EJsonError_ExpectedEndOfQuote:
			return "Expected end of quote";

		case EJsonError_KeyStartsWithNumber:
			return "Key starts with number";

		case EJsonError_InvalidQuotelessKeyCharacter:
			return "Invalid Quoteless Key Character";

		case EJsonError_InvalidDouble:
			return "Invalid Double";

		case EJsonError_InvalidInt:
			return "Invalid Integer";

		case EJsonError_ExpectedColonCharacter:
			return "Expected Colon Character";

		case EJsonError_ExpectedCommaCharacter:
			return "Expected Comma Character";

		case EJsonError_NewLineInQuote:
			return "New Line In Quote";
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


