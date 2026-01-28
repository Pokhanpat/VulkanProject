#pragma once
#include <vector>

#include "Object.h"

class Scene {
public:
	std::vector<Object*> m_objectRefs;
	
	void addObject(Object* obj);
};