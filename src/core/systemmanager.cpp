#include "core/systemmanager.h"


DLL_EXPORT SystemManager* systems = new SystemManager;


#if 0
void SystemManager::Add( BaseSystem* sys )
{
	aSystemList.push_back( sys );
}


const SystemManager::SystemList& SystemManager::GetSystemList()
{
	return aSystemList;
}
#endif
