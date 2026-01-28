#include "Scene.h"

void Scene::addObject(Object* obj) {
	m_objectRefs.push_back(obj);
}