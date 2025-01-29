#if 0

#include <memory>

#include "main.h"
#include "platform.h"
#include "core/util.h"


std::vector< UndoOperation > gUndoOperations;
size_t                       gUndoIndex;


// ---------------------------------------------------------------------


UndoOperation*               UndoSys_AddUndo()
{
	if ( gUndoOperations.size() && gUndoIndex < gUndoOperations.size() - 1 )
	{
		// clear all undo operations after this index

		return nullptr;
	}

	gUndoIndex = gUndoOperations.size() + 1;
	return &gUndoOperations.emplace_back();
}


UndoOperation* UndoSys_DoUndo()
{
	if ( gUndoIndex > gUndoOperations.size() )
	{
		// uh
		return nullptr;
	}

	auto undoOp = &gUndoOperations[ gUndoIndex - 1 ];
	if ( gUndoIndex > 0 )
		gUndoIndex--;

	return undoOp;
}


UndoOperation* UndoSys_DoRedo()
{
	if ( gUndoIndex >= gUndoOperations.size() )
	{
		// can't redo
		return nullptr;
	}

	auto undoOp = &gUndoOperations[ gUndoIndex++ ];

	return undoOp;
}


bool UndoSys_CanUndo()
{
	return gUndoOperations.size() && gUndoIndex > 0 && gUndoIndex <= gUndoOperations.size();
}


bool UndoSys_CanRedo()
{
	return gUndoOperations.size() && gUndoIndex < gUndoOperations.size();
}


bool UndoSys_Undo()
{
	UndoOperation* undo = UndoSys_DoUndo();

	if ( undo == nullptr )
	{
		printf( "Warning: Tried to undo but no actions to undo\n" );
		return false;
	}

	if ( undo->aUndoType == UndoType_Delete )
	{
		UndoData_Delete* deleteData = (UndoData_Delete*)undo->apData;
		if ( Plat_RestoreFile( deleteData->aPath ) )
			return true;
	}

	return false;
}


bool UndoSys_Redo()
{
	UndoOperation* undo = UndoSys_DoRedo();

	if ( undo == nullptr )
	{
		printf( "Warning: Tried to redo but no actions to redo\n" );
		return false;
	}

	if ( undo->aUndoType == UndoType_Delete )
	{
		UndoData_Delete* deleteData = (UndoData_Delete*)undo->apData;
		if ( Plat_DeleteFile( deleteData->aPath, false, false ) )
			return true;
	}

	return false;
}

#endif

