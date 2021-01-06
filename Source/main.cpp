#include "../Externals/Include/Common.h"
#include "model.h"
#include "camera.h"
//#include "shader_m.h"
#include "mesh.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_DEFAULT 4
#define MENU_IMAGE_ABSTARCTION 5
#define MENU_WATERCOLOR 6
#define MENU_BLOOM_EFFECT 7
#define MENU_PIXELATION 8
#define MENU_SINE_WAVE_DISTORTION 9
#define MENU_MAGNIFIER 10
#define MENU_NORMAL 11

using namespace glm;
using namespace std;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

// window size setting
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// timing
float deltaTime = 0.1f;
float lastFrame = 0.0f;
unsigned int timerCount = 0;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// screen split
float slide_bar_x = 0.7;
float quad_border = 0.4;
float texture_border = 0.7;
float prevX = SCR_WIDTH / 2.0f;
float prevY = SCR_HEIGHT / 2.0f;
bool slideMode = false;
bool lbuttonPress = false;

// post-processing
int effectID = 1;
bool using_normal_color = false;
float magnifyCenter_x = SCR_WIDTH / 2;
float magnifyCenter_y = SCR_HEIGHT / 2;

// tessellation
float dmap_depth;
bool enable_height;
bool wireframe;
bool enable_fog;
// light position
vec3 light_position = vec3(-31.75, 126.05, 197.72);

// shader
Shader *castleShader;
Shader *soldierShader;
Shader *splatShader;
Shader *leftScreenShader;
Shader *rightScreenShader;
Shader *blurShader;
Shader *skyboxShader;
Shader *terrainShader;
Shader *depthShader;

// load models
// -----------
string castlePath = "Castle/Castle OBJ.obj";
Model *castleModel;
string soldierFiringPath = "soldier_firing/soldier_firing.obj";
Model *soldierFiringModel;
string splatPath = "Splat/Splat_01.obj";
Model *splatModel;

// skybox texture path
const char *skyboxTexPath[6] = { "..\\Assets\\cubemaps\\posx.jpg",
								 "..\\Assets\\cubemaps\\negx.jpg",
								 "..\\Assets\\cubemaps\\posy.jpg",
								 "..\\Assets\\cubemaps\\negy.jpg",
								 "..\\Assets\\cubemaps\\posz.jpg",
								 "..\\Assets\\cubemaps\\negz.jpg" };

// framebuffer vertice
float leftQuadVertices[] = {
		   -1.0f,  1.0f,           0.0f, 1.0f,
	 quad_border,  1.0f, texture_border, 1.0f,
	 quad_border, -1.0f, texture_border, 0.0f,

		   -1.0f,  1.0f,           0.0f, 1.0f,
		   -1.0f, -1.0f,           0.0f, 0.0f,
	 quad_border, -1.0f, texture_border, 0.0f,
};
float rightQuadVertices[] = {
	quad_border,  1.0f, texture_border, 1.0f,
		   1.0f,  1.0f,           1.0f, 1.0f,
		   1.0f, -1.0f,           1.0f, 0.0f,

	quad_border,  1.0f, texture_border, 1.0f,
	quad_border, -1.0f, texture_border, 0.0f,
		   1.0f, -1.0f,           1.0f, 0.0f,
};

