#include "skybox.h"
#include "main.h"

#include "igraphics.h"
#include "mesh_builder.h"
#include "render/irender.h"
#include "core/util.h"


CONVAR_BOOL( r_skybox, 1, "Enable/Disable the Skybox" );
CONVAR_BOOL( r_skybox_ang_freeze, 0, "Lock the view angles of the Skybox" );


constexpr float         SKYBOX_SCALE = 100.0f;
constexpr glm::vec3     vec3_zero( 0, 0, 0 );

static bool             gSkyboxValid  = false;
static ch_handle_t           gSkyboxDraw   = CH_INVALID_HANDLE;
static ch_handle_t           gSkyboxModel  = CH_INVALID_HANDLE;
static ch_handle_t           gSkyboxShader = CH_INVALID_HANDLE;


void skybox_set_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	std::vector< ch_string > files = FileSys_ScanDir( "materials", ReadDir_AllPaths | ReadDir_NoDirs | ReadDir_Recursive );

	for ( const auto& file : files )
	{
		if ( ch_str_ends_with( file.data, file.size, "..", 2 ) )
			continue;

		// Make sure it's a chocolate material file
		if ( !ch_str_ends_with( file.data, file.size, ".cmt", 4 ) )
			continue;

		if ( args.size() && !ch_str_starts_with( file.data, file.size, args[ 0 ].data(), args[ 0 ].size() ) )
			continue;

		results.emplace_back( file.data, file.size );
	}

	ch_str_free( files.data(), files.size() );
}


CONCMD_DROP_VA( skybox_set, skybox_set_dropdown, 0, "Set the skybox material" )
{
	if ( args.empty() )
		return;

	Skybox_SetMaterial( args[ 0 ] );
}


bool Skybox_Init()
{
	Model* model    = nullptr;
	gSkyboxModel    = graphics->CreateModel( &model );
	gSkyboxShader   = graphics->GetShader( "skybox" );

	// create an empty material just to have for now
	// kind of an issue with this, funny
	ch_handle_t      mat = graphics->CreateMaterial( "__skybox", gSkyboxShader );

	MeshBuilder meshBuilder( graphics );
	meshBuilder.Start( model, "__skybox_model" );
	meshBuilder.SetMaterial( mat );

	// std::unordered_map< vertex_cube_3d_t, uint32_t > vertIndexes;
	//
	// std::vector< vertex_cube_3d_t >& vertices = GetVertices();
	// std::vector< uint32_t >&    indices  = GetIndices();

	auto CreateVert = [ & ]( const glm::vec3& pos )
	{
		meshBuilder.SetPos( pos * SKYBOX_SCALE );
		meshBuilder.NextVertex();
	};

	auto CreateTri = [ & ]( const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec3& pos2 )
	{
		CreateVert( pos0 );
		CreateVert( pos1 );
		CreateVert( pos2 );
	};

	// create the skybox mesh now
	// Create Bottom Face (-Z)
	CreateTri( { 1, 1, -1 }, { -1, 1, -1 }, { -1, -1, -1 } );
	CreateTri( { 1, -1, -1 }, { 1, 1, -1 }, { -1, -1, -1 } );

	// Create Top Face (+Z)
	CreateTri( { 1, 1, 1 }, { 1, -1, 1 }, { -1, -1, 1 } );
	CreateTri( { -1, 1, 1 }, { 1, 1, 1 }, { -1, -1, 1 } );

	// Create Left Face (-X)
	CreateTri( { -1, -1, -1 }, { -1, 1, -1 }, { -1, 1, 1 } );
	CreateTri( { -1, -1, 1 }, { -1, -1, -1 }, { -1, 1, 1 } );

	// Create Right Face (+X)
	CreateTri( { 1, 1, -1 }, { 1, -1, -1 }, { 1, -1, 1 } );
	CreateTri( { 1, 1, 1 }, { 1, 1, -1 }, { 1, -1, 1 } );

	// Create Back Face (+Y)
	CreateTri( { 1, 1, 1 }, { -1, 1, 1 }, { -1, 1, -1 } );
	CreateTri( { 1, 1, -1 }, { 1, 1, 1 }, { -1, 1, -1 } );

	// Create Front Face (-Y)
	CreateTri( { 1, -1, 1 }, { 1, -1, -1 }, { -1, -1, -1 } );
	CreateTri( { -1, -1, 1 }, { 1, -1, 1 }, { -1, -1, -1 } );

	meshBuilder.End();

	gSkyboxDraw = graphics->CreateRenderable( gSkyboxModel );

	if ( !gSkyboxDraw )
	{
		Log_Error( "Failed to create skybox renderable!\n" );
		return false;
	}

	if ( Renderable_t* renderable = graphics->GetRenderableData( gSkyboxDraw ) )
	{
		graphics->SetRenderableDebugName( gSkyboxDraw, "skybox" );
		renderable->aCastShadow = false;
		renderable->aTestVis    = false;
	}

	return true;
}


