#pragma once

#include "main.h"


bool       Gizmo_Init();
void       Gizmo_Shutdown();
void       Gizmo_UpdateInputs( EditorContext_t* context, bool mouseInView );
void       Gizmo_Draw();

EGizmoMode Gizmo_GetMode();
void       Gizmo_SetMode( EGizmoMode mode );
void       Gizmo_SetMatrix( glm::mat4& mat );

