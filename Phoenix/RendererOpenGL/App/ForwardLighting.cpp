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

#pragma region ANIM2

#include <map>
#define POINTS_NUM 10
#define delta 0.01

glm::vec3 curvePoints[POINTS_NUM] = { glm::vec3(1.0f, 0.0f, 1.0f),
									  glm::vec3(1.0f, 0.0f, 1.0f),
									  glm::vec3(2.0f, 0.0f, 2.0f),
									  glm::vec3(0.0f, 0.0f, 3.0f),
									  glm::vec3(0.5f, 0.0f, 4.0f),
									  glm::vec3(0.0f, 0.0f, 5.0f),
									  glm::vec3(0.0f, 0.0f, 6.0f),
									  glm::vec3(-1.0f, 0.0f, 7.0f),
									  glm::vec3(-1.5f, 0.0f, 8.0f),
									  glm::vec3(-2.5f, 0.0f, 8.0f) };

glm::mat4 knots;

std::map<float, float> arcLengthAt;

struct arcLengthStruct
{
	float u;
	float length;
	int segmentNum;
};
std::vector<arcLengthStruct> myPathData;
std::vector<float> arcLength;
float curDelta;


//Curve function
glm::vec3 mySpaceCurve(float u, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, glm::vec3 point4);
//Inverse lengthAt function
float inverseArcLengthAt(float s, int &segment);
//lengthAt function (binary search)
int searchByLength(int left, int right, float s);

bool map_value_compare(std::pair<float, float> a, std::pair<float, float> b) {
	return a.second < b.second;
}
glm::vec3 mySpaceCurve(float u, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3, glm::vec3 point4)
{
	return 0.5f * ((knots[0][0] * point1 + knots[0][1] * point2 + knots[0][2] * point3 + knots[0][3] * point4) +
		(knots[1][0] * point1 + knots[1][1] * point2 + knots[1][2] * point3 + knots[1][3] * point4) * u +
		(knots[2][0] * point1 + knots[2][1] * point2 + knots[2][2] * point3 + knots[2][3] * point4) * u * u +
		(knots[3][0] * point1 + knots[3][1] * point2 + knots[3][2] * point3 + knots[3][3] * point4) * u * u * u);

}

float inverseArcLengthAt(float s, int &segment)
{
	int i = searchByLength(0, arcLength.size() - 1, s);

	segment = myPathData[0].segmentNum;
	if (i == -1)
		return myPathData[0].u;

	float u_i = myPathData[i].u;
	float s_i = myPathData[i].length;
	segment = myPathData[i].segmentNum;

	if (i == arcLength.size() - 1)
		return u_i;

	float s_i1 = myPathData[i + 1].length;

	float k = (s - s_i) / (s_i1 - s_i);
	float d = k * delta;

	return u_i + d;
}

int searchByLength(int left, int right, float s)
{
	if (right >= left)
	{
		int mid = left + (right - left) / 2;

		if (arcLength[mid] == s) return mid;

		if (arcLength[mid] > s) return searchByLength(left, mid - 1, s);

		return searchByLength(mid + 1, right, s);
	}

	return left - 1;
}

#pragma endregion