// global variable
unsigned int leftQuadVAO, leftQuadVBO; // with post-processing
unsigned int rightQuadVAO, rightQuadVBO;
unsigned int skyboxVAO;
unsigned int noiseTexture;
unsigned int skyboxTexture;
unsigned int blurFramebuffer; // for first blur pass
unsigned int blurColorbuffer;
unsigned int framebuffer;
unsigned int textureColorbuffer;
unsigned int terrainVAO;
unsigned int terrainHeightTexture;
unsigned int terrainTexture;

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// screen left quad VAO
	glGenVertexArrays(1, &leftQuadVAO);
	glGenBuffers(1, &leftQuadVBO);
	glBindVertexArray(leftQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, leftQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(leftQuadVertices), &leftQuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	// screen right quad VAO
	glGenVertexArrays(1, &rightQuadVAO);
	glGenBuffers(1, &rightQuadVBO);
	glBindVertexArray(rightQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rightQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rightQuadVertices), &rightQuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// build and compile shaders
	// -------------------------
	// set shader uniform variable
	(*leftScreenShader).use();
	(*leftScreenShader).setInt("screenTexture", 0);
	(*leftScreenShader).setInt("noiseTexture", 1);
	(*leftScreenShader).setInt("blurScene", 2);
	(*leftScreenShader).setInt("effectID", effectID);
	(*leftScreenShader).setVec2("circlePos", glm::vec2(magnifyCenter_x, magnifyCenter_y));
	(*leftScreenShader).setFloat("circleRadius", 200);
	(*leftScreenShader).setFloat("zoomInRatio", 1.2);

	(*rightScreenShader).use();
	(*rightScreenShader).setInt("screenTexture", 0);

	// Load cubemap texture
	// -----------------------------------------
	(*skyboxShader).use();
	glGenTextures(1, &skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	for (int i = 0; i < 6; ++i)
	{
		texture_data image = loadImg(skyboxTexPath[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA,
			image.width, image.height,
			0, GL_RGBA, GL_UNSIGNED_BYTE,
			image.data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glGenVertexArrays(1, &skyboxVAO);

	// Terrain configuration
	// ---------------------
	dmap_depth = 6.0f;
	enable_height = true;
	wireframe = false;
	enable_fog = false;
	(*terrainShader).use();
	glGenVertexArrays(1, &terrainVAO);
	glBindVertexArray(terrainVAO);
	// Load Terrain texture
	// --------------------
	// height map
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &terrainHeightTexture);
	glBindTexture(GL_TEXTURE_2D, terrainHeightTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height, nrChannels;
	unsigned char *data = stbi_load("terragen.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
		std::cout << "Failed to load texture" << std::endl;
	stbi_image_free(data);

	// terrain color map
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &terrainTexture);
	glBindTexture(GL_TEXTURE_2D, terrainTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	data = stbi_load("terragen_color-2.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
		std::cout << "Failed to load texture" << std::endl;
	stbi_image_free(data);

	glPatchParameteri(GL_PATCH_VERTICES, 4);
	//glEnable(GL_CULL_FACE);
	glBindVertexArray(0);

	// Load noise texture, for watercolor effect
	// -----------------------------------------
	(*blurShader).use();
	(*blurShader).setInt("screenTexture", 0);

	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load image
	data = stbi_load("noise.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		std::cout << "Failed to load texture" << std::endl;

	stbi_image_free(data);

	// framebuffer configuration
	// -------------------------
	glGenFramebuffers(1, &blurFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, blurFramebuffer);
	// create color attachment texture
	glGenTextures(1, &blurColorbuffer);
	glBindTexture(GL_TEXTURE_2D, blurColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurColorbuffer, 0);

	// create renderbuffer object
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// -------------------------
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	// create color attachment texture
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	// create renderbuffer object
	
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	

}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update screen split position
	glBindBuffer(GL_ARRAY_BUFFER, leftQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(leftQuadVertices), &leftQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(leftQuadVertices), &leftQuadVertices);
	glBindBuffer(GL_ARRAY_BUFFER, rightQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rightQuadVertices), &rightQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rightQuadVertices), &rightQuadVertices);

	// view/projection transformations
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 180.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// render
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	(*skyboxShader).use();
	glBindVertexArray(skyboxVAO);
	(*skyboxShader).setInt("tex_cubemap", 0);

	glm::mat4 pv_matrix = projection * view;
	(*skyboxShader).setMat4("pv_matrix", pv_matrix);
	(*skyboxShader).setVec3("cam_pos", camera.Position);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);

	// Draw terrain
	static const GLfloat one = 1.0f;
	glClearBufferfv(GL_DEPTH, 0, &one);
	(*terrainShader).use();
	// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrainHeightTexture);
	(*terrainShader).setInt("tex_displacement", 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrainTexture);
	(*terrainShader).setInt("tex_color", 1);

	glBindVertexArray(terrainVAO);

	glm::mat4 terrain_model = glm::mat4(1.0f);
	terrain_model = glm::translate(terrain_model, glm::vec3(0.0f, -20.0f, 0.0f));
	terrain_model = glm::scale(terrain_model, glm::vec3(5.0f, 5.0f, 5.0f));
	(*terrainShader).setMat4("mv_matrix", view * terrain_model);
	(*terrainShader).setMat4("proj_matrix", projection);
	(*terrainShader).setMat4("mvp_matrix", projection * view * terrain_model);
	(*terrainShader).setFloat("dmap_depth", enable_height ? dmap_depth : 0.0f);
	(*terrainShader).setBool("enable_fog", enable_fog ? 1 : 0);
	(*terrainShader).setInt("tex_color", 1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);
	glBindVertexArray(0);

	// render the loaded castle model
	// ------------------------------
	(*castleShader).use();
	// use normal color
	if (using_normal_color)
		(*castleShader).setBool("using_normal_color", 1);
	else
		(*castleShader).setBool("using_normal_color", 0);

	(*castleShader).setMat4("projection", projection);
	(*castleShader).setMat4("view", view);
	(*castleShader).setVec3("light_pos", light_position);
	(*castleShader).setVec3("eye_pos", camera.Position);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	(*castleShader).setMat4("model", model);
	(*castleModel).Draw((*castleShader));

	// render the loaded soldier firing model
	// --------------------------------------
	(*soldierShader).use();
	// use normal color
	if (using_normal_color)
		(*soldierShader).setBool("using_normal_color", 1);
	else
		(*soldierShader).setBool("using_normal_color", 0);

	(*soldierShader).setMat4("projection", projection);
	(*soldierShader).setMat4("view", view);
	(*soldierShader).setVec3("light_pos", light_position);
	(*soldierShader).setVec3("eye_pos", camera.Position);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-23.0f, 6.75f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	(*soldierShader).setMat4("model", model);
	(*soldierFiringModel).Draw((*soldierShader));

	// render the loaded splat model
	// --------------------------------------
	(*splatShader).use();
	// use normal color
	if (using_normal_color)
		(*splatShader).setBool("using_normal_color", 1);
	else
		(*splatShader).setBool("using_normal_color", 0);

	(*splatShader).setMat4("projection", projection);
	(*splatShader).setMat4("view", view);
	(*splatShader).setVec3("light_pos", light_position);
	(*splatShader).setVec3("eye_pos", camera.Position);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	(*skyboxShader).setInt("tex_cubemap", 0);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-13.0f, -1.5f, -5.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.3f, 0.1f, 0.3f));
	(*splatShader).setMat4("model", model);
	(*splatModel).Draw((*splatShader));

	// bind back to default framebuffer
	// --------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (effectID == 3) { // for bloom effect, first blur pass
		(*blurShader).use();
		glBindFramebuffer(GL_FRAMEBUFFER, blurFramebuffer); // bind blur frame buffer
		glBindVertexArray(leftQuadVAO);
		glActiveTexture(GL_TEXTURE0);
		// use color attachment texture of main frame buffer as texture of blur frame buffer
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if (effectID == 6) { // magnify effect
		(*leftScreenShader).use();
		(*leftScreenShader).setVec2("circlePos", glm::vec2(magnifyCenter_x, magnifyCenter_y)); // update mouse clik position
	}

	(*leftScreenShader).use();
	(*leftScreenShader).setInt("effectID", effectID); // update effect ID
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	float timeValue = timeSinceStart / 1000.0;
	(*leftScreenShader).setFloat("time", timeValue); // for sin wave
	glBindVertexArray(leftQuadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer); // use the color attachment texture as the texture of the quad plane
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, noiseTexture); // for watercolor effect
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, blurColorbuffer); // for bloom effect, first blur pass
	glDrawArrays(GL_TRIANGLES, 0, 6);

	(*rightScreenShader).use();
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(rightQuadVAO);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
}

