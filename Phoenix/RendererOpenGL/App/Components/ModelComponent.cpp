#include "ModelComponent.h"

DEFINE_COMPONENT(ModelComponent)

ModelComponent::ModelComponent()
{
	pModel = new SkinnedMesh();
}

ModelComponent::~ModelComponent()
{
	delete pModel;
}

void ModelComponent::InitModel()
{
	if(filepath)
		pModel->LoadMesh(filepath, 0);
}

void ModelComponent::Draw(ShaderProgram program)
{
	pModel->Render(program);
}

START_REGISTRATION(ModelComponent)
/*
DEFINE_VARIABLE(ModelComponent, x)
DEFINE_VARIABLE(ModelComponent, y)
*/