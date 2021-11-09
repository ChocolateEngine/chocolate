/*
sprite.h ( Auhored by Demez )
Declares the Sprite class
*/
#pragma once

#include "types/transform.h"
#include "graphics/renderertypes.h"
#include "graphics/imaterialsystem.h"


class Sprite: public BaseRenderable
{
public:
	virtual ~Sprite(  ) = default;

	bool 			aNoDraw = true;

	// may not be needed with scale in Transform2D?
	float			aWidth  = 0.5;
	float			aHeight = 0.5;

	std::vector< vertex_2d_t >        aVertices;

	Transform2D		aTransform;

	std::string aPath;
};
