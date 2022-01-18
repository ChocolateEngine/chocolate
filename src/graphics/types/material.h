/*
material.h ( Authored by Demez )


*/
#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"
#include "../shaders/shaders.h"
#include "graphics/imaterial.h"

#include <vulkan/vulkan.hpp>
#include <filesystem>

class Renderer;
class ModelData;


// basically tied to obj files right now
// 152 bytes
class Material: public IMaterial
{
public:
	                    Material() {}
	virtual            ~Material() {}

	void                Init();
	void                Destroy();

	void                SetShader( const char* name ) override;
	std::string         GetShaderName(  ) override { return apShader->aName; }

	MaterialVar        *GetVar( const std::string &name ) override;
	MaterialVar        *AddVar( const std::string &name, MatVar type, void *data ) override;
	MaterialVar        *AddVar( const std::string &name, MatVar type, const std::string &raw, void *data ) override;
	MaterialVar        *AddVar( MaterialVar* var ) override;
	
	// internal version
	template <typename T>
	T GetVarData( const std::string &name, T fallback );

	Texture            *GetTexture( const std::string &name, Texture *fallback = nullptr ) override;
	float               GetFloat( const std::string &name, float fallback = 0.f ) override;
	int                 GetInt( const std::string &name, int fallback = 0 ) override;
	glm::vec2           GetVec2( const std::string &name, glm::vec2 fallback = {} ) override;
	glm::vec3           GetVec3( const std::string &name, glm::vec3 fallback = {} ) override;
	glm::vec4           GetVec4( const std::string &name, glm::vec4 fallback = {} ) override;

	std::string                 aName;
	
	// uh, this may need to be moved lol
	VkDescriptorSetLayout       apTextureLayout = nullptr;

	BaseShader*                 apShader = nullptr;

	std::vector< MaterialVar * > aVars;

	inline VkDescriptorSetLayout       GetTextureLayout() const   { return apTextureLayout; }
};

