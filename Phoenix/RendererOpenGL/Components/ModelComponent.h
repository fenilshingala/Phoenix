#include "../../../Middleware/ECS/Component.h"
#include "../../RendererOpenGL/RendererOpenGL.h"

DECLARE_COMPONENT(ModelComponent)
public:
	//int x, y;
	ModelComponent();
	~ModelComponent();

	void InitModel();
	void Draw(ShaderProgram program);

	const char* filepath;
	Model* pModel = NULL;
END


REGISTER_COMPONENT_CLASS(ModelComponent)
	//REGISTER_VARIABLE(x)
	//REGISTER_VARIABLE(y)
END