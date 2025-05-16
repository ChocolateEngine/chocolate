#pragma once

#include "core/core.h"
#include "itool.h"

struct ViewportCamera_t;

#include "igraphics.h"

class IRender;
class IInputSystem;
class IGraphics;
class IRenderSystemOld;

extern float  gFrameTime;
extern double gCurTime;

extern bool   gShowQuitConfirmation;

struct View_t
{
	glm::mat4  aViewMat;
	glm::mat4  aProjMat;

	// projection matrix * view matrix
	glm::mat4  aProjViewMat;

	// TODO: use this in the future
	// u32              aViewportIndex;

	// TODO: remove this from here, right now this is the same between ALL contexts
	glm::uvec2 aResolution;
	glm::uvec2 aOffset;

	glm::vec3  aPos{};
	glm::vec3  aAng{};
};


// -------------------------------------------------------------
// Material Editor


class MaterialEditor : public ITool
{
	const char* GetName() override
	{
		return "Material Editor";
	}

	// ISystem
	bool Init() override;
	void Shutdown() override;

	// ITool
	bool LoadSystems() override;

	bool Launch( const ToolLaunchData& launchData ) override;
	void Close() override;

	void Update( float frameTime ) override;
	void Render( float frameTime, bool resize, glm::uvec2 sWindowSize ) override;
	void Present() override;

	bool OpenAsset( const std::string& path ) override;
};


extern MaterialEditor gMatEditorTool;


void                  MaterialEditor_Draw( glm::uvec2 sOffset );

bool                  MaterialEditor_LoadMaterial( const std::string& path );
void                  MaterialEditor_FreeMaterial( ch_handle_t mat );
void                  MaterialEditor_CloseMaterial( ch_handle_t mat );
void                  MaterialEditor_CloseActiveMaterial();
void                  MaterialEditor_FreeImGuiTextures();

void                  MaterialEditor_SetActiveMaterial( ch_handle_t sMat );
ch_handle_t           MaterialEditor_GetActiveMaterial();

void                  MaterialEditor_Init();
void                  MaterialEditor_Close();
