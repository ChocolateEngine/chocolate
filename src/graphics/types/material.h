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

	bool                SetShader( const char* name ) override;
	std::string         GetShaderName(  ) override { return apShader->aName; }

	MaterialVar        *GetVar( const std::string &name ) override;
	MaterialVar        *AddVar( MaterialVar* var ) override;
	
	template <typename T>
	MaterialVar        *AddVarInternal( const std::string &name, const std::string &raw, T data );

	MaterialVar        *AddVar( const std::string &name, const std::string &raw, Texture *data ) override;
	MaterialVar        *AddVar( const std::string &name, const std::string &raw, float data ) override;
	MaterialVar        *AddVar( const std::string &name, const std::string &raw, int data ) override;
	MaterialVar        *AddVar( const std::string &name, const std::string &raw, const glm::vec2 &data ) override;
	MaterialVar        *AddVar( const std::string &name, const std::string &raw, const glm::vec3 &data ) override;
	MaterialVar        *AddVar( const std::string &name, const std::string &raw, const glm::vec4 &data ) override;

	Texture            *GetTexture( const std::string &name, Texture *fallback = nullptr ) override;
	float               GetFloat( const std::string &name, float fallback = 0.f ) override;
	int                 GetInt( const std::string &name, int fallback = 0 ) override;
	glm::vec2           GetVec2( const std::string &name, glm::vec2 fallback = {} ) override;
	glm::vec3           GetVec3( const std::string &name, glm::vec3 fallback = {} ) override;
	glm::vec4           GetVec4( const std::string &name, glm::vec4 fallback = {} ) override;

	std::string                 aName;
	
	VkDescriptorSetLayout       apTextureLayout = nullptr;

	BaseShader*                 apShader = nullptr;

	std::vector< MaterialVar * > aVars;

	inline VkDescriptorSetLayout       GetTextureLayout() const   { return apTextureLayout; }
};

