#pragma once

#include <stdint.h>

class EntityManager;
class LightingSystem
{
public:
	LightingSystem(EntityManager* pEM) : pEntityManager(pEM) {}
	~LightingSystem();

	void AddLights(uint32_t* lights, uint64_t noOfLights);
	
	EntityManager* pEntityManager = NULL;
};