#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


static std::unordered_map< std::string_view, ch_handle_t >                  gShaderNames;
static std::unordered_map< std::string_view, ShaderSets_t >            gShaderSets;  // [shader name] = descriptor sets for this shader

static std::unordered_map< ch_handle_t, EPipelineBindPoint >                gShaderBindPoint;   // [shader] = bind point
static std::unordered_map< ch_handle_t, VertexFormat >                      gShaderVertFormat;  // [shader] = vertex format
static std::unordered_map< ch_handle_t, FShader_Destroy* >                  gShaderDestroy;     // [shader] = shader destroy function
static std::unordered_map< ch_handle_t, ShaderData_t >                      gShaderData;        // [shader] = assorted shader data

// descriptor set layouts
// extern ShaderBufferArray_t                              gUniformSampler;
// extern ShaderBufferArray_t                              gUniformViewInfo;
// extern ShaderBufferArray_t                              gUniformMaterialBasic3D;
// extern ShaderBufferArray_t                              gUniformLights;

//static std::unordered_map< SurfaceDraw_t, void* >                      gShaderPushData;

static ChVector< ch_handle_t >                                          gShaderGraphics;
static ChVector< ch_handle_t >                                          gShaderCompute;


// Shader = List of Materials using that shader
// static std::unordered_map< ch_handle_t, ChVector< ShaderMaterialData > > gShaderMaterials;
// static std::unordered_map< ch_handle_t, std::vector< ShaderMaterialData > > gShaderMaterials;

// Shader = [Materials = Shader Material Data]
static std::unordered_map< ch_handle_t, std::unordered_map< ch_handle_t, ShaderMaterialData > > gShaderMaterials;

// Material = Storage Buffer
static std::unordered_map< ch_handle_t, DeviceBufferStaging_t >          gMaterialBuffers;


// shader
// list of materials using shader
// data for each material setup for that shader


CONCMD( shader_reload )
{
	render->WaitForQueues();

	Graphics_ShaderInit( true );
}


CONCMD( shader_dump )
{
	Log_MsgF( gLC_ClientGraphics, "Shader Count: %zd\n", gShaderNames.size() );

	for ( const auto& [name, shader] : gShaderNames )
	{
		auto it = gShaderData.find( shader );
		if ( it == gShaderData.end() )
		{
			Log_MsgF( gLC_ClientGraphics, "Found Invalid Shader: %s\n", name.data() );
			continue;
		}

		std::string type;

		if ( it->second.aStages & ShaderStage_Vertex )
			type += " | Vertex";

		if ( it->second.aStages & ShaderStage_Fragment )
			type += " | Fragment";

		if ( it->second.aStages & ShaderStage_Compute )
			type += " | Compute";

		Log_MsgF( gLC_ClientGraphics, "  %s%s\n", name.data(), type.data() );
	}

	Log_Msg( gLC_ClientGraphics, "\n" );
}


// --------------------------------------------------------------------------------------


std::vector< ShaderCreate_t* >& Shader_GetCreateList()
{
	static std::vector< ShaderCreate_t* > create;
	return create;
}


ch_handle_t Graphics::GetShader( const char* sName )
{
	// address sanitizer crashes when you have the input be std::string_view for some reason
	std::string_view temp = sName;
	auto it = gShaderNames.find( temp );
	if ( it != gShaderNames.end() )
		return it->second;

	Log_ErrorF( gLC_ClientGraphics, "Graphics_GetShader: Shader not found: %s\n", sName );
	return CH_INVALID_HANDLE;
}


const char* Graphics::GetShaderName( ch_handle_t sShader )
{
	for ( const auto& [name, shader] : gShaderNames )
	{
		if ( shader == sShader )
		{
			return name.data();
		}
	}

	Log_ErrorF( gLC_ClientGraphics, "Graphics_GetShader: Shader not found: %zd\n", sShader );
	return nullptr;
}


u32 Graphics::GetShaderCount()
{
	return gShaderNames.size();
}


