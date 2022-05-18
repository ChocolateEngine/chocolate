/*
sprite.h ( Auhored by Demez )

Declares the Sprite class
*/
#pragma once

#include "types/transform.h"
#include "graphics/renderertypes.h"
#include "graphics/imaterialsystem.h"


#if 0
class Sprite: public IRenderable
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

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t     GetMaterialCount() override                          { return 1; }
	virtual IMaterial* GetMaterial( size_t i ) override                     { return apMaterial; }
	virtual void       SetMaterial( size_t i, IMaterial* mat ) override     { apMaterial = mat; }

	virtual void SetShader( size_t i, const std::string& name ) override
	{
		if ( apMaterial )
			apMaterial->SetShader( name );
	}

	// --------------------------------------------------------------------------------------
	// Drawing Info

	virtual u32 GetVertexOffset( size_t material ) override     { return 0; }
	virtual u32 GetVertexCount( size_t material ) override      { return aVertices.size(); }

	virtual u32 GetIndexOffset( size_t material ) override      { return 0; }
	virtual u32 GetIndexCount( size_t material ) override       { return aIndices.size(); }

	// --------------------------------------------------------------------------------------
	// Part of IModel only, i still don't know how i would handle different vertex formats
	// maybe store it in some kind of unordered_map containing these models and the vertex type?
	// but, the vertex and index type needs to be determined by the shader pipeline actually, hmm

	virtual VertexFormatFlags                   GetVertexFormatFlags() override     { return g_vertex_flags_2d; }
	virtual size_t                              GetVertexFormatSize() override      { return sizeof( vertex_2d_t ); }
	virtual void*                               GetVertexData() override            { return aVertices.data(); };
	virtual size_t                              GetTotalVertexCount() override      { return aVertices.size(); };

	virtual std::vector< vertex_2d_t >&         GetVertices()                       { return aVertices; };
	virtual std::vector< uint32_t >&            GetIndices() override               { return aIndices; };

private:
	IMaterial*                      apMaterial = nullptr;

	std::vector< vertex_2d_t >		aVertices;
	std::vector< uint32_t >         aIndices;
};
#endif


