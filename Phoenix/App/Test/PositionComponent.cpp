#include "PositionComponent.h"

DEFINE_COMPONENT(PositionComponent)

PositionComponent::PositionComponent() : x(0.0f), y(0.0f)
{

}


START_REGISTRATION(PositionComponent)

DEFINE_VARIABLE(PositionComponent, x)
DEFINE_VARIABLE(PositionComponent, y)