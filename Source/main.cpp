#include "../Externals/Include/Common.h"
#include "model.h"
#include "camera.h"
//#include "shader_m.h"
#include "mesh.h"
#include "time.h"
#include "water_helper.h"
#include "csm_helper.h"
#include <irrklang/irrKlang.h>

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

using namespace glm;
using namespace std;
using namespace irrklang;

ISoundEngine *SoundEngine = createIrrKlangDevice();

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
Camera camera(glm::vec3(-50.0f, 30.0f, 13.0f));
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
int effectID = 0;
bool using_normal_color = false;
bool display_normal_mapping = false;
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

// water ripple effect
GLuint program_drop;
GLuint program_water;
GLuint waterBufferIn;
GLuint waterBufferOut;
GLuint ripple_vao;
float timeElapsed = 0.0f;
bool rain = false;
struct WaterColumn
{
	float height;
	float flow;
};
// water reflection & refraction
reflection_frame_t w_reflect;
refraction_frame_t w_refract;

// model_matrix
mat4 model_castle;
mat4 model_splat;
vector<mat4> model_soldier;
mat4 model_smoke;
mat4 terrain_model;
mat4 model_water;
vector<mat4> model_matrixs;
mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 180.0f);

// light position
vec3 light_position = vec3(-50, 45, 76);// vec3(55, 51, 65);

//depth for shadow
mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f)) *scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
const float shadow_range = 75.0f;
mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 1000.0f);
mat4 light_view_matrix = lookAt(light_position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

// CSM
vector<mat4> light_vp_matrices;
vector<mat4> shadow_sbpv_matrices;
mat4 view_matrix_inv = inverse(camera.GetViewMatrix());

frame_t shadowBuffer; //shadow
vector<frame_t> shadowBuffers; // cascade shadow mapping

float csm_range[NUM_CSM + 1];
float csm_range_C[NUM_CSM];
bool use_cascade = false;

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
Shader *waterShader;
vector<Shader*> Shaders;

//particle system declare
#define MAX_PARTICLE_COUNT 1000

struct DrawArraysIndirectCommand
{
	uint count;
	uint primCount;
	uint first;
	uint baseInstance;
};
DrawArraysIndirectCommand defalutDrawArraysCommand = { 0, 1, 0, 0 };

struct Particle
{
	vec3 position;
	float _padding;
	vec3 velocity;
	float lifeTime;
};

struct ParticleBuffer
{
	GLuint shaderStorageBuffer;
	GLuint indirectBuffer;
};
ParticleBuffer particleIn;
ParticleBuffer particleOut;
GLuint particleTexture_smoke;
GLuint particleTexture_spark;
GLuint updateProgram;
GLuint addProgram;
GLuint renderProgram;
GLuint particle_vao;
//particle system declare end

// load models
// -----------
string castlePath = "Castle/Castle OBJ.obj";
Model *castleModel;
string soldierFiringPath = "soldier_firing/soldier_firing.obj";
Model *soldierFiringModel;
string splatPath = "Splat/Splat_01.obj";
Model *splatModel;
string waterQuadPath = "water/quad.obj";
Model *waterQuadModel;
vector<Model*> Models;

// skybox texture path
const char *skyboxTexPath[6] = { "..\\Assets\\cubemaps2\\posx.jpg",
								 "..\\Assets\\cubemaps2\\negx.jpg",
								 "..\\Assets\\cubemaps2\\posy.png",
								 "..\\Assets\\cubemaps2\\negy.png",
								 "..\\Assets\\cubemaps2\\posz.jpg",
								 "..\\Assets\\cubemaps2\\negz.jpg" };
const char *rain_skyboxTexPath[6] = { "..\\Assets\\cubemaps2\\rain\\posx.jpg",
									  "..\\Assets\\cubemaps2\\rain\\negx.jpg",
									  "..\\Assets\\cubemaps2\\rain\\posy.jpg",
									  "..\\Assets\\cubemaps2\\rain\\negy.jpg",
									  "..\\Assets\\cubemaps2\\rain\\posz.jpg",
									  "..\\Assets\\cubemaps2\\rain\\negz.jpg" };

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
unsigned int rain_skyboxTexture;
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
unsigned int waterReflectionTexture;
unsigned int waterRefractionTexture;

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

mat4 calculate_model_multi_angle(vec3 trans, int num_angle, vector<GLfloat> rad, vector<vec3> axis, vec3 scal) {
	mat4 model = mat4(1.0f);
	model = glm::translate(model, trans);
	for (int i = 0; i < num_angle; i++) {
		model = glm::rotate(model, glm::radians(rad[i]), axis[i]);
	}
	model = glm::scale(model, scal);
	return model;
}

void cascade_shadow(vector<Model*> Mod, vector<Shader*> Mod_shader, vector<mat4> model_matrix) {
	(*depthShader).use();
	glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glCullFace(GL_FRONT);

	for (int i = 0; i < NUM_CSM; ++i) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowBuffers[i].fbo);
		glClear(GL_DEPTH_BUFFER_BIT);

		// obj
		for (auto j = 0; j < Mod.size(); ++j) {
			(*depthShader).setMat4("mvp", light_vp_matrices[i] * model_matrix[j]);
			(*Mod[j]).Draw((*Mod_shader[j]));
		}
	}
	glCullFace(GL_BACK);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void CSM_uniform(Shader* shader, mat4 model) {
	// cascade shadow
	for (int i = 0; i < NUM_CSM; ++i) {

		// shadow_texes
		ostringstream oss;
		oss << "shadow_texes[" << i << "]";
		glActiveTexture(GL_TEXTURE3 + i);
		glBindTexture(GL_TEXTURE_2D, shadowBuffers[i].depthMap);
		(*shader).setInt(oss.str(), 3 + i);

		// shadow matrices
		ostringstream oss1;
		oss1 << "shadow_matrices[" << i << "]";
		(*shader).setMat4(oss1.str(), shadow_sbpv_matrices[i] * model);

		// range
		ostringstream oss2;
		oss2 << "uCascadedRange_C[" << i << "]";
		(*shader).setFloat(oss2.str(), csm_range_C[i]);
	}
}

