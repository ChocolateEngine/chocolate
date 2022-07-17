/*
imaterialsystem.h ( Authored by Demez )

Provides a public interface to the materialsystem so that the game can use this
*/

#pragma once

#include "util.h"
#include "types/transform.h"

using VertexFormat = u16;

#include "imaterial.h"

// Flags to Determine what the Vertex Data contains
enum : VertexFormat
{
	VertexFormat_None           = 0,
	VertexFormat_Position       = (1 << 0),
	VertexFormat_Normal         = (1 << 1),
	VertexFormat_TexCoord       = (1 << 2),
	VertexFormat_Color          = (1 << 3),

	VertexFormat_MorphPos       = (1 << 4),
	
	// VertexFormat_Position2D     = (1 << 5),  // uh

	VertexFormat_BoneIndex      = (1 << 5),
	VertexFormat_BoneWeight     = (1 << 6),
};


// Always in order from top to bottom in terms of order in each vertex
// technically, you could use the above only
enum VertexAttribute : u8
{
	VertexAttribute_Position,         // vec3
	VertexAttribute_Normal,           // vec3
	VertexAttribute_TexCoord,         // vec2
	VertexAttribute_Color,            // vec3 (should be vec4 probably)

	VertexAttribute_MorphPos,         // vec3

	VertexAttribute_BoneIndex,        // uvec4
	VertexAttribute_BoneWeight,       // vec4

	VertexAttribute_Count = VertexAttribute_BoneWeight
};


// could be CH_FORMAT_R32G32B32_SFLOAT
// or CH_FORMAT_RGB323232_SFLOAT
enum class GraphicsFormat
{
	INVALID,

	// -------------------------

	R8_SRGB,
	R8_SINT,
	R8_UINT,

	R8G8_SRGB,
	R8G8_SINT,
	R8G8_UINT,

	R8G8B8_SRGB,
	R8G8B8_SINT,
	R8G8B8_UINT,

	// B8G8R8_SNORM,
	// BGR888_SNORM,

	R8G8B8A8_SRGB,
	R8G8B8A8_SINT,
	R8G8B8A8_UINT,

	// -------------------------

	R16_SFLOAT,
	R16_SINT,
	R16_UINT,

	R16G16_SFLOAT,
	R16G16_SINT,
	R16G16_UINT,

	R16G16B16_SFLOAT,
	R16G16B16_SINT,
	R16G16B16_UINT,

	R16G16B16A16_SFLOAT,
	R16G16B16A16_SINT,
	R16G16B16A16_UINT,

	// -------------------------

	R32_SFLOAT,
	R32_SINT,
	R32_UINT,

	R32G32_SFLOAT,
	R32G32_SINT,
	R32G32_UINT,

	R32G32B32_SFLOAT,
	R32G32B32_SINT,
	R32G32B32_UINT,

	R32G32B32A32_SFLOAT,
	R32G32B32A32_SINT,
	R32G32B32A32_UINT,

	// -------------------------
	// GPU Compressed Formats

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,
	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,
	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,
	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,
	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,
	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,
	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,
};


enum class VertexInputRate : bool
{
	Vertex,
	Instance
};


struct VertexAttribute_t
{
	VertexFormat    aUsage      = 0;
	GraphicsFormat  aFormat     = GraphicsFormat::INVALID;
	VertexInputRate aInputRate  = VertexInputRate::Vertex;
	u32             aStride     = 0;
	u32             aOffset     = 0;
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


enum MeshPrimType
{
	MeshPrim_Point,
	MeshPrim_Line,
	MeshPrim_LineStrip,
	MeshPrim_Tri,
	MeshPrim_TriStrip,

	MeshPrim_Count = MeshPrim_LineStrip,
};


enum PipelineDynamicStateFlags
{
	DynamicState_None                   = 0,
	DynamicState_LineWidth              = (1 << 0),
	DynamicState_DepthBias              = (1 << 1),
	DynamicState_DepthBounds            = (1 << 2),
	DynamicState_BlendConstants         = (1 << 3),
	DynamicState_StencilCompareMask     = (1 << 4),
	DynamicState_StencilWriteMask       = (1 << 5),
	DynamicState_StencilReference       = (1 << 6),
};


// maybe these two are useless?
enum PolygonCullMode
{
	PolygonCull_Disabled,
	PolygonCull_Front,
	PolygonCull_Back,
};


enum PolygonFrontFace
{
	PolygonFrontFace_Clockwise,
	PolygonFrontFace_CounterClockwise,
};


// Stencil Operation
// Logic Operation
// Blend Factor
// Blend Operation


// ===================================================================


struct VertAttribData_t
{
	VertexAttribute                     aAttrib = VertexAttribute_Position;
	void*                               apData = nullptr;

