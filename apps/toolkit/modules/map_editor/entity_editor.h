#pragma once

#include "render/irender.h"

#include "main.h"
#include "entity.h"


struct EditorRenderables
{
	ch_handle_t          gizmoTranslation;
	ch_handle_t          gizmoRotation;
	ch_handle_t          gizmoScale;

	// Base AABB's for translation axis
	AABB                baseTranslateAABB[ 3 ];
};

extern EditorRenderables gEditorRenderables;


// Entity Editor
bool                     EntEditor_Init();
void                     EntEditor_Shutdown();
void                     EntEditor_Update( float sFrameTime );
void                     EntEditor_DrawUI();

void                     EntEditor_LoadEditorRenderable( ChVector< const char* >& failList, ch_handle_t& handle, const char* path );

void                     Editor_DrawTextureInfo( TextureInfo_t& info );

// adds the entity to the selection list, making sure it's not in the list multiple times
void                     EntEditor_AddToSelection( EditorContext_t* context, ch_handle_t entity );
