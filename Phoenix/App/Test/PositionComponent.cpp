#include "PositionComponent.h"

DEFINE_COMPONENT(PositionComponent)

PositionComponent::PositionComponent() : x(0), y(0)
{

}


START_REGISTRATION(PositionComponent)

DEFINE_VARIABLE(PositionComponent, x)
DEFINE_VARIABLE(PositionComponent, y)