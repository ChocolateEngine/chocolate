/*
imaterialsystem.h ( Authored by Demez )

Provides a public interface to the materialsystem so that the game can use this
*/

#pragma once

#include "util.h"
#include "imaterial.h"
#include "types/transform.h"


using VertexFormatFlags = u64;

// Flags to Determine what the Vertex Data contains
enum : VertexFormatFlags
{
	VertexFormat_None           = 0,
	VertexFormat_Position       = (1 << 0),
	VertexFormat_Normal         = (1 << 1),
	VertexFormat_TexCoord       = (1 << 2),
	VertexFormat_Color          = (1 << 3),

	VertexFormat_Position2D     = (1 << 4),

	// VertexFormat_BoneIndex      = (1 << 4),
	// VertexFormat_BoneWeight     = (1 << 5),
};


// Always in order from top to bottom in terms of order in each vertex
enum VertexElement
{
	VertexElement_Position,         // vec3
	VertexElement_Normal,           // vec3
	VertexElement_TexCoord,         // vec2
	VertexElement_Color,            // vec3 (should be vec4 probably)

	VertexElement_Position2D,       // vec2

	// VertexElement_BoneIndex,        // uvec4
	// VertexElement_BoneWeight,       // vec4

	VertexElement_Count = VertexElement_Position2D
};


constexpr VertexFormatFlags g_vertex_flags_3d       = VertexFormat_Position    | VertexFormat_Normal   | VertexFormat_TexCoord;
constexpr VertexFormatFlags g_vertex_flags_2d       = VertexFormat_Position2D  | VertexFormat_TexCoord;
constexpr VertexFormatFlags g_vertex_flags_cube     = VertexFormat_Position;
constexpr VertexFormatFlags g_vertex_flags_debug    = VertexFormat_Position    | VertexFormat_Color;


// could be CH_FORMAT_R32G32B32_SFLOAT
// or CH_FORMAT_RGB323232_SFLOAT
enum class ColorFormat
{
	INVALID,
	R32G32B32_SFLOAT,
	R32G32_SFLOAT,
};


// Is there a better way to do this and not directly just copy source? like bruh this seems dumb
// but it works for now
enum class ShaderMatrix
{
	View,
	Projection,
	Model,

	Count = Model,
};


struct vertex_3d_t
{
	glm::vec3 pos{};
	glm::vec3 normal{};
	glm::vec2 texCoord{};

	bool operator==( const vertex_3d_t& other ) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}

	static inline VertexFormatFlags GetFormatFlags()
	{
		return g_vertex_flags_3d;
	}
};


struct vertex_debug_t
{
	glm::vec3 pos{};
	glm::vec3 color{};

	bool operator==( const vertex_debug_t& other ) const
	{
		return pos == other.pos && color == other.color;
	}

	static inline VertexFormatFlags GetFormatFlags()
	{
		return VertexFormat_Position | VertexFormat_Color;
	}
};


// Hashing Support for these vertex formats
namespace std
{
	template<  > struct hash< vertex_3d_t >
	{
		size_t operator(  )( vertex_3d_t const& vertex ) const
		{
			return  ( ( hash< glm::vec3 >(  )( vertex.pos ) ^
				//    		( hash< glm::vec3 >(  )( vertex.color ) << 1 ) ) >> 1 ) ^
				//    		( hash< glm::vec3 >(  )( vertex.normal ) << 1 ) ) >> 1 ) ^
				( hash< glm::vec3 >(  )( vertex.normal ) << 1 ) ) ) ^
				( hash< glm::vec2 >(  )( vertex.texCoord ) << 1 );
		}
	};
	template<  > struct hash< vertex_debug_t >
	{
		size_t operator(  )( vertex_debug_t const& vertex ) const
		{
			return std::hash< glm::vec3 >{}( vertex.pos ) ^ std::hash< glm::vec3 >{}( vertex.color );
		}
	};
}


// Store the vertex format in the IMaterial class


class IRenderable : public RefCounted
{
public:
	virtual                        ~IRenderable() = default;

	virtual size_t                  GetID() { return (size_t)this; }

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t                  GetMaterialCount() = 0;

	virtual IMaterial*              GetMaterial( size_t i ) = 0;
	virtual void                    SetMaterial( size_t i, IMaterial* mat ) = 0;

	virtual void                    SetShader( size_t i, const std::string& name ) = 0;

	// --------------------------------------------------------------------------------------
	// Drawing Info

	// Get Vertex Offset and Count for drawing this part of the renderable with this material
	virtual u32                     GetVertexOffset( size_t material ) = 0;
	virtual u32                     GetVertexCount( size_t material ) = 0;

