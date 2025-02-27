#pragma once


class IServerSystem : public ISystem
{
   public:
	// Call this before calling Init()
	virtual bool LoadSystems()                           = 0;

	virtual bool IsHosting()                             = 0;

	// Start Server on this map
	virtual void StartServer( const std::string& srMap ) = 0;

	virtual void CloseServer()                           = 0;

	virtual void PrintStatus()                           = 0;
};


#define ISERVER_NAME "Server"
#define ISERVER_VER  2
