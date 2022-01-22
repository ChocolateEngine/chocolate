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
Line *DebugRenderer::CreateLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) {
	return aMaterials.InitLine( sX, sY, sColor );
}

/* Clears a line from the list of lines.  */
void DebugRenderer::DestroyLine( Line *spLine ) {
	aMaterials.RemoveLine( spLine );
}

/* Initializes the debug drawer.  */
void DebugRenderer::Init() {
	aMaterials.Init();
}

/* Draws all the loaded primitives.  */
void DebugRenderer::RenderPrims( VkCommandBuffer c, View v ) {
	aMaterials.DrawPrimitives( c, v );
}