	virtual u32                     GetIndexOffset( size_t material ) = 0;
	virtual u32                     GetIndexCount( size_t material ) = 0;

	// --------------------------------------------------------------------------------------
	// Vertices/Indices Info
	// NOTE: these are stored in the material in source, so idk if im doing this right lol

	// VertexFormatFlags               aVertexFormatFlags;

	virtual VertexFormatFlags       GetVertexFormatFlags() = 0;
	virtual size_t                  GetVertexFormatSize() = 0;
	virtual void*                   GetVertexData() = 0;
	virtual size_t                  GetTotalVertexCount() = 0;

	virtual std::vector< uint32_t >&            GetIndices() = 0;
};



// Simple Model Class
class Model : public IRenderable
{
public:
	class MaterialGroup
	{
	public:
		inline void             SetShader( const std::string& name ) { if ( apMaterial ) apMaterial->SetShader( name ); };

		IMaterial*              apMaterial = nullptr;

		u32                     aVertexOffset = 0;
		u32                     aVertexCount = 0;

		// maybe use either this or above? idk
		u32                     aIndexOffset = 0;
		u32                     aIndexCount = 0;
	};

	// --------------------------------------------------------------------------------------
	// Model Data (make part of a Model class and have this be IModel?)

	std::vector< vertex_3d_t >       aVertices;
	std::vector< uint32_t >          aIndices;

	std::vector< MaterialGroup* >    aMeshes{};

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t GetMaterialCount() override
	{
		return aMeshes.size();
	}

	virtual IMaterial* GetMaterial( size_t i ) override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->apMaterial;
	}

	virtual void SetMaterial( size_t i, IMaterial* mat ) override
	{
		Assert( i < aMeshes.size() );
		aMeshes[i]->apMaterial = mat;
	}

	virtual void SetShader( size_t i, const std::string& name ) override
	{
		Assert( i < aMeshes.size() );
		aMeshes[i]->SetShader( name );
	}

	// --------------------------------------------------------------------------------------
	// Drawing Info

	// Get Vertex Offset and Count for drawing this part of the renderable with this material
	virtual u32 GetVertexOffset( size_t material ) override
	{
		Assert( material < aMeshes.size() );
		return aMeshes[material]->aVertexOffset;
	}

	virtual u32 GetVertexCount( size_t material ) override
	{
		Assert( material < aMeshes.size() );
		return aMeshes[material]->aVertexCount;
	}

	virtual u32 GetIndexOffset( size_t material ) override
	{
		Assert( material < aMeshes.size() );
		return aMeshes[material]->aIndexOffset;
	}

	virtual u32 GetIndexCount( size_t material ) override
	{
		Assert( material < aMeshes.size() );
		return aMeshes[material]->aIndexCount;
	}

	// --------------------------------------------------------------------------------------
	// Part of IModel only, i still don't know how i would handle different vertex formats
	// maybe store it in some kind of unordered_map containing these models and the vertex type?
	// but, the vertex and index type needs to be determined by the shader pipeline actually, hmm

	virtual VertexFormatFlags           GetVertexFormatFlags() override     { return vertex_3d_t::GetFormatFlags(); }
	virtual size_t                      GetVertexFormatSize() override      { return sizeof( vertex_3d_t ); }
	virtual void*                       GetVertexData() override            { return aVertices.data(); };
	virtual size_t                      GetTotalVertexCount() override      { return aVertices.size(); };

	virtual std::vector< vertex_3d_t >& GetVertices()                       { return aVertices; };
	virtual std::vector< uint32_t >&    GetIndices() override               { return aIndices; };

	// virtual std::vector< vertex_3d_t >& GetVertices() { return aVertices; };
	// virtual std::vector< uint32_t >&    GetIndices() override  { return aIndices; };

	Model() :
		aMeshes()
	{
	}

	~Model()
	{
		for ( auto mesh : aMeshes )
		{
			delete mesh;
		}
	}
};


// NOTE: this should be just a single mesh class with one material
// and the vertex format shouldn't matter because we have this other stuff
class SkyboxMesh : public IRenderable
{
public:

	IMaterial*                       apMaterial = nullptr;

	std::vector< vertex_cube_3d_t >  aVertices;
	std::vector< uint32_t >          aIndices;

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t     GetMaterialCount() override                          { return 1; }
	virtual IMaterial* GetMaterial( size_t i = 0 ) override                 { return apMaterial; }
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