void shadow(vector<Model*> Mod, vector<Shader*> Mod_shader, vector<mat4> model_matrix) {
	(*depthShader).use();
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glCullFace(GL_FRONT);

	(*depthShader).setMat4("mvp", light_vp_matrix * terrain_model);
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(terrainVAO);
	glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);

	for (auto i = 0; i < Mod.size(); ++i) {
		(*depthShader).setMat4("mvp", light_vp_matrix * model_matrix[i]);
		(*Mod[i]).Draw((*Mod_shader[i]));
	}
	glCullFace(GL_BACK);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void AddDrop()
{
	// Randomly add a "drop" of water into the grid system.
	glUseProgram(program_drop);
	glUniform2ui(0, rand() % 150+15, rand() % 150+15);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferIn);
	glDispatchCompute(10, 10, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	model_water *= translate(mat4(1.0f), vec3(0.0, -0.00075, 0.0));
}

// Add count particles to input buffers.
void AddParticle(uint count)
{
	glUseProgram(addProgram);
	glUniform1ui(0, count);
	glUniform2f(1, static_cast<float>(rand()), static_cast<float>(rand()));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleIn.shaderStorageBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, particleIn.indirectBuffer);
	glDispatchCompute(1, 1, 1);
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

	//***particle system init ***
	// Create shader program for adding particles.
	GLuint cs_add = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs_add, 1, loadShaderSource("add_cs.glsl"), NULL);
	glCompileShader(cs_add);
	addProgram = glCreateProgram();
	glAttachShader(addProgram, cs_add);
	glLinkProgram(addProgram);

	// Create shader program for updating particles. (from input to output)
	GLuint cs_update = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs_update, 1, loadShaderSource("update_cs.glsl"), NULL);
	glCompileShader(cs_update);
	updateProgram = glCreateProgram();
	glAttachShader(updateProgram, cs_update);
	glLinkProgram(updateProgram);

	// Create shader program for rendering particles.
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, loadShaderSource("render_vs.glsl"), NULL);
	glCompileShader(vs);
	GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gs, 1, loadShaderSource("render_gs.glsl"), NULL);
	glCompileShader(gs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, loadShaderSource("render_fs.glsl"), NULL);
	glCompileShader(fs);
	renderProgram = glCreateProgram();
	glAttachShader(renderProgram, vs);
	glAttachShader(renderProgram, gs);
	glAttachShader(renderProgram, fs);
	glLinkProgram(renderProgram);

	// Create shader storage buffers & indirect buffers
	glGenBuffers(1, &particleIn.shaderStorageBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleIn.shaderStorageBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * MAX_PARTICLE_COUNT, NULL, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &particleIn.indirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleIn.indirectBuffer);
	glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &particleOut.shaderStorageBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleOut.shaderStorageBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * MAX_PARTICLE_COUNT, NULL, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &particleOut.indirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleOut.indirectBuffer);
	glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand, GL_DYNAMIC_STORAGE_BIT);

	// Create particle textures
	texture_data textureData = loadImg("smoke.jpg");
	glGenTextures(1, &particleTexture_smoke);
	glBindTexture(GL_TEXTURE_2D, particleTexture_smoke);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, textureData.width, textureData.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureData.width, textureData.height, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] textureData.data;

	textureData = loadImg("spark.jpg");
	glGenTextures(1, &particleTexture_spark);
	glBindTexture(GL_TEXTURE_2D, particleTexture_spark);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, textureData.width, textureData.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureData.width, textureData.height, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] textureData.data;

	// Create VAO. We don't have any input attributes, but this is still required.
	glGenVertexArrays(1, &particle_vao);
	glBindVertexArray(particle_vao);
	//***particle system init end***

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

	glGenTextures(1, &rain_skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, rain_skyboxTexture);
	for (int i = 0; i < 6; ++i)
	{
		texture_data image = loadImg(rain_skyboxTexPath[i]);
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

	// CSM 
	shadow_sbpv_matrices.resize(NUM_CSM);
	light_vp_matrices.resize(NUM_CSM);
	//mat4 view_matrix_inv = inverse(camera.GetViewMatrix());
	float aspect_ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;//(float)SHADOW_MAP_WIDTH / (float)SHADOW_MAP_HEIGHT;
	float tan_half_hfov = tanf(radians(150.f * 0.5f));
	float tan_half_vfov = tanf(radians(150.f * aspect_ratio * 0.5f));

	// split frustum into NUM_CSM + 1 levels (flip-z)
	csm_range[0] = -0.1f; // near plane
	csm_range[1] = -35.f;
	csm_range[2] = -75.f;
	csm_range[3] = -200.0f; // far plane

	//mat4 proj_matrix = perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 180.f);
	// change to clip space
	for (int i = 0; i < NUM_CSM; ++i) {
		//vec4 view(0.f, 0.f, csm_range[i + 1], 1.f);
		vec4 view(0.f, 0.f, csm_range[i + 1], 1.f);
		vec4 clip = projection * view;
		csm_range_C[i] = clip.z;
	}

	for (int i = 0; i < NUM_CSM; ++i) {
		float xn = csm_range[i] * tan_half_hfov;
		float xf = csm_range[i + 1] * tan_half_hfov;
		float yn = csm_range[i] * tan_half_vfov;
		float yf = csm_range[i + 1] * tan_half_vfov;

		printf("\nCSM #%d\n", i);
		printf("\tnear_x: %f far_x: %f\n", xn, xf);
		printf("\tnear_y: %f far_y: %f\n", yn, yf);

		// corners in view space
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

		// change to light space
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
			//printf("\n\tViewspace: (%f, %f, %f, %f) =>\n", view.x, view.y, view.z, view.w);
			frustum_corners_L[j] = light_view_matrix * view;
			//printf("\tLight space: (%f, %f, %f, %f)\n", frustum_corners_L[j].x, frustum_corners_L[j].y, frustum_corners_L[j].z, frustum_corners_L[j].w);

			min_x = fminf(min_x, frustum_corners_L[j].x);
			max_x = fmaxf(max_x, frustum_corners_L[j].x);
			min_y = fminf(min_y, frustum_corners_L[j].y);
			max_y = fmaxf(max_y, frustum_corners_L[j].y);
			min_z = fminf(min_z, frustum_corners_L[j].z);
			max_z = fmaxf(max_z, frustum_corners_L[j].z);
		}

		// set VP matrix for light (flip-z)
		light_vp_matrices[i] = ortho(min_x, max_x, min_y, max_y, -max_z, -min_z) * light_view_matrix;
		printf("\n\tBounding Box: %f %f %f %f %f %f\n", min_x, max_x, min_y, max_y, -max_z, -min_z);
		shadow_sbpv_matrices[i] = scale_bias_matrix * light_vp_matrices[i];
	}

	// calculate model matrix
	terrain_model = calculate_model(vec3(0.0f, -50.0f, 0.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(15.0f, 15.0f, 15.0f));
	model_castle = calculate_model(vec3(-55.0f, 10.5f, 21.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(0.7f, 0.7f, 0.7f));
	model_splat = calculate_model(vec3(13.0f, -1.5f, -6.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(0.4f, 0.1f, 0.4f));
	model_water = calculate_model(vec3(-45.0f, -53.95f, -40.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(0.22f, 1.0f, 0.42f)); //-54
	model_water = calculate_model(vec3(-45.0f, 5.95f, -40.0f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(20.f, 1.0f, 30.f)); //-54
	//model_soldier = calculate_model(vec3(-65.0f, 10.75f, 21.5f), -90.0f, vec3(1.0, 0.0, 0.0), vec3(0.5f, 0.5f, 0.5f));

	vector<GLfloat> soldier_angle;
	vector<vec3> soldier_axis;
	// Pos
	vec3 pos_arr[] = { vec3(-65.0f, 10.75f, 21.5f),
					   vec3(-58.0f, 26.0f, 6.5f),
					   vec3(-54.0f, 26.0f, 6.5f),
	                   vec3(-41.0f, 17.6f, 35.5f) };
	// Angle
	vector<GLfloat> angle_arr[] = { vector<GLfloat>{-90.f},
								    vector<GLfloat>{-90.f, 90.f},
									vector<GLfloat>{-90.f, -90.f},
									vector<GLfloat>{-90.f, -45.f} };
	vector<vec3> axis_arr[] = { vector<vec3>{ vec3(1.0, 0.0, 0.0)},
								vector<vec3>{ vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0)},
								vector<vec3>{ vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0)},
								vector<vec3>{ vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0)} };
	for (int i = 0; i < (sizeof(pos_arr) / sizeof(*pos_arr)); i++) {
		for (int j = 0; j < angle_arr[i].size(); j++) {
			soldier_angle.push_back(angle_arr[i][j]);
			soldier_axis.push_back(axis_arr[i][j]);
		}
		model_soldier.push_back(calculate_model_multi_angle(pos_arr[i], soldier_angle.size(), soldier_angle, soldier_axis, vec3(0.5f, 0.5f, 0.5f)));
		soldier_angle.clear();
		soldier_axis.clear();
	}
	cout << model_soldier.size() << endl;
	// model_matrix vector
	//model_matrixs.push_back(terrain_model);
	model_matrixs.push_back(model_castle);
	model_matrixs.push_back(model_splat);
	for (int i = 0; i < model_soldier.size(); ++i) {
		model_matrixs.push_back(model_soldier[i]);
	}
	// model vector
	Models.push_back(castleModel);
	Models.push_back(splatModel);
	//Models.push_back(soldierFiringModel);
	for (int i = 0; i < model_soldier.size(); ++i) {
		Models.push_back(soldierFiringModel);
	}
	// shader vector
	Shaders.push_back(castleShader);
	Shaders.push_back(splatShader);
	Shaders.push_back(soldierShader);
	for (int i = 0; i < model_soldier.size(); ++i) {
		Shaders.push_back(soldierShader);
	}
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

	// Initialize water reflection & refraction
	create_reflection_frame(w_reflect, SCR_WIDTH, SCR_HEIGHT);
	create_refraction_frame(w_refract, SCR_WIDTH, SCR_HEIGHT);
	// Initialize random seed for water ripple effect
	// -----------------------------------------
	/*
	srand(time(NULL));
	{
		program_drop = glCreateProgram();
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		char** drop_cs = loadShaderSource("drop.cs.glsl");
		glShaderSource(cs, 1, drop_cs, NULL);
		glCompileShader(cs);
		glAttachShader(program_drop, cs);
		glLinkProgram(program_drop);
	}
	{
		program_water = glCreateProgram();
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		char** water_cs = loadShaderSource("water.cs.glsl");
		glShaderSource(cs, 1, water_cs, NULL);
		glCompileShader(cs);
		glAttachShader(program_water, cs);
		glLinkProgram(program_water);
	}

	// Create two water grid buffers of 180 * 180 water columns.
	glGenBuffers(1, &waterBufferIn);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, waterBufferIn);
	// Create initial data.
	WaterColumn *ripple_data = new WaterColumn[32400];
	for (int x = 0; x < 180; ++x)
	{
		for (int y = 0; y < 180; ++y)
		{
			int idx = y * 180 + x;
			ripple_data[idx].height = 60.0f;
			ripple_data[idx].flow = 0.0f;
		}
	}
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(WaterColumn) * 32400, ripple_data, GL_DYNAMIC_STORAGE_BIT);
	delete[] ripple_data;

	glGenBuffers(1, &waterBufferOut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, waterBufferOut);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(WaterColumn) * 32400, NULL, GL_DYNAMIC_STORAGE_BIT);

	glGenVertexArrays(1, &ripple_vao);
	glBindVertexArray(ripple_vao);

	// Create an index buffer of 2 triangles, 6 vertices.
	uint indices[] = { 0, 2, 3, 0, 3, 1 };
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * 6, indices, GL_DYNAMIC_STORAGE_BIT);
	
	AddDrop();
	*/
	// Add BGM
	SoundEngine->removeAllSoundSources();
	SoundEngine->play2D("audio/breakout.mp3", true);
}

void Render_Loaded_Model(mat4 projection, mat4 view, vec3 plane = vec3(0, -1, 0), float height = 100)
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
	//(*terrainShader).setVec3("plane", plane);
	//(*terrainShader).setFloat("plane_height", height);
	//(*castleShader).setMat4("model", terrain_model);

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
	// shadow
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	(*castleShader).setInt("shadow_tex", 2);
	(*castleShader).setMat4("shadow_matrix", shadow_matrix);

	// cascade shadow
	CSM_uniform(castleShader, model_castle);

	(*castleShader).setMat4("projection", projection);
	(*castleShader).setMat4("view", view);
	(*castleShader).setVec3("light_pos", light_position);
	(*castleShader).setVec3("eye_pos", camera.Position);
	//vec3 plane = vec3(0, -1, 0);
	(*castleShader).setVec3("plane", plane);
	(*castleShader).setFloat("plane_height", height);

	(*castleShader).setMat4("model", model_castle);
	(*castleShader).setBool("use_cascade", use_cascade);
	(*castleModel).Draw((*castleShader));

	// Update particles.
	glUseProgram(updateProgram);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleIn.shaderStorageBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, particleIn.indirectBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleOut.shaderStorageBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, particleOut.indirectBuffer);
	glUniform1f(0, 0.016f); // We use a fixed update step of 0.016 seconds.
	glNamedBufferSubData(particleOut.indirectBuffer, 0, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE); // Disable depth writing for additive blending. Remember to turn it on later...

	// Draw particles using updated buffers using additive blending.
	glBindVertexArray(particle_vao);
	glUseProgram(renderProgram);
	model_smoke = calculate_model(vec3(-20.95f, 9.5f, 0.08f), 0.0f, vec3(1.0, 0.0, 0.0), vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(0, 1, GL_FALSE, value_ptr(view * model_smoke));
	glUniformMatrix4fv(1, 1, GL_FALSE, value_ptr(projection));
	glActiveTexture(GL_TEXTURE0);
	if(rain)
		glBindTexture(GL_TEXTURE_2D, particleTexture_smoke);
	else
		glBindTexture(GL_TEXTURE_2D, particleTexture_spark);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleOut.shaderStorageBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleOut.indirectBuffer);
	glDrawArraysIndirect(GL_POINTS, 0);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	// Swap input and output buffer.
	std::swap(particleIn, particleOut);

	// render the loaded splat model and water ripple effect here ahhhhhhhhhhhhh
	// --------------------------------------
	// Update water grid.
	/*
	glUseProgram(program_water);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferIn);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, waterBufferOut);
	// Each group updates 18 * 18 of the grid. We need 10 * 10 groups in total.
	glDispatchCompute(10, 10, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	(*waterShader).use();
	glBindVertexArray(ripple_vao);
	(*waterShader).setMat4("projection", projection);
	(*waterShader).setMat4("view", view);
	(*waterShader).setMat4("model", model_water);
	(*waterShader).setVec3("light_pos", light_position);
	(*waterShader).setVec3("eye_pos", camera.Position);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferOut);
	if(rain)
		glBindTexture(GL_TEXTURE_CUBE_MAP, rain_skyboxTexture);
	else
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	(*waterShader).setInt("tex_cubemap", 0);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 179 * 179);

	std::swap(waterBufferIn, waterBufferOut);
	
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
	if (rain)
		glBindTexture(GL_TEXTURE_CUBE_MAP, rain_skyboxTexture);
	else
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	(*splatShader).setInt("tex_cubemap", 0);

	(*splatShader).setMat4("model", model_splat);
	(*splatModel).Draw((*splatShader));
	*/

	// render the loaded soldier firing model
	// --------------------------------------
	for (int i = 0; i < model_soldier.size(); ++i) {
		shadow_matrix = shadow_sbpv_matrix * model_soldier[i];

		(*soldierShader).use();
		// use normal color
		if (using_normal_color)
			(*soldierShader).setBool("using_normal_color", 1);
		else
			(*soldierShader).setBool("using_normal_color", 0);
		// shadow
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		(*soldierShader).setInt("shadow_tex", 2);
		(*soldierShader).setMat4("shadow_matrix", shadow_matrix);
		// cascade shadow
		CSM_uniform(soldierShader, model_soldier[i]);

		(*soldierShader).setMat4("projection", projection);
		(*soldierShader).setMat4("view", view);
		(*soldierShader).setVec3("light_pos", light_position);
		(*soldierShader).setVec3("eye_pos", camera.Position);

		(*soldierShader).setMat4("model", model_soldier[i]);
		(*soldierShader).setBool("use_cascade", use_cascade);
		//vec3 plane = vec3(0, -1, 0);
		(*soldierShader).setVec3("plane", plane);
		(*soldierShader).setFloat("plane_height", height);

		(*soldierShader).setMat4("model", model_soldier[i]);
		(*soldierFiringModel).Draw((*soldierShader));
	}
}