ch_handle_t Graphics::GetShaderByIndex( u32 sIndex )
{
	u32 i = 0;
	for ( const auto& [ name, shader ] : gShaderNames )
	{
		if ( sIndex != i )
		{
			i++;
			continue;
		}

		return shader;
	}

	return CH_INVALID_HANDLE;
}


u32 Graphics::GetGraphicsShaderCount()
{
	return gShaderGraphics.size();
}


ch_handle_t Graphics::GetGraphicsShaderByIndex( u32 sIndex )
{
	if ( sIndex >= gShaderGraphics.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Invalid Graphics Shader Index: %d, only have %d graphics shaders\n", sIndex, gShaderGraphics.size() );
		return CH_INVALID_HANDLE;
	}

	return gShaderGraphics[ sIndex ];
}


u32 Graphics::GetComputeShaderCount()
{
	return gShaderCompute.size();
}


ch_handle_t Graphics::GetComputeShaderByIndex( u32 sIndex )
{
	if ( sIndex >= gShaderCompute.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Invalid Compute Shader Index: %d, only have %d compute shaders\n", sIndex, gShaderCompute.size() );
		return CH_INVALID_HANDLE;
	}

	return gShaderCompute[ sIndex ];
}


u32 Graphics::GetShaderVarCount( ch_handle_t shader )
{
	ShaderData_t* data = Shader_GetData( shader );

	if ( data == nullptr )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Get Shader Data for Shader Material Var Count\n" );
		return 0;
	}

	return data->aMaterialVarCount;
}


ShaderMaterialVarDesc* Graphics::GetShaderVars( ch_handle_t shader )
{
	ShaderData_t* data = Shader_GetData( shader );

	if ( data == nullptr )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Get Shader Data for Shader Material Vars\n" );
		return nullptr;
	}

	return data->apMaterialVars;
}


