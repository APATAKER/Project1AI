#include "commonHeaders.h"
#include "ProjectStuff/openGLStuff.h"
#include "ModelLoading/cModelLoader.h"
#include "shader/cShaderManager.h"
#include "VAO/cVAOManager.h"
#include "Textures/cBasicTextureManager.h"
#include "DebugRenderer/cDebugRenderer.h"
#include "GameObject/cGameObject.h"
#include "FlyCamera/cFlyCamera.h"
#include "DeltaTime/cLowPassFilter.h"
#include "JsonLoader/cLoad.h"


cBasicTextureManager* g_pTextureManager = nullptr;
GLFWwindow* window = nullptr;
cDebugRenderer* g_pDebugRenderer = nullptr;
cFlyCamera* g_pFlyCamera = nullptr;
cLowPassFilter* avgDeltaTimeThingy = nullptr;

glm::vec3 LightPosition = glm::vec3(-25.0f, 300.0f, -150.0f);
float LightConstAtten = 0.0000001f;			
float LightLinearAtten = 0.00119f;
float LightQuadraticAtten = 9.21e-7f;
float LightSpotInnerAngle = 5.0f;
float LightSpotOuterAngle = 7.5f;
glm::vec3 LightSpotDirection = glm::vec3(0.0f, -1.0f, 0.0f);


std::vector<cGameObject*> g_vec_pGameObjects;


void DrawObject(glm::mat4 matWorld,
	cGameObject* pCurrentObject,
	GLint shaderProgID,
	cVAOManager* pVAOManager);
glm::mat4 calculateWorldMatrix(cGameObject* pCurrentObject, glm::mat4 matWorld);
void SetUpTextureBindingsForObject(
	cGameObject* pCurrentObject,
	GLint shaderProgID);