void My_Display()
{
	// shadow test
	glEnable(GL_DEPTH_TEST);
	shadow(Models, Shaders, model_matrixs);
	cascade_shadow(Models, Shaders, model_matrixs);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// shadow test end

	// update screen split position
	glBindBuffer(GL_ARRAY_BUFFER, leftQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(leftQuadVertices), &leftQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(leftQuadVertices), &leftQuadVertices);
	glBindBuffer(GL_ARRAY_BUFFER, rightQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rightQuadVertices), &rightQuadVertices, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rightQuadVertices), &rightQuadVertices);

	// view/projection transformations
	projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// render
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox
	glActiveTexture(GL_TEXTURE0);
	if (rain)
		glBindTexture(GL_TEXTURE_CUBE_MAP, rain_skyboxTexture);
	else
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
	glEnable(GL_CLIP_DISTANCE0);
	glBindFramebuffer(GL_FRAMEBUFFER, w_reflect.fbo);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float cam_distance = 2 * camera.Position.y - 6;
	camera.Position.y -= cam_distance;
	camera.Pitch = -camera.Pitch;
	view = camera.GetViewMatrix();
	// skybox
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	(*skyboxShader).use();
	glBindVertexArray(skyboxVAO);
	(*skyboxShader).setInt("tex_cubemap", 0);

	pv_matrix = projection * view;
	(*skyboxShader).setMat4("pv_matrix", pv_matrix);
	(*skyboxShader).setVec3("cam_pos", camera.Position);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);
	Render_Loaded_Model(projection, view, vec3(0, 1, 0), -6);
	camera.Position.y += cam_distance;
	camera.Pitch = -camera.Pitch;
	view = camera.GetViewMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER, w_refract.fbo);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Render_Loaded_Model(projection, view, vec3(0, -1, 0), 6);

	glDisable(GL_CLIP_DISTANCE0);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	Render_Loaded_Model(projection, view);
	mat4 shadow_matrix = shadow_sbpv_matrix * model_water;
	(*waterShader).use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, w_reflect.texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, w_refract.texture);
	(*waterShader).setMat4("projection", projection);
	(*waterShader).setMat4("view", view);
	(*waterShader).setVec3("light_pos", light_position);
	(*waterShader).setVec3("eye_pos", camera.Position);
	(*waterShader).setMat4("model", model_water);
	(*waterShader).setInt("reflectionTexture", 0);
	(*waterShader).setInt("refractionTexture", 1);
	(*waterQuadModel).Draw((*waterShader));

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

	// shadow
	create_frame(shadowBuffer, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	// cascade shadow
	shadowBuffers.resize(NUM_CSM);
	for (auto& frame : shadowBuffers) {
		create_frame(frame, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	}

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

	// Initialize water reflection & refraction
	create_reflection_frame(w_reflect, SCR_WIDTH, SCR_HEIGHT, true);
	create_refraction_frame(w_refract, SCR_WIDTH, SCR_HEIGHT, true);
}

void My_Timer(int val)
{
	timer_cnt++;
	timerCount++;
	timeElapsed += 0.8;
	if (timeElapsed > 1.0f)
	{
		if (rain)
			AddDrop();
		timeElapsed = 0;
	}
	if (timerCount > 100)
	{
		AddParticle(50);
		timerCount = 0;
	}
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
		case 'r':
			rain = !rain;
			if (rain)
			{
				SoundEngine->removeAllSoundSources();
				SoundEngine->play2D("audio/let it go-cut.mp3", true);
			}
			else
			{
				SoundEngine->removeAllSoundSources();
				SoundEngine->play2D("audio/breakout.mp3", true);
			}
			break;
		case 'c':
			use_cascade = !use_cascade;
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
	Shader s_water("water.vs.glsl", "water.fs.glsl");
	waterShader = &s_water;

	Model m_castle(castlePath);
	castleModel = &m_castle;
	Model m_soldierFiring(soldierFiringPath);
	soldierFiringModel = &m_soldierFiring;
	Model m_splat(splatPath);
	splatModel = &m_splat;
	Model m_waterQuda(waterQuadPath);
	waterQuadModel = &m_waterQuda;

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