	VertAttribData_t()
	{
	}

	~VertAttribData_t()
	{
		if ( apData )
			free( apData );

		// TEST
		apData = nullptr;
	}

	// REALLY should be uncommented but idc right now
// private:
// 	VertAttribData_t( const VertAttribData_t& other );
};


struct VertexData_t
{
	VertexFormat                        aFormat;
	u32                                 aCount;
	std::vector< VertAttribData_t >     aData;
	bool                                aLocked;  // if locked, we can create a vertex buffer
};


struct BlendShapeData_t
{
	std::string aName;
	float       aValue;

	// std::vector<> aData;
};


// ===================================================================


// could change to IMesh or IDrawable, idk
// NOTE: may need to go partly back to that Mesh/Model setup i had before
// because they way this Renderable is setup, if the shaders used in this renderable
// expect different vertex formats, then this will fall apart from only having shared vertex data
// we need some kind of specific vertex data or something, idk
class IModel : public RefCounted
{
public:
	virtual                            ~IModel() = default;

	inline size_t                       GetID() { return (size_t)this; }

	// --------------------------------------------------------------------------------------
	// Surfaces, only have one material
	
	virtual size_t                      GetSurfaceCount() = 0;

	// for creating new surfaces
	virtual void                        SetSurfaceCount( size_t sCount ) = 0;
	virtual void                        AddSurface() = 0;

	// NOTE: Upon setting material, recreate vertex and index buffers if the vertex format changed
	virtual IMaterial*                  GetMaterial( size_t i ) = 0;
	virtual void                        SetMaterial( size_t i, IMaterial* spMaterial ) = 0;

	virtual VertexData_t&               GetSurfaceVertexData( size_t i ) = 0;
	virtual std::vector< uint32_t >&    GetSurfaceIndices( size_t i ) = 0;

	virtual bool                        GetVertexDataLocked( size_t i ) = 0;
	virtual void                        SetVertexDataLocked( size_t i, bool sLocked ) = 0;

	// TODO: make some kind of aabb thing
	// virtual aabb_t                  GetAABB() = 0;

	// --------------------------------------------------------------------------------------
	// Blend Shapes

	virtual size_t                      GetBlendShapeCount() = 0;
	virtual void                        SetBlendShapeCount( size_t sCount ) = 0;

	virtual std::string_view            GetBlendShapeName( size_t i ) = 0;
	virtual size_t                      GetBlendShapeIndex( std::string_view sName ) = 0;

	virtual float                       GetBlendShapeValue( size_t i ) = 0;
	virtual void                        SetBlendShapeValue( size_t i, float val ) = 0;

	// --------------------------------------------------------------------------------------
	// Skeleton

	// --------------------------------------------------------------------------------------
	// Drawing Info
	// TODO: BRING THIS BACK

	// Get Vertex Offset and Count for drawing this part of the renderable with this material
	// virtual u32                        GetVertexOffset( size_t material ) = 0;
	// virtual u32                        GetVertexCount( size_t material ) = 0;
	// 								    
	// virtual u32                        GetIndexOffset( size_t material ) = 0;
	// virtual u32                        GetIndexCount( size_t material ) = 0;

	// --------------------------------------------------------------------------------------
	// Other Info

	// --------------------------------------------------------------------------------------
	// Vertices/Indices Info
	// NOTE: these are stored in the material in source, so idk if im doing this right lol

	// might become SurfaceData_t
	// VertexData_t                       aVertexData;

	// virtual void*                      GetVertexData() = 0;
	// get offset to data
	// virtual VertexData_t&              GetVertexData( VertexAttribute attrib ) = 0;

	// inline VertexData_t&               GetVertexData()      { return aVertexData; }
	// inline VertexFormat                GetVertexFormat()    { return aVertexData.aFormat; }
	// inline size_t                      GetVertexCount()     { return aVertexData.aSize; }

	// virtual std::vector< uint32_t >&            GetIndices() = 0;
};


// IDEA: create a model with specific vertex formats passed in?
// like create a model for position and color only, idk


// Simple Model Class
class Model : public IModel
{
public:
	class Surface
	{
	public:
		inline void                     SetShader( const std::string& name ) { if ( apMaterial ) apMaterial->SetShader( name ); };

		IMaterial*                      apMaterial = nullptr;

