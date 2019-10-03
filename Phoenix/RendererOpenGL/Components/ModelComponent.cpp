#include "ModelComponent.h"

DEFINE_COMPONENT(ModelComponent)

ModelComponent::ModelComponent()
{
}

ModelComponent::~ModelComponent()
{
}

void ModelComponent::InitModel()
{
	if(filepath)
		pModel = LoadModel(filepath, false);
}

void ModelComponent::Draw(ShaderProgram program)
{
	pModel->Draw(program);
}

START_REGISTRATION(ModelComponent)
/*
DEFINE_VARIABLE(ModelComponent, x)
DEFINE_VARIABLE(ModelComponent, y)
*/