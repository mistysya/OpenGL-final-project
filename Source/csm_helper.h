#pragma once

//#include "common.h"
#include "../Externals/Include/Common.h"

#define NUM_CSM 3

struct frame_t
{
	GLuint fbo;
	GLuint depth_texture;
};

void check_frame()
{
	GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (ret != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FRAMEBUFFER ERROR\nstatus:" << ret << std::endl;
		exit(EXIT_FAILURE);
	}
}

void create_frame(frame_t& frame, int width, int height, bool is_reshape = false)
{
	if (!is_reshape) {
		glGenFramebuffers(1, &frame.fbo);
	}
	else {
		glDeleteTextures(1, &frame.depth_texture);
	}

	glGenTextures(1, &frame.depth_texture);
	glBindTexture(GL_TEXTURE_2D, frame.depth_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frame.depth_texture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	check_frame();
}

