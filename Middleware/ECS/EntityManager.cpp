#include "EntityManager.h"

#include <stdint.h>

tinystl::unordered_map<uint32_t, std::list<Component*>> mComponentPoolMap;

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

std::list<Component*> EntityManager::GetComponents(uint32_t type)
{
	tinystl::unordered_map<uint32_t, std::list<Component*>>::iterator itr = mComponentPoolMap.find(type);
	return itr->second;
}

Entity::~Entity()
{
	for (tinystl::unordered_hash_node<uint32, Component*> component : mComponents)
	{
		Component* pComponent = component.second;

		uint32_t type = pComponent->getType();
		tinystl::unordered_map<uint32_t, std::list<Component*>>::iterator itr = mComponentPoolMap.find(type);
		if (itr != mComponentPoolMap.end())
		{
			itr->second.remove(pComponent);
		}

		pComponent->~Component();
	}
}

void Entity::AddComponent(Component* component)
{
	uint32_t type = component->getType();
	mComponents.insert({ type, component });

	tinystl::unordered_map<uint32_t, std::list<Component*>>::iterator itr = mComponentPoolMap.find(type);
	if (itr == mComponentPoolMap.end())
	{
		std::list<Component*> list;
		list.push_front(component);
		mComponentPoolMap.insert({ type, list });
	}
	else
	{
		itr->second.push_front(component);
	}
}