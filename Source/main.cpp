#include "../Externals/Include/Common.h"
#include "model.h"
#include "camera.h"
#include "csm_helper.h"
//#include "shader_m.h"
#include "mesh.h"
#include "time.h"

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

#define SHADOW_MAP_WIDTH 7680
#define SHADOW_MAP_HEIGHT 4320
#define NUM_CSM 3

using namespace glm;
using namespace std;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

// window size setting
unsigned int SCR_WIDTH = 1280; // const
unsigned int SCR_HEIGHT = 720; // const

// timing
float deltaTime = 0.1f;
float lastFrame = 0.0f;
unsigned int timerCount = 0;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
mat4 projection, view;

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
bool display_normal_mapping = false;
bool enable_cascade_shadow = true;
float magnifyCenter_x = SCR_WIDTH / 2;
float magnifyCenter_y = SCR_HEIGHT / 2;

// tessellation
float dmap_depth;
bool enable_height;
bool wireframe;
bool enable_fog;

// ssao
GLuint ssao_vao;
GLuint kernal_ubo;
GLuint noise_map;
GLuint ssao_fbo;
GLuint ssao_rbo;
GLuint ssao_tex;
bool ssao = false;
struct
{
	GLuint fbo;
	GLuint normal_map;
	GLuint depth_map;
} gbuffer;

// model_matrix
mat4 model_castle;
mat4 model_splat;
mat4 model_soldier;
mat4 terrain_model;
vector<mat4> model_matrixs;

// light position
//vec3 light_position = vec3(-50, 45, 76);// vec3(55, 51, 65);
vec3 light_position = vec3(50, 50, 0);// vec3(55, 51, 65);

//depth
mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f)) *scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
const float shadow_range = 75.0f;
mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 1000.0f);
mat4 light_view_matrix = lookAt(light_position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;
struct {
	GLuint fbo;
	GLuint depthMap;
} shadowBuffer;

// cascade shadow
vector<mat4> light_matrices;
vector<mat4> shadow_sbpv_matrices;
vector<frame_t> depth_frames;
float csm_range[NUM_CSM + 1];
float csm_range_C[NUM_CSM];

// shader
Shader *castleShader;
Shader *soldierShader;
Shader *splatShader;
Shader *leftScreenShader;
Shader *rightScreenShader;
Shader *blurShader;
Shader *skyboxShader;
Shader* terrainShader;
Shader *depthShader;
Shader *ssaoShader;
vector<Shader*> Shaders;

// load models
// -----------
string castlePath = "Castle/Castle OBJ.obj";
Model *castleModel;
string soldierFiringPath = "soldier_firing/soldier_firing.obj";
Model *soldierFiringModel;
string splatPath = "Splat/Splat_01.obj";
Model *splatModel;
vector<Model*> Models;

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
unsigned int castleNormalTexture;
unsigned int soldierNormalTexture;
unsigned int terrainNormalTexture;

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

mat4 calculate_model(vec3 trans, GLfloat rad, vec3 axis, vec3 scal) {
	mat4 model = mat4(1.0f);
	model = glm::translate(model, trans);
	model = glm::rotate(model, glm::radians(rad), axis);
	model = glm::scale(model, scal);
	return model;
}

void shadow(vector<Model*> Mod, vector<Shader*> Mod_shader, vector<mat4> model_matrix) {
	(*depthShader).use();
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glCullFace(GL_FRONT);

	// terrain render
	(*depthShader).setMat4("mvp", light_vp_matrix * terrain_model);
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(terrainVAO);
	glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);

	// other model render
	for (auto i = 0; i < Mod.size(); ++i) {
		(*depthShader).setMat4("mvp", light_vp_matrix * model_matrix[i]);
		(*Mod[i]).Draw((*Mod_shader[i]));
	}
	glCullFace(GL_BACK);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void cascade_shadow(vector<Model*> Mod, vector<Shader*> Mod_shader, vector<mat4> model_matrix) {
	(*depthShader).use();
	glViewport(0, 0, (float)SCR_WIDTH, (float)SCR_HEIGHT);

	for (int i = 0; i < NUM_CSM; ++i) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, depth_frames[i].fbo);
		glClear(GL_DEPTH_BUFFER_BIT);

		// terrain render
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(4.0f, 4.0f);
		glCullFace(GL_FRONT);
		(*depthShader).setMat4("mvp", light_matrices[i] * terrain_model);
		if (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(terrainVAO);
		glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);

		// other model render
		for (auto i = 0; i < Mod.size(); ++i) {
			(*depthShader).setMat4("mvp", light_matrices[i] * model_matrix[i]); // NOTICE uMVP
			(*Mod[i]).Draw((*Mod_shader[i]));
		}

		glCullFace(GL_BACK);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
}