int main()
{
	window = creatOpenGL(window);

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	std::cout << "OpenGL version: " << major << "." << minor << std::endl;

	cDebugRenderer* g_pDebugRenderer = new cDebugRenderer();
	if (!g_pDebugRenderer->initialize())
	{
		std::cout << "Error init on DebugShader: " << g_pDebugRenderer->getLastError() << std::endl;
	}


	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

	std::string jsonFilename = "Config/config.json";
	rapidjson::Document document = cJSONUtility::open_document(jsonFilename);

	
	///######################## MODEL #### LOADING ##### STARTS ### HERE ##########################################
	cModelLoader* pTheModelLoader = new cModelLoader();


	// Models Loaded here
	
	std::string errorString = "";

	size_t numModels = document["models"].Size();

	std::vector<cMesh> vModelMesh;
	for(size_t c = 0;c<numModels;c++)
	{
		cMesh Mesh;
		if (pTheModelLoader->LoadModel_Assimp(document["models"][c].GetString(), Mesh, errorString))
		{
			std::cout << document["models"][c].GetString()<< " model loaded" << std::endl;
			vModelMesh.push_back(Mesh);
		}
		else
		{
			std::cout << "\nerror:" << errorString << std::endl;
		}
	}

	///######################## MODEL #### LOADING ##### ENDS ### HERE ##########################################

	///######################## SHADER #### LOADING ## STARTS ### HERE #############################################
	cShaderManager* pTheShaderManager = new cShaderManager();

	//std::cout <<  << std::endl;

	cShaderManager::cShader vertexShad;
	vertexShad.fileName = document["shaders"]["vert"].GetString();

	cShaderManager::cShader fragShader;
	fragShader.fileName = document["shaders"]["frag"].GetString();

	if (!pTheShaderManager->createProgramFromFile("SimpleShader", vertexShad, fragShader))
	{
		std::cout << "Error: didn't compile the shader" << std::endl;
		std::cout << pTheShaderManager->getLastError();
		return -1;
	}
	GLuint shaderProgID = pTheShaderManager->getIDFromFriendlyName("SimpleShader");
	///######################## SHADER #### LOADING ## ENDS ### HERE #############################################


	//##### MODELS ### LOADING ### INTO ### VERTEX ### ARRAY ### OBJECT #### (DATA PUSHED INTO SHADER CODE PART)###########

	//cVAOManager* pTheVAOManager = new cVAOManager();
	cVAOManager* pTheVAOManager = cVAOManager::getInstance();
	// Singleton done Here


	for(size_t c = 0;c<numModels;c++)
	{
		sModelDrawInfo drawInfo;
		pTheVAOManager->LoadModelIntoVAO(document["MeshName"][c].GetString(), vModelMesh[c], drawInfo, shaderProgID);
	}


	//##### MODELS ### LOADING ### INTO ### VERTEX ### ARRAY ### OBJECT #### (DATA PUSHED INTO SHADER CODE PART)###########

	
	// Loading Textures

	::g_pTextureManager = new cBasicTextureManager();
	::g_pTextureManager->SetBasePath("assets/textures");
	// Normal Textures
	size_t numTex = document["Textures"].Size();
	for(size_t c=0;c<numTex;c++)
	{
		if (!::g_pTextureManager->Create2DTextureFromBMPFile(document["Textures"][c].GetString(), true))
		{
			std::cout << "\nDidn't load texture" << std::endl;
		}
	}
	// CubeMap Texture
	::g_pTextureManager->SetBasePath("assets/textures/cubemaps/");

	if (::g_pTextureManager->CreateCubeTextureFromBMPFiles("space",
		"SpaceBox_right1_posX.bmp", "SpaceBox_left2_negX.bmp",
		"SpaceBox_top3_posY.bmp", "SpaceBox_bottom4_negY.bmp",
		"SpaceBox_front5_posZ.bmp", "SpaceBox_back6_negZ.bmp", true, errorString))
		std::cout << "\nSpace skybox loaded" << std::endl;
	else
		std::cout << "\nskybox error: " << errorString << std::endl;
	
	// Loading Textures


	//##### GAME ### OBJECTS ### TO ### CREATED ### HERE ##################################################################

	size_t numGameObjects = document["GameObjects"].Size();
	for(size_t c=0;c<numGameObjects;c++)
	{
		cGameObject* gameobject = new cGameObject();
		rapidjson::Value& jgameobj = document["GameObjects"][c];
		
		gameobject->meshName = jgameobj["meshname"].GetString();
		gameobject->friendlyName = jgameobj["friendlyname"].GetString();
		gameobject->positionXYZ = glm::vec3(jgameobj["position"]["x"].GetFloat(),
											jgameobj["position"]["y"].GetFloat(),
											jgameobj["position"]["z"].GetFloat());
		gameobject->setOrientation(glm::vec3(
											jgameobj["rotation"]["x"].GetFloat(),
											jgameobj["rotation"]["y"].GetFloat(),
											jgameobj["rotation"]["z"].GetFloat()));
		gameobject->scale = jgameobj["scale"].GetFloat();
		gameobject->objectColourRGBA = glm::vec4(jgameobj["objectcolor"]["r"].GetFloat(),
												jgameobj["objectcolor"]["g"].GetFloat(),
												jgameobj["objectcolor"]["b"].GetFloat(),
												jgameobj["objectcolor"]["a"].GetFloat());
		gameobject->debugColour = glm::vec4(jgameobj["debugcolor"]["r"].GetFloat(),
											jgameobj["debugcolor"]["g"].GetFloat(),
											jgameobj["debugcolor"]["b"].GetFloat(),
											jgameobj["debugcolor"]["a"].GetFloat());
		for(int i=0;i<jgameobj["tex"].Size();i++)
		{
			gameobject->textures[i] = jgameobj["tex"][i].GetString();
			gameobject->textureRatio[i] = jgameobj["texratio"][i].GetFloat();
		}
		g_vec_pGameObjects.push_back(gameobject);
	}

	//##### GAME ### OBJECTS ### TO ### CREATED ### HERE ##################################################################


	// Camera Created here
	::g_pFlyCamera = new cFlyCamera();
	::g_pFlyCamera->eye = glm::vec3(0.0f, 65.0, -340.0);
	::g_pFlyCamera->movementSpeed = 0.25f;
	::g_pFlyCamera->movementSpeed = 2.5f;
	// Camera Created here

	glEnable(GL_DEPTH);			// Write to the depth buffer
	glEnable(GL_DEPTH_TEST);	// Test with buffer when drawing

	// Calculating DeltaTime
	// Get the initial time	
	cLowPassFilter avgDeltaTimeThingy;

	// Get the initial time
	double lastTime = glfwGetTime();
	// Calculating DeltaTime


	
	while (!glfwWindowShouldClose(window))
	{
		// Updating DeltaTime
			// Get the initial time
		double currentTime = glfwGetTime();

		// Frame time... (how many seconds since last frame)
		double deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		const double SOME_HUGE_TIME = 0.1;	// 100 ms;
		if (deltaTime > SOME_HUGE_TIME)
		{
			deltaTime = SOME_HUGE_TIME;
		}

		avgDeltaTimeThingy.addValue(deltaTime);
		// Updating DeltaTime

		ProcessAsyncKeys(window);
		ProcessAsyncMouse(window);

		glUseProgram(shaderProgID);

		float ratio;
		int width, height;
		glm::mat4 p, v;


		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		// Projection matrix
		p = glm::perspective(0.6f,		// FOV
			ratio,			// Aspect ratio
			1.0f,			// Near clipping plane
			10000.0f);		// Far clipping plane

		// View matrix
		v = glm::mat4(1.0f);
		v = glm::lookAt(::g_pFlyCamera->getEye(),
			::g_pFlyCamera->getAtInWorldSpace(),
			::g_pFlyCamera->getUpVector());

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//lights into shader
		GLint L_0_position = glGetUniformLocation(shaderProgID, "theLights[0].position");
		GLint L_0_diffuse = glGetUniformLocation(shaderProgID, "theLights[0].diffuse");
		GLint L_0_specular = glGetUniformLocation(shaderProgID, "theLights[0].specular");
		GLint L_0_atten = glGetUniformLocation(shaderProgID, "theLights[0].atten");
		GLint L_0_direction = glGetUniformLocation(shaderProgID, "theLights[0].direction");
		GLint L_0_param1 = glGetUniformLocation(shaderProgID, "theLights[0].param1");
		GLint L_0_param2 = glGetUniformLocation(shaderProgID, "theLights[0].param2");

		glUniform4f(L_0_position,
			LightPosition.x,
			LightPosition.y,
			LightPosition.z,
			1.0f);
		glUniform4f(L_0_diffuse, 1.0f, 1.0f, 1.0f, 1.0f);	// White
		glUniform4f(L_0_specular, 1.0f, 1.0f, 1.0f, 1.0f);	// White
		glUniform4f(L_0_atten, 0.0f,  // constant attenuation
			LightLinearAtten,  // Linear 
			LightQuadraticAtten,	// Quadratic 
			1000000.0f);	// Distance cut off
// Point light:
		glUniform4f(L_0_param1, 0.0f /*POINT light*/, 0.0f, 0.0f, 1.0f);
		glUniform4f(L_0_param2, 1.0f /*Light is on*/, 0.0f, 0.0f, 1.0f);


		GLint eyeLocation_UL = glGetUniformLocation(shaderProgID, "eyeLocation");
		glUniform4f(eyeLocation_UL,
			::g_pFlyCamera->getEye().x,
			::g_pFlyCamera->getEye().y,
			::g_pFlyCamera->getEye().z, 1.0f);

		GLint matView_UL = glGetUniformLocation(shaderProgID, "matView");
		GLint matProj_UL = glGetUniformLocation(shaderProgID, "matProj");

		glUniformMatrix4fv(matView_UL, 1, GL_FALSE, glm::value_ptr(v));
		glUniformMatrix4fv(matProj_UL, 1, GL_FALSE, glm::value_ptr(p));

		std::stringstream ssTitle;
		ssTitle
			<< g_pFlyCamera->eye.x << ", "
			<< g_pFlyCamera->eye.y << ", "
			<< g_pFlyCamera->eye.z
			<< "object postion: "
			<< g_vec_pGameObjects[0]->positionXYZ.x << " : "
			<< g_vec_pGameObjects[0]->positionXYZ.y<< " : "
			<< g_vec_pGameObjects[0]->positionXYZ.z;
		glfwSetWindowTitle(window, ssTitle.str().c_str());

		for (int index = 0; index != ::g_vec_pGameObjects.size(); index++)
		{

			cGameObject* pCurrentObject = ::g_vec_pGameObjects[index];

			glm::mat4 matModel = glm::mat4(1.0f);	// Identity matrix

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...



		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	
	glfwDestroyWindow(window);
	glfwTerminate();

	delete pTheModelLoader;
	delete pTheShaderManager; 
	delete pTheVAOManager;
	delete g_pFlyCamera;
	delete g_pDebugRenderer;
	for (int i = 0; i < g_vec_pGameObjects.size(); i++)
		delete g_vec_pGameObjects[i];

	exit(EXIT_SUCCESS);

	return 0;
}

void DrawObject(glm::mat4 matModel,
	cGameObject* pCurrentObject,
	GLint shaderProgID,
	cVAOManager* pVAOManager)
{

	if (pCurrentObject->isVisible == false)
	{
		return;
	}


	// Turns on "alpha transparency"
	glEnable(GL_BLEND);

	// Reads what's on the buffer already, and 
	// blends it with the incoming colour based on the "alpha" 
	// value, which is the 4th colour output
	// RGB+A
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// This block of code, where I generate the world matrix from the 
	// position, scale, and rotation (orientation) of the object
	// has been placed into calculateWorldMatrix()


	// ************ 
	// Set the texture bindings and samplers

	// See if this is a skybox object? 
	GLint bIsSkyBox_UL = glGetUniformLocation(shaderProgID, "bIsSkyBox");
	if (pCurrentObject->friendlyName != "skybox")
	{
		// Is a regular 2D textured object
		SetUpTextureBindingsForObject(pCurrentObject, shaderProgID);
		glUniform1f(bIsSkyBox_UL, (float)GL_FALSE);

		// Don't draw back facing triangles (default)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);		// Don't draw "back facing" triangles
	}
	else
	{
		// Draw the back facing triangles. 
		// Because we are inside the object, so it will force a draw on the "back" of the sphere 
		//glCullFace(GL_FRONT_AND_BACK);
		glDisable(GL_CULL_FACE);	// Draw everything

		glUniform1f(bIsSkyBox_UL, (float)GL_TRUE);

		GLuint skyBoxTextureID = ::g_pTextureManager->getTextureIDFromName("space");
		glActiveTexture(GL_TEXTURE26);				// Texture Unit 26
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTextureID);	// Texture now assoc with texture unit 0

		// Tie the texture units to the samplers in the shader
		GLint skyBoxSampler_UL = glGetUniformLocation(shaderProgID, "skyBox");
		glUniform1i(skyBoxSampler_UL, 26);	// Texture unit 26
	}


	// ************


	//glm::mat4 matWorld = glm::mat4(1.0f);	// identity matry

	glm::mat4 matWorldCurrentGO = calculateWorldMatrix(pCurrentObject, matModel);



	//uniform mat4 matModel;		// Model or World 
	//uniform mat4 matView; 		// View or camera
	//uniform mat4 matProj;
	GLint matModel_UL = glGetUniformLocation(shaderProgID, "matModel");

	glUniformMatrix4fv(matModel_UL, 1, GL_FALSE, glm::value_ptr(matWorldCurrentGO));
	//glUniformMatrix4fv(matView_UL, 1, GL_FALSE, glm::value_ptr(v));
	//glUniformMatrix4fv(matProj_UL, 1, GL_FALSE, glm::value_ptr(p));

	// Calcualte the inverse transpose of the model matrix and pass that...
	// Stripping away scaling and translation, leaving only rotation
	// Because the normal is only a direction, really
	GLint matModelIT_UL = glGetUniformLocation(shaderProgID, "matModelInverseTranspose");
	glm::mat4 matModelInverseTranspose = glm::inverse(glm::transpose(matWorldCurrentGO));
	glUniformMatrix4fv(matModelIT_UL, 1, GL_FALSE, glm::value_ptr(matModelInverseTranspose));




	// Find the location of the uniform variable newColour
	GLint newColour_location = glGetUniformLocation(shaderProgID, "newColour");

	glUniform3f(newColour_location,
		pCurrentObject->objectColourRGBA.r,
		pCurrentObject->objectColourRGBA.g,
		pCurrentObject->objectColourRGBA.b);


	GLint diffuseColour_UL = glGetUniformLocation(shaderProgID, "diffuseColour");
	glUniform4f(diffuseColour_UL,
		pCurrentObject->objectColourRGBA.r,
		pCurrentObject->objectColourRGBA.g,
		pCurrentObject->objectColourRGBA.b,
		pCurrentObject->alphaTransparency);	// *********

	GLint specularColour_UL = glGetUniformLocation(shaderProgID, "specularColour");
	glUniform4f(specularColour_UL,
		1.0f,	// R
		1.0f,	// G
		1.0f,	// B
		1000.0f);	// Specular "power" (how shinny the object is)
					// 1.0 to really big (10000.0f)


//uniform vec4 debugColour;
//uniform bool bDoNotLight;
	GLint debugColour_UL = glGetUniformLocation(shaderProgID, "debugColour");
	GLint bDoNotLight_UL = glGetUniformLocation(shaderProgID, "bDoNotLight");

	if (pCurrentObject->isWireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);		// LINES
		glUniform4f(debugColour_UL,
			pCurrentObject->debugColour.r,
			pCurrentObject->debugColour.g,
			pCurrentObject->debugColour.b,
			pCurrentObject->debugColour.a);
		glUniform1f(bDoNotLight_UL, (float)GL_TRUE);
	}
	else
	{	// Regular object (lit and not wireframe)
		glUniform1f(bDoNotLight_UL, (float)GL_FALSE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		// SOLID
	}
	//glPointSize(15.0f);

	if (pCurrentObject->disableDepthBufferTest)
	{
		glDisable(GL_DEPTH_TEST);					// DEPTH Test OFF
	}
	else
	{
		glEnable(GL_DEPTH_TEST);						// Turn ON depth test
	}

	if (pCurrentObject->disableDepthBufferWrite)
	{
		glDisable(GL_DEPTH);						// DON'T Write to depth buffer
	}
	else
	{
		glEnable(GL_DEPTH);								// Write to depth buffer
	}


	//		glDrawArrays(GL_TRIANGLES, 0, 2844);
	//		glDrawArrays(GL_TRIANGLES, 0, numberOfVertsOnGPU);

	sModelDrawInfo drawInfo;
	//if (pTheVAOManager->FindDrawInfoByModelName("bunny", drawInfo))
	if (pVAOManager->FindDrawInfoByModelName(pCurrentObject->meshName, drawInfo))
	{
		glBindVertexArray(drawInfo.VAO_ID);
		glDrawElements(GL_TRIANGLES,
			drawInfo.numberOfIndices,
			GL_UNSIGNED_INT,
			0);
		glBindVertexArray(0);
	}


	// Draw any child objects...
	for (std::vector<cGameObject*>::iterator itCGO = pCurrentObject->vec_pChildObjects.begin();
		itCGO != pCurrentObject->vec_pChildObjects.end(); itCGO++)
	{
		// I'm passing in the current game object matrix... 
		cGameObject* pChildGO = *itCGO;

		// NOTE: Scale of the parent object will mess around 
		//	with the translations (and later scaling) of the child object.
		float inverseScale = 1.0f / pCurrentObject->scale;
		glm::mat4 matInverseScale = glm::scale(glm::mat4(1.0f),
			glm::vec3(inverseScale, inverseScale, inverseScale));

		// Apply the inverse of the parent's scale to the matrix, 
		// leaving only tranlation and rotation
		glm::mat4 matParentNoScale = matWorldCurrentGO * matInverseScale;


		DrawObject(matParentNoScale, pChildGO, shaderProgID, pVAOManager);
	}//for (std::vector<cGameObject*>::iterator itCGO




	return;
} // DrawObject;

void SetUpTextureBindingsForObject(
	cGameObject* pCurrentObject,
	GLint shaderProgID)
{

	//// Tie the texture to the texture unit
	//GLuint texSamp0_UL = ::g_pTextureManager->getTextureIDFromName("Pizza.bmp");
	//glActiveTexture(GL_TEXTURE0);				// Texture Unit 0
	//glBindTexture(GL_TEXTURE_2D, texSamp0_UL);	// Texture now assoc with texture unit 0

	// Tie the texture to the texture unit
	GLuint texSamp0_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->textures[0]);
	glActiveTexture(GL_TEXTURE0);				// Texture Unit 0
	glBindTexture(GL_TEXTURE_2D, texSamp0_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp1_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->textures[1]);
	glActiveTexture(GL_TEXTURE1);				// Texture Unit 1
	glBindTexture(GL_TEXTURE_2D, texSamp1_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp2_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->textures[2]);
	glActiveTexture(GL_TEXTURE2);				// Texture Unit 2
	glBindTexture(GL_TEXTURE_2D, texSamp2_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp3_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->textures[3]);
	glActiveTexture(GL_TEXTURE3);				// Texture Unit 3
	glBindTexture(GL_TEXTURE_2D, texSamp3_UL);	// Texture now assoc with texture unit 0

	// Tie the texture units to the samplers in the shader
	GLint textSamp00_UL = glGetUniformLocation(shaderProgID, "textSamp00");
	glUniform1i(textSamp00_UL, 0);	// Texture unit 0

	GLint textSamp01_UL = glGetUniformLocation(shaderProgID, "textSamp01");
	glUniform1i(textSamp01_UL, 1);	// Texture unit 1

	GLint textSamp02_UL = glGetUniformLocation(shaderProgID, "textSamp02");
	glUniform1i(textSamp02_UL, 2);	// Texture unit 2

	GLint textSamp03_UL = glGetUniformLocation(shaderProgID, "textSamp03");
	glUniform1i(textSamp03_UL, 3);	// Texture unit 3


	GLint tex0_ratio_UL = glGetUniformLocation(shaderProgID, "tex_0_3_ratio");
	glUniform4f(tex0_ratio_UL,
		pCurrentObject->textureRatio[0],		// 1.0
		pCurrentObject->textureRatio[1],
		pCurrentObject->textureRatio[2],
		pCurrentObject->textureRatio[3]);

	//{
	//	//textureWhatTheWhat
	//	GLuint texSampWHAT_ID = ::g_pTextureManager->getTextureIDFromName("WhatTheWhat.bmp");
	//	glActiveTexture(GL_TEXTURE13);				// Texture Unit 13
	//	glBindTexture(GL_TEXTURE_2D, texSampWHAT_ID);	// Texture now assoc with texture unit 0

	//	GLint textureWhatTheWhat_UL = glGetUniformLocation(shaderProgID, "textureWhatTheWhat");
	//	glUniform1i(textureWhatTheWhat_UL, 13);	// Texture unit 13
	//}



	return;
}

