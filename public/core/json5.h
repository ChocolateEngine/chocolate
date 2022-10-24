#pragma once

#include <vector>

enum EJsonError
{
	EJsonError_None,
	EJsonError_RootIsNullptr,
	EJsonError_DataIsNullptr,
	EJsonError_InvalidCharacter,
	EJsonError_OutOfMemory,
	EJsonError_UnexpectedRootType,
	EJsonError_ExpectedEndOfBlock,
	EJsonError_ExpectedEndOfArray,
	EJsonError_ExpectedEndOfQuote,
	EJsonError_KeyStartsWithNumber,
	EJsonError_InvalidQuotelessKeyCharacter,
	EJsonError_InvalidDouble,
	EJsonError_InvalidInt,
	EJsonError_ExpectedColonCharacter,
	EJsonError_ExpectedCommaCharacter,
	EJsonError_NewLineInQuote,

	EJsonError_Unknown,  // FALLBACK ERROR TYPE
};


enum EJsonType : char
{
	EJsonType_Invalid,
	EJsonType_Object,
	EJsonType_String,
	EJsonType_Int,
	EJsonType_Double,
	// EJsonType_Bool,
	EJsonType_False,
	EJsonType_True,
	EJsonType_Null,
	EJsonType_Array,
};

struct JsonObject_t;

// TODO: use this for arrays
struct JsonArrayValue_t
{
	EJsonType aType  = EJsonType_Invalid;

	union
	{
		// JsonObject_t* apObjects;
		std::vector< JsonObject_t > aObjects{};  // REMOVE ME AND USE POINTERS - TAKES UP 32 BYTES !!!!!!!!!!!!!!!!
		// JsonArrayValue_t* apArray;
		int64_t       aInt;
		double        aDouble;
		char*         apString;
	};
};


struct JsonObject_t
{
	char*     apName = nullptr;
	EJsonType aType  = EJsonType_Invalid;

	union
	{
		// JsonObject_t* apObjects;
		std::vector< JsonObject_t > aObjects{};  // REMOVE ME AND USE POINTERS - TAKES UP 32 BYTES !!!!!!!!!!!!!!!!
		// JsonArrayValue_t* apArray;
		int64_t           aInt;
		double            aDouble;
		char*             apString;
	};

	JsonObject_t()
	{
	}

	JsonObject_t( const JsonObject_t& other )
	{
		memcpy( this, &other, sizeof( JsonObject_t ) );
	}

	~JsonObject_t()
	{
	}
};


CORE_API EJsonError  Json_Parse( JsonObject_t* spRoot, const char* spSource );
CORE_API void        Json_Free( JsonObject_t* spRoot );
CORE_API const char* Json_ErrorToStr( EJsonError sErr );

