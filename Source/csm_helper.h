#pragma once

#include "common.h"

#define NUM_CSM 3

struct camera_t
{
	float fov;
	float near_plane;
	float far_plane;
	int width;
	int height;
	glm::vec3 pos;
};

struct frame_t
{
	GLuint fbo;
	GLuint depthMap;
};

void check_frame()
{
	GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (ret != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FRAMEBUFFER ERROR\nstatus:" << ret << std::endl;
		exit(EXIT_FAILURE);
	}
}

void create_frame(frame_t& frame, int width, int height)
{
	glGenFramebuffers(1, &frame.fbo);

	glGenTextures(1, &frame.depthMap);
	glBindTexture(GL_TEXTURE_2D, frame.depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frame.depthMap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	check_frame();
}