glm::mat4 calculateWorldMatrix(cGameObject* pCurrentObject, glm::mat4 matWorld)
{

	//glm::mat4 matWorld = glm::mat4(1.0f);


	// ******* TRANSLATION TRANSFORM *********
	glm::mat4 matTrans
		= glm::translate(glm::mat4(1.0f),
			glm::vec3(pCurrentObject->positionXYZ.x,
				pCurrentObject->positionXYZ.y,
				pCurrentObject->positionXYZ.z));
	matWorld = matWorld * matTrans;
	// ******* TRANSLATION TRANSFORM *********

	//// ******* ROTATION TRANSFORM *********
	//if ( pCurrentObject->friendlyName != "StarDestroyer" )
	//{ 
	//	//mat4x4_rotate_Z(m, m, (float) glfwGetTime());
	//	glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f),
	//									pCurrentObject->rotationXYZ.z,					// Angle 
	//									glm::vec3(0.0f, 0.0f, 1.0f));
	//	matWorld = matWorld * rotateZ;
//
	//	glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f),
	//									pCurrentObject->rotationXYZ.y,	//(float)glfwGetTime(),					// Angle 
	//									glm::vec3(0.0f, 1.0f, 0.0f));
	//	matWorld = matWorld * rotateY;
//
	//	glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f),
	//									pCurrentObject->rotationXYZ.x,	// (float)glfwGetTime(),					// Angle 
	//									glm::vec3(1.0f, 0.0f, 0.0f));
	//	matWorld = matWorld * rotateX;
	//}
	//else
	//{
	//	// HACK: Adjust the rotation of the ship.
	//	glm::quat qAdjust = glm::quat( glm::vec3(0.0f, glm::radians(0.1f), 0.0f )); 
	//	pCurrentObject->qRotation = pCurrentObject->qRotation * qAdjust;

	glm::mat4 matRotation = glm::mat4(pCurrentObject->getQOrientation());
	matWorld = matWorld * matRotation;
	//}
	// ******* ROTATION TRANSFORM *********

	// ******* SCALE TRANSFORM *********
	glm::mat4 scale = glm::scale(glm::mat4(1.0f),
		glm::vec3(pCurrentObject->scale,
			pCurrentObject->scale,
			pCurrentObject->scale));
	matWorld = matWorld * scale;
	// ******* SCALE TRANSFORM *********


	return matWorld;
}