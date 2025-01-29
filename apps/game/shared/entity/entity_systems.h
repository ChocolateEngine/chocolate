#pragma once

#include "entity.h"


class LightSystem : public IEntityComponentSystem
{
  public:
	LightSystem() {}
	~LightSystem() {}

	void ComponentAdded( Entity sEntity, void* spData ) override;
	void ComponentRemoved( Entity sEntity, void* spData ) override;
	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};

extern LightSystem gLightEntSystems;
LightSystem&       GetLightEntSys();


// ------------------------------------------------------------


class EntSys_Transform : public IEntityComponentSystem
{
  public:
	EntSys_Transform() {}
	~EntSys_Transform() {}

	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};

extern EntSys_Transform gEntSys_Transform;


// ------------------------------------------------------------


class EntSys_Renderable : public IEntityComponentSystem
{
  public:
	EntSys_Renderable() {}
	~EntSys_Renderable() {}

	void ComponentAdded( Entity sEntity, void* spData ) override;
	void ComponentRemoved( Entity sEntity, void* spData ) override;
	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};

extern EntSys_Renderable gEntSys_Renderable;
EntSys_Renderable&       GetRenderableEntSys();


