#include "EntityManager.h"


EntityManager::EntityManager()
{
	mEntityIDCounter = 1;
}

EntityManager::~EntityManager()
{
	for (tinystl::unordered_hash_node<uint32, Entity*> entity : mEntities)
	{
		entity.second->~Entity();
	}
}

void Entity::AddComponent(Component* component)
{
	mComponents.insert({ component->getType(), component });
}