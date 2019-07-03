#pragma once

#include "../../Common/Thirdparty/stl/TINYSTL/hash.h"
#include "../../Common/Thirdparty/stl/TINYSTL/string.h"
#include "../../Common/Thirdparty/stl/TINYSTL/vector.h"
#include "../../Common/Thirdparty/stl/TINYSTL/unordered_map.h"

typedef unsigned int uint32;

class ComponentRepresentation
{
public:
	virtual ~ComponentRepresentation() {}
	virtual uint32 getComponentID() const = 0;
	virtual const char* getComponentName() const = 0;
	virtual tinystl::unordered_map<tinystl::string, uint32> getVarNames() = 0;
};


class Component
{
public:
	virtual ~Component() {}
	virtual Component* clone() const = 0;
	virtual uint32 getType() const = 0;
	virtual ComponentRepresentation* createRepresentation() = 0;
	virtual void destroyRepresentation(ComponentRepresentation* pRep) = 0;
};

// Unique ID
static uint32 m_VarIdCounter = 0;
static const uint32 generateUniqueID()
{
	return m_VarIdCounter++;
}

#define DECLARE_COMPONENT(Component_) \
class Component_ : public Component { \
	public: \
		virtual Component_* clone() const override; \
		virtual uint32 getType() const override; \
		static  uint32 getTypeStatic(); \
		virtual ComponentRepresentation* createRepresentation() override; \
		virtual void destroyRepresentation(ComponentRepresentation* pRep) override; \
\
		static uint32 Component_##typeHash; \
	private:


#define END };


#define DEFINE_COMPONENT(Component_) \
	Component_* Component_::clone() const { return new Component_; } \
	uint32 Component_::getType() const { return Component_::getTypeStatic(); } \
	uint32 Component_::getTypeStatic() {  return Component_##typeHash; } \
	ComponentRepresentation* Component_::createRepresentation() { return new Component_##Representation; } \
	void Component_::destroyRepresentation(ComponentRepresentation* pRep) { pRep->~ComponentRepresentation(); delete pRep; } \
\
	uint32 Component_::Component_##typeHash = (uint32)(tinystl::hash(#Component_));


#define REGISTER_COMPONENT_CLASS(Component_) \
class Component_##Representation : public ComponentRepresentation \
{ \
public:\
	Component_##Representation() {} \
	Component_##Representation(Component_* const comp) \
	{ \
		 \
	} \
	\
	virtual uint32 getComponentID() const override { return ComponentID; }\
	virtual const char* getComponentName() const override { return #Component_; }\
	static const uint32 storeIDandName(tinystl::string var_name, const uint32 id)\
	{\
		Component_##varNames.emplace({var_name, id}); \
		return id; \
	}\
	tinystl::unordered_map<tinystl::string, uint32> getVarNames() { return Component_##Representation::Component_##varNames; } \
	\
	static const uint32 ComponentID; \
	static uint32 Component_VarCounter; \
	static tinystl::unordered_map<tinystl::string, uint32> Component_##varNames;


#define START_REGISTRATION(Component_) \
	uint32 Component_##Representation::Component_VarCounter = 0; \
	const uint32 Component_##Representation::ComponentID = generateUniqueID(); \
	tinystl::unordered_map<tinystl::string, uint32> Component_##Representation::Component_##varNames;

#define REGISTER_VARIABLE(x) \
static const uint32 x;

#define DEFINE_VARIABLE(Component_, x) \
const uint32 Component_##Representation::x = Component_##Representation::storeIDandName(#x, Component_VarCounter++);