bool Shader_CreatePipelineLayout( std::string_view sName, ch_handle_t& srLayout, FShader_GetPipelineLayoutCreate fCreate )
{
	if ( fCreate == nullptr )
	{
		Log_Error( gLC_ClientGraphics, "FShader_GetPipelineLayoutCreate is nullptr!\n" );
		return false;
	}

	PipelineLayoutCreate_t pipelineCreate{};
	pipelineCreate.aLayouts.reserve( pipelineCreate.aLayouts.capacity() + EShaderSlot_Count );
	pipelineCreate.aLayouts.push_back( gShaderDescriptorData.aGlobalLayout );

	auto find = gShaderDescriptorData.aPerShaderLayout.find( sName );
	if ( find == gShaderDescriptorData.aPerShaderLayout.end() )
	{
		Log_ErrorF( gLC_ClientGraphics, "No Shader Descriptor Set Layout made for shader \"%s\"\n", sName.data() );
		return false;
	}

	pipelineCreate.aLayouts.push_back( find->second );

	fCreate( pipelineCreate );

	if ( !render->CreatePipelineLayout( srLayout, pipelineCreate ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to create Pipeline Layout\n" );
		return false;
	}
	
	return true;
}


bool Shader_CreateGraphicsPipeline( ShaderCreate_t& srCreate, ch_handle_t& srPipeline, ch_handle_t& srLayout, ch_handle_t sRenderPass )
{
	if ( srCreate.apGraphicsCreate == nullptr )
	{
		Log_Error( gLC_ClientGraphics, "FShader_GetGraphicsPipelineCreate is nullptr!\n" );
		return false;
	}

	GraphicsPipelineCreate_t pipelineCreate{};
	srCreate.apGraphicsCreate( pipelineCreate );

	pipelineCreate.aRenderPass     = sRenderPass;
	pipelineCreate.apName          = srCreate.apName;
	pipelineCreate.aPipelineLayout = srLayout;

	if ( !render->CreateGraphicsPipeline( srPipeline, pipelineCreate ) )
	{
		Log_ErrorF( "Failed to create Graphics Pipeline for Shader \"%s\"\n", srCreate.apName );
		return false;
	}

	return true;
}


bool Shader_CreateComputePipeline( ShaderCreate_t& srCreate, ch_handle_t& srPipeline, ch_handle_t& srLayout, ch_handle_t sRenderPass )
{
	if ( srCreate.apComputeCreate == nullptr )
	{
		Log_Error( gLC_ClientGraphics, "FShader_GetComputePipelineCreate is nullptr!\n" );
		return false;
	}

	ComputePipelineCreate_t pipelineCreate{};
	srCreate.apComputeCreate( pipelineCreate );

	pipelineCreate.apName          = srCreate.apName;
	pipelineCreate.aPipelineLayout = srLayout;

	if ( !render->CreateComputePipeline( srPipeline, pipelineCreate ) )
	{
		Log_ErrorF( "Failed to create Compute Pipeline for Shader \"%s\"\n", srCreate.apName );
		return false;
	}

	return true;
}


bool Graphics_CreateShader( bool sRecreate, ch_handle_t sRenderPass, ShaderCreate_t& srCreate )
{
	ch_handle_t pipeline = CH_INVALID_HANDLE;

	auto   nameFind = gShaderNames.find( srCreate.apName );
	if ( nameFind != gShaderNames.end() )
		pipeline = nameFind->second;

	ShaderData_t shaderData{};

	if ( pipeline )
	{
		auto it = gShaderData.find( pipeline );
		if ( it != gShaderData.end() )
		{
			shaderData = it->second;
		}
	}

	if ( !Shader_CreatePipelineLayout( srCreate.apName, shaderData.aLayout, srCreate.apLayoutCreate ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to create Pipeline Layout\n" );
		return false;
	}

	if ( srCreate.aBindPoint == EPipelineBindPoint_Graphics )
	{
		if ( !Shader_CreateGraphicsPipeline( srCreate, pipeline, shaderData.aLayout, sRenderPass ) )
		{
			Log_Error( gLC_ClientGraphics, "Failed to create Graphics Pipeline\n" );
			return false;
		}

		if ( !sRecreate )
			gShaderGraphics.push_back( pipeline );
	}
	else
	{
		if ( !Shader_CreateComputePipeline( srCreate, pipeline, shaderData.aLayout, sRenderPass ) )
		{
			Log_Error( gLC_ClientGraphics, "Failed to create Compute Pipeline\n" );
			return false;
		}

		if ( !sRecreate )
			gShaderCompute.push_back( pipeline );
	}

	gShaderNames[ srCreate.apName ] = pipeline;
	gShaderBindPoint[ pipeline ]    = srCreate.aBindPoint;
	gShaderVertFormat[ pipeline ]   = srCreate.aVertexFormat;

	shaderData.aFlags               = srCreate.aFlags;
	shaderData.aStages              = srCreate.aStages;
	shaderData.aDynamicState        = srCreate.aDynamicState;

	shaderData.apBindings           = srCreate.apBindings;
	shaderData.aBindingCount        = srCreate.aBindingCount;

	// shaderData.aPushSize            = srCreate.aPushSize;
	// shaderData.apPushSetup          = srCreate.apPushSetup;

	shaderData.aMaterialBufferBinding = srCreate.aMaterialBufferBinding;
	shaderData.aMaterialSize        = srCreate.aMaterialSize;
	shaderData.aMaterialVarCount    = srCreate.aMaterialVarCount;
	shaderData.apMaterialVars       = srCreate.apMaterialVars;
	shaderData.aUseMaterialBuffer   = srCreate.aUseMaterialBuffer;

	if ( srCreate.apShaderPush )
		shaderData.apPush = srCreate.apShaderPush;

	if ( srCreate.apShaderPushComp )
		shaderData.apPushComp = srCreate.apShaderPushComp;

	gShaderData[ pipeline ] = shaderData;

	// check for any default textures
	for ( u32 varI = 0; varI < shaderData.aMaterialVarCount; varI++ )
	{
		ShaderMaterialVarDesc& desc = shaderData.apMaterialVars[ varI ];

		if ( desc.type == EMatVar_Texture )
		{
			// TODO: probably expose this in the shader material var descriptors?
			TextureCreateData_t createData{};
			createData.aUsage = EImageUsage_Sampled;

			render->LoadTexture( desc.defaultTextureHandle, desc.defaultTexture, createData );
		}
	}

	if ( shaderData.aMaterialVarCount )
	{
		gShaderMaterials[ pipeline ];
	}

	if ( !sRecreate )
	{
		if ( srCreate.apDestroy )
			gShaderDestroy[ pipeline ] = srCreate.apDestroy;

		if ( srCreate.apInit )
			return srCreate.apInit();
	}

	return true;
}


void Shader_Destroy( ch_handle_t sShader )
{
	auto it = gShaderData.find( sShader );
	if ( it != gShaderData.end() )
	{
		Log_WarnF( gLC_ClientGraphics, "Shader_Destroy: Failed to find shader: %zd\n", sShader );
		return;
	}

	ShaderData_t& shaderData = it->second;

	// check for any default textures
	for ( u32 varI = 0; varI < shaderData.aMaterialVarCount; varI++ )
	{
		ShaderMaterialVarDesc& desc = shaderData.apMaterialVars[ varI ];

		if ( desc.type == EMatVar_Texture )
		{
			render->FreeTexture( desc.defaultTextureHandle );
		}
	}

	gShaderMaterials.erase( sShader );

	render->DestroyPipelineLayout( shaderData.aLayout );
	render->DestroyPipeline( sShader );

	for ( const auto& [ name, shader ] : gShaderNames )
	{
		if ( shader == sShader )
		{
			gShaderNames.erase( name );
			break;
		}
	}

	gShaderBindPoint.erase( sShader );
	gShaderVertFormat.erase( sShader );
	gShaderData.erase( sShader );
}


bool Graphics_ShaderInit( bool sRecreate )
{
	for ( ShaderCreate_t* shaderCreate : Shader_GetCreateList() )
	{
		ch_handle_t renderPass = gGraphicsData.aRenderPassGraphics;

		if ( shaderCreate->aRenderPass == ERenderPass_Shadow )
			renderPass = gGraphicsData.aRenderPassShadow;

		if ( shaderCreate->aRenderPass == ERenderPass_Select )
			renderPass = gGraphicsData.aRenderPassSelect;

		if ( !Graphics_CreateShader( sRecreate, renderPass, *shaderCreate ) )
		{
			Log_ErrorF( gLC_ClientGraphics, "Failed to create shader \"%s\"\n", shaderCreate->apName );
			return false;
		}
	}

	return true;
}


void Graphics_ShaderDestroy()
{
	Log_Error( gLC_ClientGraphics, "TODO: Delete Shaders!!!!!\n" );
}


EPipelineBindPoint Shader_GetPipelineBindPoint( ch_handle_t sShader )
{
	PROF_SCOPE();

	auto it = gShaderBindPoint.find( sShader );
	if ( it != gShaderBindPoint.end() )
		return it->second;
	
	Log_Error( gLC_ClientGraphics, "Unable to find Shader Pipeline Bind Point!\n" );
	return EPipelineBindPoint_Graphics;
}


ShaderData_t* Shader_GetData( ch_handle_t sShader )
{
	PROF_SCOPE();

	auto it = gShaderData.find( sShader );
	if ( it != gShaderData.end() )
		return &it->second;
	
	Log_Error( gLC_ClientGraphics, "Unable to find Shader Data!\n" );
	return nullptr;
}


bool Shader_ParseRequirements( ShaderRequirmentsList_t& srOutput )
{
	for ( ShaderCreate_t* shaderCreate : Shader_GetCreateList() )
	{
		// create the per shader descriptor sets for this shader
		ShaderSets_t& shaderSets = gShaderSets[ shaderCreate->apName ];

		// future demez here - what the hell does this do and what's the purpose of this?
		if ( shaderCreate->aBindingCount > 0 )
		{
			for ( u32 i = 0; i < shaderCreate->aBindingCount; i++ )
			{
				ShaderRequirement_t require{};
				require.aShader       = shaderCreate->apName;
				require.apBindings    = shaderCreate->apBindings;
				require.aBindingCount = shaderCreate->aBindingCount;

				srOutput.aItems.push_back( require );
			}
		}
		else
		{
			ShaderRequirement_t require{};
			require.aShader       = shaderCreate->apName;
			require.apBindings    = shaderCreate->apBindings;
			require.aBindingCount = shaderCreate->aBindingCount;

			srOutput.aItems.push_back( require );
		}
	}

	return true;
}


// ch_handle_t Shader_RegisterDescriptorData( EShaderSlot sSlot, FShader_DescriptorData* sCallback )
// {
// 
// }


bool Shader_Bind( ch_handle_t sCmd, u32 sIndex, ch_handle_t sShader )
{
	PROF_SCOPE();

	ShaderData_t* shaderData = Shader_GetData( sShader );
	if ( !shaderData )
		return false;

	if ( !render->CmdBindPipeline( sCmd, sShader ) )
		return false;

	// Bind Descriptor Sets (TODO: Keep track of what is currently bound so we don't need to rebind set 0)
	ChVector< ch_handle_t > descSets;
	descSets.reserve( 2 );

	// TODO: Should only be done once per frame
	descSets.push_back( gShaderDescriptorData.aGlobalSets.apSets[ sIndex ] );

	// AAAA
	std::string_view shaderName = gGraphics.GetShaderName( sShader );
	descSets.push_back( gShaderDescriptorData.aPerShaderSets[ shaderName ].apSets[ sIndex ] );

	if ( descSets.size() )
	{
		EPipelineBindPoint bindPoint = Shader_GetPipelineBindPoint( sShader );
		render->CmdBindDescriptorSets( sCmd, sIndex, bindPoint, shaderData->aLayout, descSets.data(), descSets.size() );
	}

	return true;
}


bool Shader_PreMaterialDraw( ch_handle_t sCmd, u32 sIndex, ShaderData_t* spShaderData, ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	if ( !sPushData.apRenderable )
		return false;

	if ( sPushData.aViewportIndex == UINT32_MAX )
		return false;

	if ( spShaderData->apPush )
	{
		spShaderData->apPush( sCmd, spShaderData->aLayout, sPushData );
	}

	return true;
}


VertexFormat Shader_GetVertexFormat( ch_handle_t sShader )
{
	return VertexFormat_All;

#if 0
	PROF_SCOPE();

	auto it = gShaderVertFormat.find( sShader );

	if ( it == gShaderVertFormat.end() )
	{
		Log_Error( gLC_ClientGraphics, "Failed to find vertex format for shader!\n" );
		return VertexFormat_None;
	}

	return it->second;
#endif
}


// BAD
// gets the total count of material buffers for this shader
//ChVector< DeviceBufferStaging_t* > Shader_GetMaterialBufferCount( ch_handle_t sShader, ShaderData_t* sShaderData )
//{
//	i = 0;
//	for ( const auto& [ mat, buffer ] : gMaterialBuffers )
//	{
//		gMaterialBufferIndex[ mat ]          = i;
//		update.apBindings[ 0 ].apData[ i++ ] = buffer;
//	}
//}


void Shader_UpdateMaterialDescriptorSets( ch_handle_t shader, ShaderData_t* shaderData, std::unordered_map< ch_handle_t, ShaderMaterialData >& shaderMatDataList )
{
	// can't update without any material buffers
	if ( shaderMatDataList.size() == 0 )
		return;

	CH_ASSERT_MSG( shaderData->aBindingCount != 0, "Shader has no bindings, but we need one for materials!" );

	const char* shaderName = gGraphics.GetShaderName( shader );

	// update the descriptor sets
	WriteDescSet_t update{};

	update.aDescSetCount = gShaderDescriptorData.aPerShaderSets[ shaderName ].aCount;
	update.apDescSets    = gShaderDescriptorData.aPerShaderSets[ shaderName ].apSets;

	// update.aBindingCount = CH_ARR_SIZE( gBasic3D_Bindings );
	update.aBindingCount = shaderData->aBindingCount;
	update.apBindings    = ch_calloc< WriteDescSetBinding_t >( update.aBindingCount );

	size_t i             = 0;
	for ( size_t binding = 0; binding < update.aBindingCount; binding++ )
	{
		update.apBindings[ i ].aBinding = shaderData->apBindings[ binding ].aBinding;
		update.apBindings[ i ].aType    = shaderData->apBindings[ binding ].aType;
		update.apBindings[ i ].aCount   = shaderData->apBindings[ binding ].aCount;
		i++;
	}

	// GOD - update material indexes
	u32 matIndex = 0;
	for ( auto& [ material, shaderMat ] : shaderMatDataList )
	{
		shaderMat.matIndex = matIndex++;
	}

	// update.apBindings[ shaderData->aMaterialBufferBinding ].apData = ch_calloc< ch_handle_t >( CH_BASIC3D_MAX_MATERIALS );
	update.apBindings[ shaderData->aMaterialBufferBinding ].apData = ch_calloc< ch_handle_t >( shaderMatDataList.size() );
	update.apBindings[ shaderData->aMaterialBufferBinding ].aCount = shaderMatDataList.size();

	i                                                            = 0;

	// for ( ch_handle_t buffer : bufferList )
	// {
	// 	update.apBindings[ shaderData->aMaterialBufferBinding ].apData[ i++ ] = buffer;
	// }

	for ( const auto& [ mat, matBuffer ] : gMaterialBuffers )
	{
		if ( gGraphics.Mat_GetShader( mat ) != shader )
			continue;

		auto it = shaderMatDataList.find( mat );
		if ( it == shaderMatDataList.end() )
		{
			Log_Error( "Failed to find material in use by shader\n" );
			continue;
		}

		update.apBindings[ shaderData->aMaterialBufferBinding ].apData[ it->second.matIndex ] = matBuffer.aBuffer;

		// update.apBindings[ shaderData->aMaterialBufferBinding ].apData[ i++ ] = matBuffer.aBuffer;
	}

	// update.aImages = gViewportBuffers;
	render->UpdateDescSets( &update, 1 );

	free( update.apBindings[ shaderData->aMaterialBufferBinding ].apData );
	free( update.apBindings );
}


void Shader_RemoveMaterial( ch_handle_t sMat )
{
	// TODO: queue this

	ch_handle_t shader = gGraphics.Mat_GetShader( sMat );

	if ( shader == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "Material does not have shader!\n" );
		return;
	}

	auto shaderIt = gShaderMaterials.find( shader );

	if ( shaderIt == gShaderMaterials.end() )
	{
		// shader does not use materials probably
		return;
	}

	ShaderData_t*                                         shaderData   = Shader_GetData( shader );
	std::unordered_map< ch_handle_t, ShaderMaterialData >& matData      = shaderIt->second;
	bool                                                  foundMatData = false;

	auto matIt = matData.find( sMat );

	if ( matIt != matData.end() )
	{
		matData.erase( matIt );
		foundMatData = true;
	}

	// Update Descriptor Sets after removing the material data
	if ( foundMatData && shaderData->aUseMaterialBuffer )
	{
		auto it = gMaterialBuffers.find( sMat );

		if ( it == gMaterialBuffers.end() )
		{
			const char* matName = gGraphics.Mat_GetName( sMat );
			Log_ErrorF( "Failed to find buffer to free for material %s\n", matName );
		}
		else
		{
			Graphics_FreeStagingBuffer( it->second );
			gMaterialBuffers.erase( it );
		}

		Shader_UpdateMaterialDescriptorSets( shader, shaderData, matData );
	}

	if ( !foundMatData )
		Log_Error( gLC_ClientGraphics, "Failed to find material in use by shader\n" );
}


void Shader_AddMaterial( ch_handle_t sMat )
{
	// TODO: queue this

	ch_handle_t shader = gGraphics.Mat_GetShader( sMat );

	if ( shader == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "Material does not have shader!\n" );
		return;
	}

	auto shaderIt = gShaderMaterials.find( shader );

	if ( shaderIt == gShaderMaterials.end() )
	{
		// shader does not use materials probably
		return;
	}

	ShaderMaterialData data = { sMat };
	
	// TODO: find shader data, and write vars
	ShaderData_t*      shaderData = Shader_GetData( shader );

	// Shaders might use this material data in push constants, so add it even if the shader doesn't use material buffers
	shaderIt->second[ sMat ] = data;

	if ( shaderData->aUseMaterialBuffer )
	{
		const char*            matName    = gGraphics.Mat_GetName( sMat );
		DeviceBufferStaging_t& buffer     = gMaterialBuffers[ sMat ];

		if ( !Graphics_CreateStagingBuffer( buffer, shaderData->aMaterialSize, matName, matName ) )
		{
			Log_FatalF( "Failed to create buffer for material %s\n", matName );
		}

		Shader_UpdateMaterialDescriptorSets( shader, shaderData, shaderIt->second );
	}
}