void My_Timer(int val)
{
	timer_cnt++;
	timerCount++;
	glutPostRedisplay();
	if (timer_enabled)
		glutTimerFunc(timer_speed, My_Timer, val);
}

void modify_slider_setting() {
	leftQuadVertices[4] = quad_border;
	leftQuadVertices[8] = quad_border;
	leftQuadVertices[20] = quad_border;
	leftQuadVertices[6] = texture_border;
	leftQuadVertices[10] = texture_border;
	leftQuadVertices[22] = texture_border;
	rightQuadVertices[0] = quad_border;
	rightQuadVertices[12] = quad_border;
	rightQuadVertices[16] = quad_border;
	rightQuadVertices[2] = texture_border;
	rightQuadVertices[14] = texture_border;
	rightQuadVertices[18] = texture_border;
}
void My_Mouse_Move(int x, int y)
{
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}
	lastX = x;
	lastY = y;
}

void My_Mouse_Motion(int x, int y)
{
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}
	float xoffset = x - lastX;
	float yoffset = lastY - y;

	if (lbuttonPress) {
		xoffset = x - prevX;
		yoffset = prevY - y;
		prevX = x;
		prevY = y;
		if (slideMode) {
			float slide_offset = xoffset / SCR_WIDTH;
			slide_bar_x += slide_offset;
			texture_border += slide_offset;
			quad_border = 2 * slide_bar_x - 1;

			if (slide_bar_x >= 1 || quad_border >= 1 || texture_border >= 1) { // avoid sider bar out of border
				slide_bar_x = 1;
				quad_border = 1;
				texture_border = 1;
			}
			if (slide_bar_x <= 0 || quad_border <= -1 || texture_border <= 0) { // avoid sider bar out of border
				slide_bar_x = 0;
				quad_border = -1;
				texture_border = 0;
			}
			modify_slider_setting();
		}
		else {
			camera.ProcessMouseMovement(xoffset, yoffset); // change look direction
		}
	}
	lastX = x;
	lastY = y;
	glutPostRedisplay();
}

