#pragma once

#include "../shared/system.h"
#include "../glm/vec2.hpp"

class BaseInputSystem : public BaseSystem
{
public:

	virtual const glm::vec2& GetMouseDelta(  ) = 0;
	virtual const glm::vec2& GetMousePos(  ) = 0;

	virtual bool WindowHasFocus(  ) = 0;
};

