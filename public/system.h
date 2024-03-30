/*
system.h ( Authored by p0lyh3dron )

Defines the base system that all engine
systems inherit.
*/
#pragma once

class ISystem
{
   public:
	// Main Update Function for this system (remove this? all systems usually have their own update function)
	virtual void Update( float sDT ){};

	// Initialize System
	virtual bool Init() { return true; };

	// Shutdown This System
	virtual void Shutdown(){};

	// Destructs the system, freeing any used memory
	virtual ~ISystem() = default;
};