void Shader_WriteMaterialBuffer( ch_handle_t mat, ch_handle_t shader, ShaderData_t* shaderData, ShaderMaterialData* materialData )
{
	auto bufIt = gMaterialBuffers.find( mat );

	if ( bufIt == gMaterialBuffers.end() )
		return;

	// Prepare New Data
	char* writeData = (char*)calloc( 1, shaderData->aMaterialSize );

	if ( writeData == nullptr )
	{
		Log_Error( "Failed to allocate memory for material buffer write\n" );
		return;
	}

	for ( u32 varI = 0; varI < shaderData->aMaterialVarCount; varI++ )
	{
		ShaderMaterialVarDesc& desc = shaderData->apMaterialVars[ varI ];

		switch ( desc.type )
		{
			case EMatVar_Texture:
			{
				int texIndex = render->GetTextureIndex( materialData->vars[ varI ].aTexture );
				memcpy( writeData + desc.dataOffset, &texIndex, desc.dataSize );
				break;
			}
			case EMatVar_Float:
			{
				memcpy( writeData + desc.dataOffset, &materialData->vars[ varI ].aFloat, desc.dataSize );
				break;
			}
			case EMatVar_Int:
			{
				memcpy( writeData + desc.dataOffset, &materialData->vars[ varI ].aInt, desc.dataSize );
				break;
			}
			//case EMatVar_Bool:
			//{
			//	bool* b = (bool*)( writeData + desc.dataOffset );
			//	*b = materialData->vars[ varI ].aBool;
			//	break;
			//}
			case EMatVar_Vec2:
			{
				memcpy( writeData + desc.dataOffset, &materialData->vars[ varI ].aVec2, desc.dataSize );
				break;
			}
			case EMatVar_Vec3:
			{
				memcpy( writeData + desc.dataOffset, &materialData->vars[ varI ].aVec3, desc.dataSize );
				break;
			}
			case EMatVar_Vec4:
			{
				memcpy( writeData + desc.dataOffset, &materialData->vars[ varI ].aVec4, desc.dataSize );
				break;
			}

			default:
				Log_ErrorF( "Unknown Material Var Type: %d", desc.type );
				break;
		}
	}

	render->BufferWrite( bufIt->second.aStagingBuffer, shaderData->aMaterialSize, writeData );

	free( writeData );

	// write new material data to the buffer
	BufferRegionCopy_t copy;
	copy.aSrcOffset = 0;
	copy.aDstOffset = 0;
	copy.aSize      = shaderData->aMaterialSize;

	render->BufferCopyQueued( bufIt->second.aStagingBuffer, bufIt->second.aBuffer, &copy, 1 );
}


