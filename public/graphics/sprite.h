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
protected:
	glm::mat4       aMatrix;
	std::string     aPath;
public:
	float	        aWidth  = 0.5;
	float	        aHeight = 0.5;
	Transform2D     aTransform;

	virtual ~Sprite(  ) = default;

	/* Getters.  */
	glm::mat4      &GetMatrix()                      { return aMatrix; }
	std::string    &GetPath()                        { return aPath; }
	const char     *GetCPath()                       { return aPath.c_str(); }
	int             GetPathLen()                     { return aPath.size(); }

	/* Setters.  */
	void            SetPath( const std::string &srPath )   { aPath = srPath; }


	virtual glm::mat4               GetModelMatrix() { return glm::mat4( 1.f ); };

	virtual IMaterial*              GetMaterial() override { return apMaterial; };
	virtual void                    SetMaterial( IMaterial* mat ) override { apMaterial = mat; };

	virtual inline void             SetShader( const char* name ) override { if ( apMaterial ) apMaterial->SetShader( name ); };

	virtual std::vector< vertex_2d_t >& GetVertices() { return aVertices; };
	virtual std::vector< uint32_t >& GetIndices() { return aIndices; };

private:
	IMaterial*                      apMaterial = nullptr;

	std::vector< vertex_2d_t >		aVertices;
	std::vector< uint32_t >         aIndices;
};


