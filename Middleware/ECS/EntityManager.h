#pragma once

#include "Component.h"

class Entity;
class EntityManager;

typedef unsigned int uint32;
typedef uint32		 EntityID;
typedef tinystl::unordered_map<uint32, Component*> ComponentMap;
typedef tinystl::unordered_map<EntityID, Entity*>  EntityMap;

class Entity
{
	friend class EntityManager;
public:
	Entity() : ID(0), mComponents() {}
	~Entity()
	{
		for (tinystl::unordered_hash_node<uint32, Component*> component : mComponents)
		{
			component.second->~Component();
		}
	}

	template <typename T>
	T* GetComponent()
	{
		T* componentOut = nullptr;
		
		ComponentMap::iterator it = mComponents.find(T::getTypeStatic());
		if (it != mComponents.end())
			componentOut = (T*)it->second;

		return componentOut;
	}

private:
	void AddComponent(Component* component);

	EntityID ID;
	ComponentMap mComponents;
};

class EntityManager
{
public:
	EntityManager();
	~EntityManager();

	template <typename T>
	void addComponent(EntityID id)
	{
		Entity* pEntity = getEntityByID(id);
		if (pEntity)
		{
			T* pComponent = new T;
			pEntity->AddComponent(pComponent);
		}
	}

	EntityID createEntity()
	{
		Entity* pEntity = new Entity;
		pEntity->ID = mEntityIDCounter++;
		mEntities.emplace({ pEntity->ID, pEntity });
		return pEntity->ID;
	}

	void destroyEntity(EntityID id)
	{
		Entity* pEntity = getEntityByID(id);
		if (pEntity)
		{
			delete pEntity;
		}
	}

	Entity* getEntityByID(EntityID id)
	{
		EntityMap::iterator itr = mEntities.find(id);
		if (itr != mEntities.end())
		{
			return itr->second;
		}
		return nullptr;
	}

private:
	EntityMap mEntities;
	EntityID  mEntityIDCounter;
};