		// NOTE: these could be a pointer, and multiple surfaces could share the same vertex data and indices 
		VertexData_t                    aVertexData;
		std::vector< uint32_t >         aIndices;

		// u32                     aVertexOffset = 0;
		// u32                     aVertexCount = 0;
		// 
		// // maybe use either this or above? idk
		// u32                     aIndexOffset = 0;
		// u32                     aIndexCount = 0;
	};

	std::vector< Surface* >    aMeshes{};

	// --------------------------------------------------------------------------------------
	// Surfaces, only have one material for now

	virtual size_t GetSurfaceCount() override
	{
		return aMeshes.size();
	}

	// for creating new surfaces
	virtual void SetSurfaceCount( size_t sCount ) override
	{
		if ( sCount < aMeshes.size() )
		{
			// delete meshes after that count
			for ( size_t i = sCount; i < aMeshes.size(); i++ )
			{
				if ( aMeshes[i] )
					delete aMeshes[i];
			}

			aMeshes.resize( sCount );
		}
		else
		{
			size_t i = sCount - aMeshes.size();
			aMeshes.resize( sCount );

			for ( ; i < sCount; i++ )
				aMeshes[i] = new Surface;
		}

	}

	virtual void AddSurface() override
	{
		aMeshes.push_back( new Surface );
	}

	virtual IMaterial* GetMaterial( size_t i )  override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->apMaterial;
	}

	virtual void SetMaterial( size_t i, IMaterial* spMaterial ) override;

	virtual VertexData_t& GetSurfaceVertexData( size_t i ) override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->aVertexData;
	}

	virtual std::vector< uint32_t >& GetSurfaceIndices( size_t i ) override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->aIndices;
	}

	virtual bool GetVertexDataLocked( size_t i ) override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->aVertexData.aLocked;
	}

	virtual void SetVertexDataLocked( size_t i, bool sLocked ) override
	{
		Assert( i < aMeshes.size() );
		aMeshes[i]->aVertexData.aLocked = sLocked;
	}

	// --------------------------------------------------------------------------------------
	// Blend Shapes

	virtual size_t GetBlendShapeCount() override
	{
		return 0;
	}

	virtual void SetBlendShapeCount( size_t sCount ) override
	{
	}

	// virtual BlendShapeData_t& GetBlendShapeData( size_t i ) override
	// {
	// }

#if 1
	virtual std::string_view GetBlendShapeName( size_t i ) override
	{
		return "";
	}

	virtual size_t GetBlendShapeIndex( std::string_view sName ) override
	{
		return 0;
	}

	virtual float GetBlendShapeValue( size_t i ) override
	{
		return 0.f;
	}

	virtual void SetBlendShapeValue( size_t i, float val ) override
	{
	}
#endif

	// --------------------------------------------------------------------------------------
	// Materials

