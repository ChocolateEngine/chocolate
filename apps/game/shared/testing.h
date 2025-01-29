#pragma once

struct Model;
class IPhysicsObject;


#define DEFAULT_PROTOGEN_PATH "models/protogen_wip_25d/protogen_25d.glb"
//#define DEFAULT_PROTOGEN_PATH "models/protogen_wip_25d/protogen_wip_25d.obj"


struct ModelPhysTest
{
	Model*                         mdl;
	std::vector< IPhysicsObject* > physObj;
};


void TEST_Init();
void TEST_Shutdown();
void TEST_EntUpdate();

void TEST_CL_UpdateProtos( float frameTime );
void TEST_SV_UpdateProtos( float frameTime );

void TEST_UpdateAudio();