	virtual VertexFormatFlags                   GetVertexFormatFlags() override     { return g_vertex_flags_cube; }
	virtual size_t                              GetVertexFormatSize() override      { return sizeof( vertex_cube_3d_t ); }
	virtual void*                               GetVertexData() override            { return aVertices.data(); };
	virtual size_t                              GetTotalVertexCount() override      { return aVertices.size(); };

	virtual std::vector< vertex_cube_3d_t >&    GetVertices()                       { return aVertices; };
	virtual std::vector< uint32_t >&            GetIndices() override               { return aIndices; };

	SkyboxMesh()  {};
	~SkyboxMesh() {}
};


// AWFUL
struct RenderableDrawData
{
	// REMOVE ME ASAP AAAAAAA
	Transform aTransform;

	// should be this only, and probably view and projection
	glm::mat4 aMatrix;
};


class Model;
class Sprite;


using RenderableBufferFlags = u32;


// TODO: use this later probably idk
enum : RenderableBufferFlags
{
	RenderableBuffer_None       = 0,
	RenderableBuffer_Vertex     = (1 << 0),
	RenderableBuffer_Index      = (1 << 1),
	RenderableBuffer_Uniform    = (1 << 2),
};


class IMaterialSystem
{
public:

	virtual ~IMaterialSystem() = default;

	/* Create a new empty material */
	virtual IMaterial*	                CreateMaterial() = 0;
	virtual IMaterial*	                CreateMaterial( const std::string& srShader ) = 0;

	/* Find an already loaded material by it's name if it exists */
	virtual IMaterial*	                FindMaterial( const std::string& name ) = 0;

	/* Delete a material */
	virtual void                        DeleteMaterial( IMaterial* mat ) = 0;

	/* Parse a Chocolate Material File (cmt) */
	virtual IMaterial*                  ParseMaterial( const std::string &path ) = 0;

	/* Get Error Material */
	virtual IMaterial*	                GetErrorMaterial( const std::string& shader ) = 0;

	/* Get Error Texture */
	virtual Texture*                    GetMissingTexture() = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a Material for a parameter, but it's needed right now
	// Creates a texture if it doesn't exist yet
	virtual Texture*                    CreateTexture( const std::string &path ) = 0;
	virtual int                         GetTextureId( Texture *spTexture ) = 0;

	/* Find an already loaded texture in vram (useless, use the above func, it will do the check for you) */
	// virtual TextureDescriptor          *FindTexture( const std::string &path ) = 0;

	// Create a Vertex and Index buffer for a Renderable.
	virtual void                        CreateVertexBuffer( IRenderable* renderable ) = 0;
	virtual void                        CreateIndexBuffer( IRenderable* renderable ) = 0;

	// Check if a Renderable has a Vertex and/or Index buffer.
	virtual bool                        HasVertexBuffer( IRenderable* renderable ) = 0;
	virtual bool                        HasIndexBuffer( IRenderable* renderable ) = 0;

	// Free a Vertex and Index buffer for a Renderable.
	virtual void                        FreeVertexBuffer( IRenderable* renderable ) = 0;
	virtual void                        FreeIndexBuffer( IRenderable* renderable ) = 0;

	virtual void                        FreeAllBuffers( IRenderable* renderable ) = 0;

	// New buffer methods
	// virtual void                        CreateBuffers( IRenderable* renderable, RenderableBufferFlags flags ) = 0;
	// virtual RenderableBufferFlags       GetBuffers( IRenderable* renderable ) = 0;
	// virtual void                        FreeBuffers( IRenderable* renderable, RenderableBufferFlags flags ) = 0;

	// Call this on renderable creation to assign it an Id
	virtual void                        RegisterRenderable( IRenderable* renderable ) = 0;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	virtual void                        DestroyRenderable( IRenderable* renderable ) = 0;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	virtual void                        AddRenderable( IRenderable* spRenderable ) = 0;
	virtual void                        AddRenderable( IRenderable* spRenderable, const RenderableDrawData& srDrawData ) = 0;

	virtual BaseShader*                 GetShader( const std::string& name ) = 0;

	// Awful Mesh Functions
	virtual void                        MeshInit( Model* mesh ) = 0;
	virtual void                        MeshReInit( Model* mesh ) = 0;
	virtual void                        MeshFreeOldResources( IRenderable* mesh ) = 0;

	virtual ColorFormat                 GetVertexElementFormat( VertexElement element ) = 0;
	virtual size_t                      GetVertexElementSize( VertexElement element ) = 0;

	// =====================================================================================
	// Material Handle Functions
};

