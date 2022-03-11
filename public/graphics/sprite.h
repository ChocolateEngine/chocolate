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
	bool 	        aNoDraw = true;
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

	virtual VkBuffer&               GetVertexBuffer() override { return aVertexBuffer; };
	virtual VkBuffer&               GetIndexBuffer() override { return aIndexBuffer; };

	virtual VkDeviceMemory&         GetVertexBufferMem() override { return aVertexBufferMem; };
	virtual VkDeviceMemory&         GetIndexBufferMem() override { return aIndexBufferMem; };

	virtual IMaterial*              GetMaterial() override { return apMaterial; };
	virtual void                    SetMaterial( IMaterial* mat ) override { apMaterial = mat; };

	virtual inline void             SetShader( const char* name ) override { if ( apMaterial ) apMaterial->SetShader( name ); };

	virtual std::vector< vertex_2d_t >& GetVertices() { return aVertices; };
	virtual std::vector< uint32_t >& GetIndices() { return aIndices; };

private:
	IMaterial*                      apMaterial = nullptr;

	// Set the shader for the material by the shader name

	// TODO: remove this from BaseRenderable, really shouldn't have this,
	// gonna have to have some base types for this so game can use this
	// or we just store this in graphics like how we store uniform buffers
	VkBuffer                        aVertexBuffer = nullptr;
	VkBuffer                        aIndexBuffer = nullptr;
	VkDeviceMemory                  aVertexBufferMem = nullptr;
	VkDeviceMemory                  aIndexBufferMem = nullptr;

	std::vector< vertex_2d_t >		aVertices;
	std::vector< uint32_t >         aIndices;
};