void Skybox_Destroy()
{
	if ( gSkyboxModel )
		graphics->FreeModel( gSkyboxModel );

	if ( gSkyboxDraw )
		graphics->FreeRenderable( gSkyboxDraw );

	gSkyboxModel = CH_INVALID_HANDLE;
	gSkyboxDraw  = CH_INVALID_HANDLE;
}


void Skybox_SetAng( const glm::vec3& srAng )
{
	if ( !gSkyboxModel && !Skybox_Init() )
		return;

	if ( !gSkyboxValid || r_skybox_ang_freeze || !graphics->Model_GetMaterial( gSkyboxModel, 0 ) )
		return;

	if ( Renderable_t* renderable = graphics->GetRenderableData( gSkyboxDraw ) )
	{
		Util_ToViewMatrixY( renderable->aModelMatrix, srAng );
		renderable->aVisible = r_skybox;
	}
}


void Skybox_SetMaterial( const std::string& srPath )
{
	if ( !gSkyboxModel && !Skybox_Init() )
	{
		Log_Error( "Failed to create skybox model\n" );
		return;
	}

	// graphics->FreeRenderable( gSkyboxDraw );
	// gSkyboxDraw    = CH_INVALID_HANDLE;
	gSkyboxValid   = false;

	Renderable_t* renderable = graphics->GetRenderableData( gSkyboxDraw );
	renderable->aVisible = false;

	ch_handle_t prevMat = graphics->Model_GetMaterial( gSkyboxModel, 0 );

	if ( prevMat )
	{
		graphics->Model_SetMaterial( gSkyboxModel, 0, CH_INVALID_HANDLE );
		graphics->FreeMaterial( prevMat );
	}

	if ( srPath.empty() )
		return;

	ch_handle_t mat = graphics->LoadMaterial( srPath.data(), srPath.size() );
	if ( mat == CH_INVALID_HANDLE )
		return;

	graphics->Model_SetMaterial( gSkyboxModel, 0, mat );

	if ( graphics->Mat_GetShader( mat ) != gSkyboxShader )
	{
		Log_WarnF( "Skybox Material is not using skybox shader: %s\n", srPath.c_str() );
		return;
	}

	// gSkyboxDraw = graphics->CreateRenderable( gSkyboxModel );
	// 
	// if ( !gSkyboxDraw )
	// {
	// 	Log_Error( "Failed to create skybox renderable!\n" );
	// 	return;
	// }

	renderable->aVisible = true;
	gSkyboxValid         = true;

	graphics->ResetRenderableMaterials( gSkyboxDraw );
}


const char* Skybox_GetMaterialName()
{
	if ( ( !gSkyboxModel && !Skybox_Init() ) || !gSkyboxValid )
		return nullptr;

	ch_handle_t mat = graphics->Model_GetMaterial( gSkyboxModel, 0 );
	if ( mat == CH_INVALID_HANDLE )
		return nullptr;

	return graphics->Mat_GetName( mat );
}


ch_handle_t Skybox_GetMaterial()
{
	if ( ( !gSkyboxModel && !Skybox_Init() ) || !gSkyboxValid )
		return CH_INVALID_HANDLE;

	ch_handle_t mat = graphics->Model_GetMaterial( gSkyboxModel, 0 );
	return mat;
}

