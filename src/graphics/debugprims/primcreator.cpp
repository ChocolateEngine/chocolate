/*
 *	primcreator.cpp
 *
 *	Authored by "p0ly_" on January 20, 2022
 *
 *	Defines the debug primitive class, a class to be
 *	used in the renderer to draw debug primitives
 *	
 */
#include "primcreator.h"

/* Draws a line from sX to sY until program exit.  */
void DebugRenderer::CreateLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor ) {
	aMaterials.InitLine( sX, sY, sColor );
}

/* Initializes the debug drawer.  */
void DebugRenderer::Init() {
	aMaterials.Init();
}

/* Build full debug mesh */
void DebugRenderer::ResetMesh() {
	aMaterials.ResetMesh();
}

/* Create Vertex Buffer */
void DebugRenderer::PrepareMeshForDraw() {
	aMaterials.PrepareMeshForDraw();
}
