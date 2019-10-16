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

	SkinnedMesh mesh;
	mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/guard/boblampclean.md5mesh");

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};
	// first, configure the cube's VAO (and VBO)
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUseProgram(skinningShader.mId);
	int zero = 0;
	skinningShader.SetUniform("gColorMap", &zero);

	window.initGui();

	float timer = 0.0f;
	bool drawBones = false;

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
			glBindVertexArray(cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);


			// also draw the lamp object
			glUseProgram(lampShader.mId);

			lampShader.SetUniform("projection", &projection);
			lampShader.SetUniform("view", &view);
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
			lampShader.SetUniform("model", &model);

			glBindVertexArray(lightVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			// skinning
			tinystl::vector<aiMatrix4x4> Transforms, BoneTransforms;
			mesh.BoneTransform(timer / 1000.0f, Transforms, BoneTransforms);

			if(!drawBones)
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
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
				model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
				skinningShader.SetUniform("model", &model);

				mesh.Render();
			}


			if (drawBones)
			{
				glUseProgram(lampShader.mId);
				lampShader.SetUniform("projection", &projection);
				lampShader.SetUniform("view", &view);

				glBindVertexArray(lightVAO);
				for (unsigned int i = 0; i < BoneTransforms.size(); ++i)
				{
					aiMatrix4x4 aiModel = BoneTransforms[i];
					glm::mat4 boneModel = toGlmMat(aiModel);

					glm::mat4 myModel(1.0f);
					myModel = glm::translate(myModel, glm::vec3(0.0f, -1.0f, 0.0f));
					myModel = glm::rotate(myModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					myModel = glm::scale(myModel, glm::vec3(0.05f, 0.05f, 0.05f));

					boneModel = myModel * boneModel;

					lampShader.SetUniform("model", &boneModel);
					//glUniformMatrix4fv(glGetUniformLocation(lampShader.mId, "model"), 1, GL_TRUE, (const GLfloat*)&aiModel);
					glDrawArrays(GL_TRIANGLES, 0, 36);
				}
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

					ImGui::Checkbox("Draw Bones", &drawBones);

				ImGui::End();

				window.endGuiFrame();
			}

			window.swapWindow();

			window.update();
			processInputs();
		}

		window.endFrame();
	}

	window.exitGui();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	window.exitWindow();
}

#endif