void My_Mouse(int button, int state, int x, int y)
{
	float xoffset = 0;
	float yoffset = 0;
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			if (lbuttonPress == false) {
				prevX = lastX;
				prevY = lastY;
				float xpos = prevX / SCR_WIDTH;
				if (xpos < slide_bar_x + 0.05 && xpos > slide_bar_x - 0.05)
					slideMode = true;
				else
					slideMode = false;
				if (effectID == 6) {
					magnifyCenter_x = lastX;
					magnifyCenter_y = lastY;
				}
			}
			lbuttonPress = true;
		}
		else if (state == GLUT_UP) {
			lbuttonPress = false;
		}
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'w':
			camera.ProcessKeyboard(FORWARD, deltaTime);
			break;
		case 's':
			camera.ProcessKeyboard(BACKWARD, deltaTime);
			break;
		case 'a':
			camera.ProcessKeyboard(LEFT, deltaTime);
			break;
		case 'd':
			camera.ProcessKeyboard(RIGHT, deltaTime);
			break;
		case 'z':
			camera.ProcessKeyboard(UP, deltaTime);
			break;
		case 'x':
			camera.ProcessKeyboard(DOWN, deltaTime);
			break;
		case 'q':
			effectID -= 1;
			effectID += 7;
			effectID %= 7;
			break;
		case 'e':
			effectID += 1;
			effectID += 7;
			effectID %= 7;
			break;
		case 'n':
			using_normal_color = !using_normal_color;
			break;
		default:
			cout << "Nothing" << endl;
			break;
	}
	glutPostRedisplay();
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_DEFAULT:
		effectID = 0;
		break;
	case MENU_IMAGE_ABSTARCTION:
		effectID = 1;
		break;
	case MENU_WATERCOLOR:
		effectID = 2;
		break;
	case MENU_BLOOM_EFFECT:
		effectID = 3;
		break;
	case MENU_PIXELATION:
		effectID = 4;
		break;
	case MENU_SINE_WAVE_DISTORTION:
		effectID = 5;
		break;
	case MENU_MAGNIFIER:
		effectID = 6;
		break;
	case MENU_NORMAL:
		using_normal_color = !using_normal_color;
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("Final_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();

	Shader s_castle("model_loading.vs", "model_loading.fs");
	castleShader = &s_castle;
	Shader s_solider("soldier_model.vs", "soldier_model.fs");
	soldierShader = &s_solider;
	Shader s_splat("splat_model.vs", "splat_model.fs");
	splatShader = &s_splat;
	Shader s_leftScreen("framebuffers_screen.vs", "post_processing.fs");
	leftScreenShader = &s_leftScreen;
	Shader s_rightScreen("framebuffers_screen.vs", "framebuffers_screen.fs");
	rightScreenShader = &s_rightScreen;
	Shader s_blur("framebuffers_screen.vs", "blur.fs");
	blurShader = &s_blur;
	Shader s_skybox("skybox.vs.glsl", "skybox.fs.glsl");
	skyboxShader = &s_skybox;
	Shader s_terrain("terrain.vs", "terrain.fs", "terrain.tcs", "terrain.tes");
	terrainShader = &s_terrain;
	Shader s_depth("depth.vs.glsl", "depth.fs.glsl");
	depthShader = &s_depth;

	Model m_castle(castlePath);
	castleModel = &m_castle;
	Model m_soldierFiring(soldierFiringPath);
	soldierFiringModel = &m_soldierFiring;
	Model m_splat(splatPath);
	splatModel = &m_splat;

	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int menu_effect = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Effect", menu_effect);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_effect);
	glutAddMenuEntry("Default", MENU_DEFAULT);
	glutAddMenuEntry("Image Abstraction", MENU_IMAGE_ABSTARCTION);
	glutAddMenuEntry("Watercolor", MENU_WATERCOLOR);
	glutAddMenuEntry("Bloom Effect", MENU_BLOOM_EFFECT);
	glutAddMenuEntry("Pixelation", MENU_PIXELATION);
	glutAddMenuEntry("Sine Wave Distortion", MENU_SINE_WAVE_DISTORTION);
	glutAddMenuEntry("Magnifier", MENU_MAGNIFIER);
	glutAddMenuEntry("Normal", MENU_NORMAL);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_Mouse_Motion);
	glutPassiveMotionFunc(My_Mouse_Move);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