void Shader_UpdateMaterialVars()
{
	for ( const auto& mat : gGraphicsData.aDirtyMaterials )
	{
		ch_handle_t    shader     = gGraphics.Mat_GetShader( mat );
		ShaderData_t* shaderData = Shader_GetData( shader );

		if ( shaderData->aMaterialVarCount == 0 )
			continue;

		ShaderMaterialData* data = Shader_GetMaterialData( shader, mat );

		// ?????
		if ( !data )
		{
			std::string_view shaderName = gGraphics.GetShaderName( shader );
			Log_ErrorF( gLC_ClientGraphics, "Shader does not have material data: %s\n", shaderName.empty() ? "UNNAMED" : shaderName.data() );
			continue;
		}

		// update vars
		data->vars.resize( shaderData->aMaterialVarCount );
		for ( u32 varI = 0; varI < shaderData->aMaterialVarCount; varI++ )
		{
			ShaderMaterialVarDesc& desc = shaderData->apMaterialVars[ varI ];
			data->vars[ varI ].aType    = desc.type;

			switch ( desc.type )
			{
				case EMatVar_Texture:
				{
					data->vars[ varI ].aTexture = gGraphics.Mat_GetTexture( mat, desc.name, desc.defaultTextureHandle );
					break;
				}
				case EMatVar_Float:
				{
					data->vars[ varI ].aFloat = gGraphics.Mat_GetFloat( mat, desc.name, desc.defaultFloat );
					break;
				}
				case EMatVar_Int:
				{
					data->vars[ varI ].aInt = gGraphics.Mat_GetInt( mat, desc.name, desc.defaultInt );
					break;
				}
				//case EMatVar_Bool:
				//{
				//	data->vars[ varI ].aBool = gGraphics.Mat_GetBool( mat, desc.name, desc.defaultBool );
				//	break;
				//}
				case EMatVar_Vec2:
				{
					data->vars[ varI ].aVec2 = gGraphics.Mat_GetVec2( mat, desc.name, desc.defaultVec2 );
					break;
				}
				case EMatVar_Vec3:
				{
					data->vars[ varI ].aVec3 = gGraphics.Mat_GetVec3( mat, desc.name, desc.defaultVec3 );
					break;
				}
				case EMatVar_Vec4:
				{
					data->vars[ varI ].aVec4 = gGraphics.Mat_GetVec4( mat, desc.name, desc.defaultVec4 );
					break;
				}

				default:
					Log_ErrorF( "Unknown Material Var Type: %d", desc.type );
					break;
			}
		}

		// update buffer if needed
		if ( shaderData->aUseMaterialBuffer )
		{
			Shader_WriteMaterialBuffer( mat, shader, shaderData, data );
		}
	}

	gGraphicsData.aDirtyMaterials.clear();
}


ShaderMaterialData* Shader_GetMaterialData( ch_handle_t sShader, ch_handle_t sMat )
{
	auto shaderIt = gShaderMaterials.find( sShader );

	if ( shaderIt == gShaderMaterials.end() )
	{
		// shader does not use materials probably
		return nullptr;
	}

	auto matIt = shaderIt->second.find( sMat );

	if ( matIt == shaderIt->second.end() )
	{
		// material not found
		return nullptr;
	}

	return &matIt->second;
}


const std::unordered_map< ch_handle_t, ShaderMaterialData >* Shader_GetMaterialDataMap( ch_handle_t sShader )
{
	auto shaderIt = gShaderMaterials.find( sShader );

	if ( shaderIt == gShaderMaterials.end() )
	{
		// shader does not use materials probably
		return nullptr;
	}

	return &shaderIt->second;
}


CONCMD( r_update_material_descriptors )
{
	for ( auto& [ shader, matList ] : gShaderMaterials )
	{
		// TODO: find shader data, and write vars
		ShaderData_t* shaderData = Shader_GetData( shader );

		if ( !shaderData->aUseMaterialBuffer )
			continue;

		Shader_UpdateMaterialDescriptorSets( shader, shaderData, matList );
	}
}

