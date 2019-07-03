#include "../../../Middleware/ECS/Component.h"

DECLARE_COMPONENT(PositionComponent)
public:
	int x, y;
	PositionComponent();
END



REGISTER_COMPONENT_CLASS(PositionComponent)
	REGISTER_VARIABLE(x)
	REGISTER_VARIABLE(y)
END