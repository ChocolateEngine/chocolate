#pragma once


void EditorView_Init();
void EditorView_Shutdown();
void EditorView_Update();


void Util_ComputeCameraRay( glm::vec3& srStart, glm::vec3& srDir, glm::vec2 sMousePos = { FLT_MAX, FLT_MAX }, glm::vec2 sViewportSize = { FLT_MAX, FLT_MAX } );

