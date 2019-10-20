#include <iostream>
#include "PositionComponent.h"
#include "../../../Middleware/ECS/EntityManager.h"

// I am Stud Shingala
int main()
{
	EntityManager manager;
	EntityID id = manager.createEntity();
	Entity* pEntity = manager.getEntityByID(id);
	if (pEntity)
	{
		manager.addComponent<PositionComponent>(id);
		PositionComponent* position = pEntity->GetComponent<PositionComponent>();
		ComponentRepresentation* rep = position->createRepresentation();
		auto map = rep->getVarNames();
		for (auto x : map)
		{
			tinystl::string str = x.first;
			int x = 0;
		}
		position->destroyRepresentation(rep);
	}
	
	getchar();
	return 0;
}