void My_Init()
{
	projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera.NearPlane, camera.FarPlane);
	view = camera.GetViewMatrix();
	// CSM setting
	// -----------
	// CSM shadow
	depth_frames.resize(NUM_CSM);
	for (auto& frame : depth_frames) {
		create_frame(frame, (float)SCR_WIDTH, (float)SCR_HEIGHT);
	}
	// CSM
	light_matrices.resize(NUM_CSM);
	shadow_sbpv_matrices.resize(NUM_CSM);
	mat4 view_matrix_inv = inverse(view);
	mat4 view_matrix_light = lookAt(light_position, vec3(0.f), vec3(0.f, 1.f, 0.f));
	float aspect_ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
	float tan_half_hfov = tanf(radians(camera.Zoom * 0.5f));
	float tan_half_vfov = tanf(radians(camera.Zoom * aspect_ratio * 0.5f));
	printf("aspect_ratio: %f\ntan_half_hfov: %f\ntan_half_vfov: %f\n", aspect_ratio, tan_half_hfov, tan_half_vfov);
	// split frustum into NUM_CSM + 1 levels (flip-z) (in camera coordinate position is negative)
	csm_range[0] = -camera.NearPlane;
	csm_range[1] = -30.f;
	csm_range[2] = -90.f;
	csm_range[3] = -camera.FarPlane;
	// change to clip space (for projection matrix)
	for (int i = 0; i < NUM_CSM; ++i) {
		vec4 view_direction(0.f, 0.f, csm_range[i + 1], 1.f);
		vec4 clip = projection * view_direction;
		csm_range_C[i] = clip.z;
	}
	// calculate AABB bounding box
	for (int i = 0; i < NUM_CSM; ++i) {
		// xy position of camera view cone in world coordinate
		float xn = csm_range[i] * tan_half_hfov;
		float xf = csm_range[i + 1] * tan_half_hfov;
		float yn = csm_range[i] * tan_half_vfov;
		float yf = csm_range[i + 1] * tan_half_vfov;

		printf("\nCSM #%d\n", i);
		printf("\tnear_x: %f far_x: %f\n", xn, xf);
		printf("\tnear_y: %f far_y: %f\n", yn, yf);
		// camera view cone corners in view space in world coordinate
		vec4 frustum_corners[8] = {
			// near plane
			vec4(xn, yn, csm_range[i], 1.f),
			vec4(-xn, yn, csm_range[i], 1.f),
			vec4(xn, -yn, csm_range[i], 1.f),
			vec4(-xn, -yn, csm_range[i], 1.f),
			// far plane
			vec4(xf, yf, csm_range[i + 1], 1.f),
			vec4(-xf, yf, csm_range[i + 1], 1.f),
			vec4(xf, -yf, csm_range[i + 1], 1.f),
			vec4(-xf, -yf, csm_range[i + 1], 1.f),
		};

		// change to light space ( I have no idea how it works???)
		vec4 frustum_corners_L[8];

		float min_x = std::numeric_limits<float>::max();
		float max_x = std::numeric_limits<float>::min();
		float min_y = std::numeric_limits<float>::max();
		float max_y = std::numeric_limits<float>::min();
		float min_z = std::numeric_limits<float>::max();
		float max_z = std::numeric_limits<float>::min();

		// find local bounding box
		for (int j = 0; j < 8; ++j) {
			vec4 view = view_matrix_inv * frustum_corners[j];
			printf("\n\tViewspace: (%f, %f, %f, %f) =>\n", view.x, view.y, view.z, view.w);
			frustum_corners_L[j] = view_matrix_light * view;
			printf("\tLight space: (%f, %f, %f, %f)\n", frustum_corners_L[j].x, frustum_corners_L[j].y, frustum_corners_L[j].z, frustum_corners_L[j].w);

			min_x = fminf(min_x, frustum_corners_L[j].x);
			max_x = fmaxf(max_x, frustum_corners_L[j].x);
			min_y = fminf(min_y, frustum_corners_L[j].y);
			max_y = fmaxf(max_y, frustum_corners_L[j].y);
			min_z = fminf(min_z, frustum_corners_L[j].z);
			max_z = fmaxf(max_z, frustum_corners_L[j].z);
		}

		// set VP matrix for light (flip-z)
		light_matrices[i] = ortho(min_x, max_x, min_y, max_y, -max_z, -min_z) * view_matrix_light;
		shadow_sbpv_matrices[i] = scale_bias_matrix * light_matrices[i];
		printf("\n\tBounding Box: %f %f %f %f %f %f\n", min_x, max_x, min_y, max_y, -max_z, -min_z);
		// reference for how it work : https://blog.csdn.net/ZJU_fish1996/article/details/103689924
	}
	// -------------------------------------------------------------------------------------------

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
	(*rightScreenShader).setInt("ssaoTexture", 1);

	(*blurShader).use();
	(*blurShader).setInt("screenTexture", 0);

	// Load cubemap texture
	// -----------------------------------------
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
	unsigned char* data = stbi_load("terragen.jpg", &width, &height, &nrChannels, 0);
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
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load image
	//int width, height, nrChannels;
	data = stbi_load("noise.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		std::cout << "Failed to load texture" << std::endl;

	stbi_image_free(data);

	// calculate model matrix
	terrain_model = calculate_model(vec3(0.0f, -20.0f, 0.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(5.0f, 5.0f, 5.0f));
	model_castle = calculate_model(vec3(0.0f, -1.75f, 0.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(1.0f, 1.0f, 1.0f));
	model_splat = calculate_model(vec3(-13.0f, -1.5f, -5.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(0.3f, 0.1f, 0.3f));
	model_soldier = calculate_model(vec3(-23.0f, 6.75f, 0.0f), -90.0f, vec3(1.0, 0.0, 0.0), vec3(0.5f, 0.5f, 0.5f));
	// model_matrix vector
	//model_matrixs.push_back(terrain_model);
	model_matrixs.push_back(model_castle);
	model_matrixs.push_back(model_splat);
	model_matrixs.push_back(model_soldier);
	// model vector
	Models.push_back(castleModel);
	Models.push_back(splatModel);
	Models.push_back(soldierFiringModel);
	// shader vector
	Shaders.push_back(castleShader);
	Shaders.push_back(splatShader);
	Shaders.push_back(soldierShader);
	//Shaders.push_back(terrainShader);

	// Set up ssao
	// -----------------------------------------
	(*ssaoShader).use();
	(*ssaoShader).setBlock("Kernels");

	glGenVertexArrays(1, &ssao_vao);
	glBindVertexArray(ssao_vao);

	// Begin Initialize Kernal UBO
	glGenBuffers(1, &kernal_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, kernal_ubo);
	vec4 kernals[32];
	srand(time(NULL));
	for (int i = 0; i < 32; ++i)
	{
		float scale = i / 32.0;
		scale = 0.1f + 0.9f * scale * scale;
		kernals[i] = vec4(normalize(vec3(
			rand() / (float)RAND_MAX * 2.0f - 1.0f,
			rand() / (float)RAND_MAX * 2.0f - 1.0f,
			rand() / (float)RAND_MAX * 0.85f + 0.15f)) * scale,
			0.0f
		);
	}
	glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(vec4), &kernals[0][0], GL_STATIC_DRAW);

	// Begin Initialize Random Noise Map
	glGenTextures(1, &noise_map);
	glBindTexture(GL_TEXTURE_2D, noise_map);
	vec3 noiseData[16];
	for (int i = 0; i < 16; ++i)
	{
		noiseData[i] = normalize(vec3(
			rand() / (float)RAND_MAX, // 0.0 ~ 1.0
			rand() / (float)RAND_MAX, // 0.0 ~ 1.0
			0.0f
		));
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_FLOAT, &noiseData[0][0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Begin Initialize G Buffer
	glGenFramebuffers(1, &gbuffer.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);

	glGenTextures(2, &gbuffer.normal_map);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbuffer.normal_map, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbuffer.depth_map, 0);

	glGenFramebuffers(1, &ssao_fbo);
}

void Set_Cascade_Uniform(Shader *shader) {
	(*shader).use();
	// Set shadow texture index  NOTICE texture index
	for (int i = 0; i < NUM_CSM; ++i) {
		// depth texture
		ostringstream oss;
		oss << "uDepthTexture[" << i + 3 << "]";
		(*shader).setInt(oss.str(), i + 3);
		glActiveTexture(GL_TEXTURE0 + i + 3);
		glBindTexture(GL_TEXTURE_2D, depth_frames[i].depth_texture);
	}
	// light matrix
	(*shader).setMat4("csm_L[0]", light_matrices[0], NUM_CSM);

	for (int i = 0; i < NUM_CSM; ++i) {
		ostringstream oss;
		oss << "uCascadedRange_C[" << i << "]";
		(*shader).setFloat(oss.str(), csm_range_C[i]);
	}
}

void Render_Loaded_Model(mat4 projection, mat4 view)
{
	// render terrain
	// --------------
	mat4 shadow_matrix = shadow_sbpv_matrix * terrain_model;
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

	// shadow uniform
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	(*terrainShader).setInt("shadow_tex", 2);
	(*terrainShader).setMat4("shadow_matrix", shadow_matrix);

	glBindVertexArray(terrainVAO);

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
	// --------------------------------------
	shadow_matrix = shadow_sbpv_matrix * model_castle;

	(*castleShader).use();
	// use normal color
	if (using_normal_color)
		(*castleShader).setBool("using_normal_color", 1);
	else
		(*castleShader).setBool("using_normal_color", 0);
	// test normal mapping
	if (display_normal_mapping)
		(*castleShader).setBool("display_normal_mapping", 1);
	else
		(*castleShader).setBool("display_normal_mapping", 0);
	// cascade shadow mapping
	if (enable_cascade_shadow)
		(*castleShader).setBool("enable_cascade_shadow", 1);
	else
		(*castleShader).setBool("enable_cascade_shadow", 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	(*castleShader).setInt("shadow_tex", 2);
	(*castleShader).setMat4("shadow_matrix", shadow_matrix);

	(*castleShader).setMat4("projection", projection);
	(*castleShader).setMat4("view", view);
	(*castleShader).setVec3("light_pos", light_position);
	(*castleShader).setVec3("eye_pos", camera.Position);

	(*castleShader).setMat4("model", model_castle);
	//Set_Cascade_Uniform(castleShader);

	// Set shadow texture index  NOTICE texture index
	for (int i = 0; i < NUM_CSM; ++i) {
		// depth texture
		ostringstream oss;
		oss << "uDepthTexture[" << i << "]";
		(*castleShader).setInt(oss.str(), i + 3);
		glActiveTexture(GL_TEXTURE0 + i + 3);
		glBindTexture(GL_TEXTURE_2D, depth_frames[i].depth_texture);
	}
	// light matrix
	(*castleShader).setMat4("csm_L[0]", light_matrices[0], NUM_CSM);

	// shadow matrices
	for (int i = 0; i < NUM_CSM; ++i) {
		ostringstream oss;
		oss << "shadow_matrices[" << i << "]";
		(*castleShader).setMat4(oss.str(), shadow_sbpv_matrices[i] * model_castle);
	}

	// range
	for (int i = 0; i < NUM_CSM; ++i) {
		ostringstream oss;
		oss << "uCascadedRange_C[" << i << "]";
		(*castleShader).setFloat(oss.str(), csm_range_C[i]);
	}
	(*castleModel).Draw((*castleShader));

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

	(*splatShader).setMat4("model", model_splat);
	(*splatModel).Draw((*splatShader));

	// render the loaded soldier firing model
	// --------------------------------------
	shadow_matrix = shadow_sbpv_matrix * model_soldier;

	(*soldierShader).use();
	// use normal color
	if (using_normal_color)
		(*soldierShader).setBool("using_normal_color", 1);
	else
		(*soldierShader).setBool("using_normal_color", 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	(*soldierShader).setInt("shadow_tex", 2);
	(*soldierShader).setMat4("shadow_matrix", shadow_matrix);

	(*soldierShader).setMat4("projection", projection);
	(*soldierShader).setMat4("view", view);
	(*soldierShader).setVec3("light_pos", light_position);
	(*soldierShader).setVec3("eye_pos", camera.Position);

	(*soldierShader).setMat4("model", model_soldier);
	(*soldierFiringModel).Draw((*soldierShader));
}

void My_Display()
{
	// shadow test
	glEnable(GL_DEPTH_TEST);
	/*
	if (enable_cascade_shadow)
		cascade_shadow(Models, Shaders, model_matrixs);
	else
		shadow(Models, Shaders, model_matrixs);*/
	cascade_shadow(Models, Shaders, model_matrixs);
	shadow(Models, Shaders, model_matrixs);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// shadow test end

	// update screen split position
	glBindBuffer(GL_ARRAY_BUFFER, leftQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(leftQuadVertices), &leftQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(leftQuadVertices), &leftQuadVertices);
	glBindBuffer(GL_ARRAY_BUFFER, rightQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rightQuadVertices), &rightQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rightQuadVertices), &rightQuadVertices);

	projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera.NearPlane, camera.FarPlane);
	view = camera.GetViewMatrix();

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

	// render the loaded models
	// ------------------------------
	Render_Loaded_Model(projection, view);

	// draw ssao
	// --------------------------------
	// Begin Depth - Normal Pass
	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat ones[] = { 1.0f };

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, ones);

	// render the loaded models
	// ------------------------------
	using_normal_color = true;
	Render_Loaded_Model(projection, view);
	using_normal_color = false;

	// Begin SSAO Pass
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ssao_fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearBufferfv(GL_COLOR, 0, white);
	(*ssaoShader).use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	(*ssaoShader).setInt("normal_map", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	(*ssaoShader).setInt("depth_map", 1);
	(*ssaoShader).setMat4("proj", projection);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noise_map);
	(*ssaoShader).setInt("noise_map", 2);

	(*ssaoShader).set2Float("noise_scale", SCR_WIDTH / 4.0f, SCR_HEIGHT / 4.0f);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, kernal_ubo);

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(ssao_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	// bind back to default framebuffer
	// --------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	glBindVertexArray(rightQuadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ssao_tex);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	magnifyCenter_x = SCR_WIDTH / 2;
	magnifyCenter_y = SCR_HEIGHT / 2;
	prevX = SCR_WIDTH / 2.0f;
	lastX = SCR_WIDTH / 2.0f;
	lastY = SCR_HEIGHT / 2.0f;
	prevY = SCR_HEIGHT / 2.0f;
	projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, camera.NearPlane, camera.FarPlane);
	view = camera.GetViewMatrix();

	glGenFramebuffers(1, &shadowBuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	glGenTextures(1, &shadowBuffer.depthMap);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowBuffer.depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	// create rbos and textures for ssao here!!! 
	// ---------------------------
	glDeleteRenderbuffers(1, &ssao_rbo);
	glDeleteTextures(1, &ssao_tex);

	glGenRenderbuffers(1, &ssao_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, ssao_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

	// create ssao fbo texture
	glGenTextures(1, &ssao_tex);
	glBindTexture(GL_TEXTURE_2D, ssao_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// bind to ssao fbo
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ssao_fbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ssao_rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_tex, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
		case 'o':
			ssao = !ssao;
			(*rightScreenShader).setBool("ssao", ssao);
			break;
		case 'p':
			display_normal_mapping = !display_normal_mapping;
			break;
		case 'c':
			enable_cascade_shadow = !enable_cascade_shadow;
			break;
		default:
			cout << "Nothing" << endl;
			break;
	}
	glutPostRedisplay();
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
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
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
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

	//glutInitContextVersion(4, 2);
	//glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("Final_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();

	Shader s_castle("model_loading.vs.glsl", "model_loading.fs.glsl");
	castleShader = &s_castle;
	Shader s_solider("soldier_model.vs.glsl", "soldier_model.fs.glsl");
	soldierShader = &s_solider;
	Shader s_splat("splat_model.vs.glsl", "splat_model.fs.glsl");
	splatShader = &s_splat;
	Shader s_leftScreen("framebuffers_screen.vs.glsl", "post_processing.fs.glsl");
	leftScreenShader = &s_leftScreen;
	Shader s_rightScreen("framebuffers_screen.vs.glsl", "framebuffers_screen.fs.glsl");
	rightScreenShader = &s_rightScreen;
	Shader s_blur("framebuffers_screen.vs.glsl", "blur.fs.glsl");
	blurShader = &s_blur;
	Shader s_skybox("skybox.vs.glsl", "skybox.fs.glsl");
	skyboxShader = &s_skybox;
	Shader s_terrain("terrain.vs.glsl", "terrain.fs.glsl", "terrain.tcs.glsl", "terrain.tes.glsl");
	terrainShader = &s_terrain;
	Shader s_depth("depth.vs.glsl", "depth.fs.glsl");
	depthShader = &s_depth;
	Shader s_ssao("ssao.vs.glsl", "ssao.fs.glsl");
	ssaoShader = &s_ssao;

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