void Run()
{
	//knot sequence for the curve
	knots[0][0] = 0.0f;   knots[0][1] = 2.0f;   knots[0][2] = 0.0f;   knots[0][3] = 0.0f;
	knots[1][0] = -1.0f;  knots[1][1] = 0.0f;   knots[1][2] = 1.0f;   knots[1][3] = 0.0f;
	knots[2][0] = 2.0f;   knots[2][1] = -5.0f;  knots[2][2] = 4.0f;   knots[2][3] = -1.0f;
	knots[3][0] = -1.0f;  knots[3][1] = 3.0f;   knots[3][2] = -3.0f;  knots[3][3] = 1.0f;

	window.initWindow();
	lastX = window.windowWidth()  / 2.0f;
	lastY = window.windowHeight() / 2.0f;
	camera.Position = glm::vec3(0.0f, 1.0f, 10.0f);

	glEnable(GL_DEPTH_TEST);
	
	ShaderProgram lightingShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.frag");
	ShaderProgram lampShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");
	ShaderProgram skinningShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/skinning.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/skinning.frag");

	OpenGLRenderer* pOpenglRenderer = new OpenGLRenderer();

	SkinnedMesh meshWalking, meshIdle;
	//mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/guard/boblampclean.md5mesh");
	//mesh.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/nanosuit/nanosuit.obj");
	meshWalking.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/Walking.dae");
	meshIdle.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/Idle.dae");
	//mesh.AddAnimation("../../Phoenix/RendererOpenGL/App/Resources/Objects/Walking.fbx");
	//mesh.AddAnimation("../../Phoenix/RendererOpenGL/App/Resources/Objects/Hip Hop Dancing.fbx");

	SkinnedMesh* pMesh = &meshIdle;

	glUseProgram(skinningShader.mId);
	int zero = 0;
	skinningShader.SetUniform("gColorMap", &zero);
	skinningShader.SetUniform("isAnim", &pMesh->mIsAnim);

	window.initGui();

	float timer = 0.0f;
	bool drawJoints = false;
	bool drawBones = false;

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(0);

	int k = 0;
	float totalLength = 0.0f;
	std::vector<glm::vec3> plot;
	//calculating length of the curve
	for (int i = 0; i < POINTS_NUM - 3; i++)
		for (float t = 0; t < 1.0f - delta; t += delta)
		{
			arcLengthStruct pointData;
			pointData.length = totalLength + glm::distance(mySpaceCurve(t, curvePoints[i], curvePoints[i + 1], curvePoints[i + 2], curvePoints[i + 3]),
				mySpaceCurve(t + delta, curvePoints[i], curvePoints[i + 1], curvePoints[i + 2], curvePoints[i + 3]));
			pointData.segmentNum = i;
			pointData.u = t;

			totalLength = pointData.length;

			myPathData.push_back(pointData);
			plot.push_back(mySpaceCurve(t, curvePoints[i], curvePoints[i + 1], curvePoints[i + 2], curvePoints[i + 3]));
		}

	//normalizing
	for (auto it = myPathData.begin(); it != myPathData.end(); it++)
	{
		it->length /= totalLength;
		arcLength.push_back(it->length);
	}

	
	//initial angle
	glm::vec3 currAngle = glm::normalize(mySpaceCurve(delta, curvePoints[0], curvePoints[0 + 1], curvePoints[0 + 2], curvePoints[0 + 3]) -
		mySpaceCurve(0.0f, curvePoints[0], curvePoints[0 + 1], curvePoints[0 + 2], curvePoints[0 + 3]));//derivativeOfMySpaceCurve(0.0f, curvePoints[0], curvePoints[0 + 1], curvePoints[0 + 2], curvePoints[0 + 3]);

	float speed = 0.085f;
	float currentDistance = 0.0f;
	float t = 0.0f;

	float var = 0.009f;
	float speedFactor = 1.0f;

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
			model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
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


#pragma region ANIM2_MODEL

			//calculating speed (with easy-in/easy-out at the ends)
			float v;
			int segment;
			float v0 = 2 / (1 - 0.2 + 0.8);
			if (t < 0.2)
				v = v0 * t / 0.2;
			else if (t > 0.8)
				v = v0 * (1 - t) / (1 - 0.8);
			else
				v = v0;

			//getting parameter at specific time
			float u = inverseArcLengthAt(currentDistance, segment);

			currentDistance += deltaTime * v * speed;

			t += deltaTime * speed;
			if (t >= 1.0f)
			{
				t = 0.0f;
				currentDistance = 0.0f;
			}

			glm::vec3 translationPos;
			glm::vec3 rotationVec;
			glm::vec3 W;

			//calculating translation
			translationPos = mySpaceCurve(u, curvePoints[segment], curvePoints[segment + 1], curvePoints[segment + 2], curvePoints[segment + 3]);

			//calcualting rotation
			if (u + delta >= 1.0f)
				W = translationPos - mySpaceCurve(u - delta, curvePoints[segment], curvePoints[segment + 1], curvePoints[segment + 2], curvePoints[segment + 3]);
			else
				W = mySpaceCurve(u + delta, curvePoints[segment], curvePoints[segment + 1], curvePoints[segment + 2], curvePoints[segment + 3]) - translationPos;

			rotationVec = glm::normalize(W);

#pragma endregion

			glm::mat4 pathModel(1.0f);
			//pathModel = glm::translate(pathModel, glm::vec3(0.0f, -1.5f, 0.0f)); // translate it down so it's at the center of the scene

			glUseProgram(lampShader.mId);
			lampShader.SetUniform("model", &pathModel);
			lampShader.SetUniform("projection", &projection);
			lampShader.SetUniform("view", &view);

			glBindVertexArray(VAO);
			
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(plot[0]) * 700, &plot[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			
			glUseProgram(lampShader.mId);
			glBindVertexArray(VAO);
			//pOpenglRenderer->RenderCube();
			glPointSize(10.0f);
			glDrawArrays(GL_LINES, 0, 700);

			/*glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 10);*/

			pMesh = t > 0.05 && t < 0.95 ? &meshWalking : &meshIdle;

			speedFactor = t < 0.1f ? t / 0.1f :
						  t > 0.9f ? (1.0f - t) / 0.1f : 1.0f;

			speedFactor = 1.0f - speedFactor;

			// skinning
			tinystl::vector<aiMatrix4x4> Transforms, BoneTransforms;
			pMesh->BoneTransform(timer / 1000.0f, Transforms, BoneTransforms, speedFactor);
			

			model = glm::mat4(1.0f);
			//model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
			// for guard
			/*model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));*/
			/*model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0, 0.0, 0.0));
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
			model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0.0, 0.0, 1.0));*/
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f) + translationPos); // translate it down so it's at the center of the scene
			model = glm::rotate(model, glm::radians(30.0f) - acos(glm::dot(currAngle, rotationVec)), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(var, var, var));

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
				
				pMesh->Render();
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

				uint32_t size = (uint32_t)pMesh->mLineSegments.size();
				tinystl::vector<float> vertices(size * 2 * 3);

				for (uint32_t i = 1; i < size; ++i)
				{
					aiMatrix4x4 parent = pMesh->mLineSegments[i].mParent;
					aiMatrix4x4 child = pMesh->mLineSegments[i].mChild;

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
					ImGui::InputFloat("var", &var, 0.001f, 1.0f, 3.0f);
					
					ImGui::InputInt("Animation Index", &animationIndex);
					pMesh->SetCurrentAnimation(animationIndex);

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