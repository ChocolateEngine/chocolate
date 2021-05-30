#ifndef ENTITYCOMPONENTS_H
#define ENTITYCOMPONENTS_H

#include "../core/graphics.h"
#include "../types/enums.h"

typedef struct
{
	entity_type_e type;
	union
	{
		sprite_t sprite;
		model_t model;
	}
}render_module_t;

#endif
