#include "../Picker.h"

#if FORWARD_LIGHTING

#include "../Common.h"

// lighting
glm::vec3 lightPos(1.2f, 3.0f, 2.0f);
static const uint32_t MAX_BONES = 100;

glm::mat4 toGlmMat(aiMatrix4x4 aiMat)
{
	glm::mat4 mat;
	aiMat.Transpose();
	mat[0][0] = aiMat.a1;
	mat[0][1] = aiMat.a2;
	mat[0][2] = aiMat.a3;
	mat[0][3] = aiMat.a4;
	mat[1][0] = aiMat.b1;
	mat[1][1] = aiMat.b2;
	mat[1][2] = aiMat.b3;
	mat[1][3] = aiMat.b4;
	mat[2][0] = aiMat.c1;
	mat[2][1] = aiMat.c2;
	mat[2][2] = aiMat.c3;
	mat[2][3] = aiMat.c4;
	mat[3][0] = aiMat.d1;
	mat[3][1] = aiMat.d2;
	mat[3][2] = aiMat.d3;
	mat[3][3] = aiMat.d4;

	return mat;
}

void Run()
{
	window.initWindow();
	lastX = window.windowWidth()  / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	glEnable(GL_DEPTH_TEST);
	
	ShaderProgram lightingShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.frag");
	ShaderProgram lampShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");
	ShaderProgram skinningShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/skinning.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/skinning.frag");

	OpenGLRenderer* pOpenglRenderer = new OpenGLRenderer();

	SkinnedMesh mesh;
	//mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/guard/boblampclean.md5mesh");
	//mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/nanosuit/nanosuit.obj");
	mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/Jumping.fbx");
	mesh.AddAnimation("../../Phoenix/RendererOpenGL/App/Resources/Objects/Walking.fbx");
	//mesh.AddAnimation("../../Phoenix/RendererOpenGL/App/Resources/Objects/Hip Hop Dancing.fbx");

	glUseProgram(skinningShader.mId);
	int zero = 0;
	skinningShader.SetUniform("gColorMap", &zero);
	skinningShader.SetUniform("isAnim", &mesh.mIsAnim);

	window.initGui();

	float timer = 0.0f;
	bool drawJoints = false;
	bool drawBones = false;

	int animationIndex = 0;
	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();
		timer += window.frameTime();

		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();

			// draw our first triangle
			glUseProgram(lightingShader.mId);
			glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
			lightingShader.SetUniform("objectColor", &objectColor);
			glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightingShader.SetUniform("lightColor", &lightColor);
			lightingShader.SetUniform("lightPos", &lightPos);
			lightingShader.SetUniform("viewPos", &camera.Position);

			lightingShader.SetUniform("projection", &projection);
			lightingShader.SetUniform("view", &view);

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(10.0f, 0.3f, 10.0f));
			model = glm::translate(model, glm::vec3(0.0f, -4.0f, 0.0f));
			lightingShader.SetUniform("model", &model);

			// render the cube
			pOpenglRenderer->RenderCube();

			// also draw the lamp object
			glUseProgram(lampShader.mId);

			lampShader.SetUniform("projection", &projection);
			lampShader.SetUniform("view", &view);
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
			lampShader.SetUniform("model", &model);

			pOpenglRenderer->RenderCube();

			// skinning
			tinystl::vector<aiMatrix4x4> Transforms, BoneTransforms;
			mesh.BoneTransform(timer / 1000.0f, Transforms, BoneTransforms);
			
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
			// for guard
			/*model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));*/
			model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));

			if(!drawJoints && !drawBones)
			{
				glUseProgram(skinningShader.mId);
				for (uint32_t i = 0; i < Transforms.size(); i++)
				{
					assert(i < MAX_BONES);
					std::string gBone = "gBones[" + std::to_string(i) + "]";
					glUniformMatrix4fv(glGetUniformLocation(skinningShader.mId, gBone.c_str()), 1, GL_TRUE, (const GLfloat*)&Transforms[i]);
				}

				skinningShader.SetUniform("gEyeWorldPos", &camera.Position);

				skinningShader.SetUniform("projection", &projection);
				skinningShader.SetUniform("view", &view);
				skinningShader.SetUniform("model", &model);
				
				mesh.Render();
			}


			if (drawJoints)
			{
				glUseProgram(lampShader.mId);
				lampShader.SetUniform("projection", &projection);
				lampShader.SetUniform("view", &view);
				
				for (unsigned int i = 0; i < BoneTransforms.size(); ++i)
				{
					aiMatrix4x4 aiModel = BoneTransforms[i];
					glm::mat4 boneModel = toGlmMat(aiModel);

					boneModel = model * boneModel;

					lampShader.SetUniform("model", &boneModel);
					pOpenglRenderer->RenderCube();
				}
				glBindVertexArray(0);
			}

			if (drawBones)
			{
				glUseProgram(lampShader.mId);
				lampShader.SetUniform("projection", &projection);
				lampShader.SetUniform("view", &view);

				uint32_t size = (uint32_t)mesh.mLineSegments.size();
				tinystl::vector<float> vertices(size * 2 * 3);

				for (uint32_t i = 1; i < size; ++i)
				{
					aiMatrix4x4 parent = mesh.mLineSegments[i].mParent;
					aiMatrix4x4 child = mesh.mLineSegments[i].mChild;

					aiVector3D parentPos, parentScale, parentRot;
					aiVector3D childPos, childScale, childRot;

					parent.Decompose(parentScale, parentRot, parentPos);
					child.Decompose(childScale, childRot, childPos);

					glm::vec3 A(parentPos.x, parentPos.y, parentPos.z);
					glm::vec3 B(childPos.x, childPos.y, childPos.z);

					glm::mat4 FinalmodelMatrix = model * pOpenglRenderer->ModelMatForLineBWTwoPoints(A, B);

					lampShader.SetUniform("model", &FinalmodelMatrix);

					pOpenglRenderer->RenderLine();
				}
				glBindVertexArray(0);

			}
			

			// GUI
			{
				window.beginGuiFrame();

				bool truebool = true;
				
				str = "controlled fps: " + std::to_string(window.frameRate());
				ImGui::Begin("BLEH!", &truebool);
				
					ImGui::Text(str.c_str());
				
					str = "actual fps: " + std::to_string(window.actualFrameRate());
					ImGui::Text(str.c_str());

					ImGui::Checkbox("Draw Joints", &drawJoints);
					ImGui::Checkbox("Draw Bones", &drawBones);
					
					ImGui::InputInt("Animation Index", &animationIndex);
					mesh.SetCurrentAnimation(animationIndex);

				ImGui::End();

				window.endGuiFrame();
			}

			window.swapWindow();

			window.update();
			processInputs();
		}

		window.endFrame();
	}

	delete pOpenglRenderer;

	window.exitGui();

	window.exitWindow();
}

#endif