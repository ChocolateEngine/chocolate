#pragma once


class IClientSystem : public ISystem
{
   public:
	// Ran Before the server is updated
	virtual void PreUpdate( float frameTime )  = 0;

	virtual void PostUpdate( float frameTime ) = 0;

	// Are we connected to a server
	virtual bool Connected()                   = 0;

	// To Disconnect From a Server and shut it down if we're hosting, called by manager
	virtual void Disconnect()                  = 0;

	virtual void PrintStatus()                 = 0;

	// Sends this convar over to the server to execute
	virtual void SendConVar( std::string_view sName, const std::vector< std::string >& srArgs = {} ) = 0;
};


#define ICLIENT_NAME "Client"
#define ICLIENT_VER  1