#if 0
	virtual size_t GetMaterialCount() override
	{
		return aMeshes.size();
	}

	virtual IMaterial* GetMaterial( size_t i ) override
	{
		Assert( i < aMeshes.size() );
		return aMeshes[i]->apMaterial;
	}

	virtual void SetMaterial( size_t i, IMaterial* spMaterial ) override
	{
		Assert( i < aMeshes.size() );
		aMeshes[i]->apMaterial = spMaterial;
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
#endif

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



// ===================================================================
// New IRenderable idea, rename the one above to IModel or IMesh
// 
// ch_graphics.dll will call these functions when drawing
// in game code, you can calculate everything before hand
// might work well for threading as well since it's only going to read already calculated data, idk
// 
// but what about for something like an eye look shader?
// where a parameter of this could be eye look position or a direction, a vec3
// or a cubemap for a model (could be a graphics dll thing technically)
// you could material var it, but i don't like that too much, oh well
// ===================================================================
class IRenderable : public RefCounted
{
public:
	virtual IModel*                         GetModel() = 0;
	virtual const glm::mat4&                GetModelMatrix() = 0;
	virtual const std::vector< float >&     GetMorphWeights() = 0;
	// virtual void                            GetBoneWeights() = 0;
};


class DefaultRenderable : public IRenderable
{
public:
	virtual IModel*                         GetModel() override
	{
		return apModel;
	}

	virtual const glm::mat4&                GetModelMatrix() override
	{
		return aMatrix;
	}

	virtual const std::vector< float >&     GetMorphWeights() override
	{
		return aMorphWeights;
	}

	IModel*                 apModel = nullptr;
	glm::mat4               aMatrix{};
	std::vector< float >	aMorphWeights;
};


using RenderableHandle_t = size_t;


// ===================================================================


#if 0
struct Viewport_t
{
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};
#endif


// ===================================================================


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
	virtual void                        CreateVertexBuffer( IModel* renderable, size_t surface ) = 0;
	virtual void                        CreateIndexBuffer( IModel* renderable, size_t surface ) = 0;

	virtual void                        CreateVertexBuffers( IModel* renderable ) = 0;
	virtual void                        CreateIndexBuffers( IModel* renderable ) = 0;

	// Check if a Renderable has a Vertex and/or Index buffer.
	virtual bool                        HasVertexBuffer( IModel* renderable, size_t surface ) = 0;
	virtual bool                        HasIndexBuffer( IModel* renderable, size_t surface ) = 0;

	// Free a Vertex and Index buffer for a Renderable.
	virtual void                        FreeVertexBuffer( IModel* renderable, size_t surface ) = 0;
	virtual void                        FreeIndexBuffer( IModel* renderable, size_t surface ) = 0;

	virtual void                        FreeVertexBuffers( IModel* renderable ) = 0;
	virtual void                        FreeIndexBuffers( IModel* renderable ) = 0;

	virtual void                        FreeAllBuffers( IModel* renderable ) = 0;

	virtual void                        InitUniformBuffer( IModel* renderable ) = 0;

	// New buffer methods
	// virtual void                        CreateBuffers( IModel* renderable, RenderableBufferFlags flags ) = 0;
	// virtual RenderableBufferFlags       GetBuffers( IModel* renderable ) = 0;
	// virtual void                        FreeBuffers( IModel* renderable, RenderableBufferFlags flags ) = 0;

	// Create a renderable to add components to control how this model will be drawn
	// virtual Handle                      CreateRenderable( IModel* model ) = 0;

	// virtual Handle                      DestroyRenderable( IModel* model ) = 0;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	virtual void                        DestroyModel( IModel* renderable ) = 0;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	virtual void                        AddRenderable( IRenderable* spRenderable ) = 0;

	virtual BaseShader*                 GetShader( const std::string& name ) = 0;

	// Awful Mesh Functions
	virtual void                        MeshFreeOldResources( IModel* mesh ) = 0;

	virtual GraphicsFormat              GetVertexAttributeFormat( VertexAttribute element ) = 0;
	virtual size_t                      GetVertexAttributeTypeSize( VertexAttribute element ) = 0;
	virtual size_t                      GetVertexAttributeSize( VertexAttribute element ) = 0;
	virtual size_t                      GetVertexFormatSize( VertexFormat format ) = 0;

	// virtual size_t                      GetFormatSize( DataFormat format ) = 0;

	// calls vkCmdSetViewport? but we would need to call this during a render pass
	// which we have no access to currently in the game code
	// virtual void                     SetViewport( const Viewport_t& viewport );
	// virtual const Viewport_t&        GetViewport();

	// =====================================================================================
	// Material Handle Functions
};


inline void Model::SetMaterial( size_t i, IMaterial* spMaterial )
{
	Assert( i < aMeshes.size() );

	if ( aMeshes[i]->apMaterial == spMaterial )
		return;

	if ( aMeshes[i]->apMaterial == nullptr )
	{
		aMeshes[i]->apMaterial = spMaterial;

		if ( aMeshes[i]->aVertexData.aLocked )
		{
			auto matSys = spMaterial->GetMaterialSystem();
			matSys->CreateVertexBuffer( this, i );
		}

		return;
	}

	// NOTE: THIS NEEDS TO BE RE-THOUGHT !!!
	// spMaterial could be nullptr and apMaterial could of been deleted earlier
	// so there's no way to get the material system, aaaa

	if ( aMeshes[i]->aVertexData.aLocked )
	{
		if ( spMaterial == nullptr )
		{
			auto matSys = aMeshes[i]->apMaterial->GetMaterialSystem();

			if ( matSys->HasVertexBuffer( this, i ) )
				matSys->FreeVertexBuffer( this, i );
		}

		// Is the vertex format between the two different?
		else if ( aMeshes[i]->apMaterial->GetVertexFormat() != spMaterial->GetVertexFormat() )
		{
			auto matSys = spMaterial->GetMaterialSystem();

			// Only Recreate this vertex buffer if we already have one
			if ( matSys->HasVertexBuffer( this, i ) )
			{
				matSys->FreeVertexBuffer( this, i );
				matSys->CreateVertexBuffer( this, i );
			}
		}
	}

	aMeshes[i]->apMaterial = spMaterial;
}


