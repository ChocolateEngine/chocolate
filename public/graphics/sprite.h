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
	using Vertices = std::vector< vertex_2d_t >;
protected:
	glm::mat4       aMatrix;
	std::string     aPath;
public:
	bool 	        aNoDraw = true;
	float	        aWidth  = 0.5;
	float	        aHeight = 0.5;
        Vertices        aVertices;
	Transform2D     aTransform;

	/* Getters.  */
	glm::mat4      &GetMatrix()                      { return aMatrix; }
	std::string    &GetPath()                        { return aPath; }
	const char     *GetCPath()                       { return aPath.c_str(); }
	int             GetPathLen()                     { return aPath.size(); }

	/* Setters.  */
	void            SetPath( const std::string &srPath )   { aPath = srPath; }
	
	virtual ~Sprite(  ) = default;
};


