#pragma once

#include <glm/vec3.hpp>

#include "entity/entity.h"


#if CH_CLIENT
bool        Skybox_Init();
void        Skybox_Destroy();
void        Skybox_Draw();
void        Skybox_SetAng( const glm::vec3& srAng );
void        Skybox_SetMaterial( const std::string& srPath );
const char* Skybox_GetMaterialName();
#endif


// Component
struct CSkybox
{
	ComponentNetVar< std::string > aMaterialPath;
};


class SkyboxSystem : public IEntityComponentSystem
{
   public:
	SkyboxSystem() {}
	~SkyboxSystem() {}

	// Called when the component is added to this entity
	virtual void ComponentAdded( Entity sEntity, void* spData );

	// Called when the component is removed from this entity
	virtual void ComponentRemoved( Entity sEntity, void* spData );

	// Called when the component data has been updated (ONLY ON CLIENT RIGHT NOW)
	virtual void ComponentUpdated( Entity sEntity, void* spData );

	virtual void Update();
};

