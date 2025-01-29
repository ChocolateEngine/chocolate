#pragma once

extern ConVarFlag_t CVARF_INPUT;

using ButtonInput_t = int;

enum EBtnInput
{
	EBtnInput_None    = 0,

	EBtnInput_Forward = ( 1 << 1 ),
	EBtnInput_Back    = ( 1 << 2 ),
	EBtnInput_Right   = ( 1 << 3 ),
	EBtnInput_Left    = ( 1 << 4 ),

	EBtnInput_Sprint  = ( 1 << 5 ),
	EBtnInput_Duck    = ( 1 << 6 ),
	EBtnInput_Jump    = ( 1 << 7 ),
	EBtnInput_Zoom    = ( 1 << 8 ),
};


constexpr int IN_CVAR_JUST_RELEASED = -1;
constexpr int IN_CVAR_RELEASED      = 0;
constexpr int IN_CVAR_PRESSED       = 1;
constexpr int IN_CVAR_JUST_PRESSED  = 2;

