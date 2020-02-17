#include "../../../../Middleware/ECS/Component.h"
#include "../../../RendererOpenGL/RendererOpenGL.h"

struct LightBlock
{
	glm::vec3 lightPosition;
	float pad0;
	glm::vec3 lightColor;
	float pad1;

	float Linear;
	float Quadratic;
	float pad2;
	float pad3;
};

DECLARE_COMPONENT(LightComponent)
public:
	LightComponent();
	~LightComponent();

	LightBlock light;
END


REGISTER_COMPONENT_CLASS(LightComponent)
	// register variables
END