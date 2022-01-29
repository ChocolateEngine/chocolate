/*
 *	primcreator.h
 *
 *	Authored by "p0ly_" on January 20, 2022
 *
 *	Declares the debug primitive class, a class to be
 *	used in the renderer to draw debug primitives
 *	
 */
#pragma once

#include <glm/glm.hpp>

#include "vk_prim.h"

#define VULKAN 1

class DebugRenderer {
protected:
#if VULKAN
	VulkanPrimitiveMaterials aMaterials;
#endif	/* VULKAN  */
public:
	/* Draws a line from sX to sY on current frame */
	void  CreateLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );
	/* Initializes the debug drawer.  */
	void Init();
	/* Reset the debug mesh */
	void ResetMesh();
	/* Create Vertex Buffer */
	void PrepareMeshForDraw();
};