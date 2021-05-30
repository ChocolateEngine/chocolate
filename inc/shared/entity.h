#ifndef ENTITY_H
#define ENTITY_H

#include "../core/graphics.h"
#include "../types/enums.h"

class entity_c
{
	protected:

	float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

	public:

	std::string type;
	std::string name;

	entity_c
		(  );
	~entity_c
		(  );
};

#endif
