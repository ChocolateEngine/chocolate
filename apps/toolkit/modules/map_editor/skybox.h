#pragma once

#include <glm/vec3.hpp>


bool        Skybox_Init();
void        Skybox_Destroy();
void        Skybox_SetAng( const glm::vec3& srAng );
void        Skybox_SetMaterial( const std::string& srPath );
const char* Skybox_GetMaterialName();
ch_handle_t  Skybox_GetMaterial();

