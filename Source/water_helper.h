#pragma once

#include "../Externals/Include/Common.h"

struct reflection_frame_t
{
	GLuint fbo;
	GLuint texture;
	GLuint depthBuffer;
};

struct refraction_frame_t
{
	GLuint fbo;
	GLuint texture;
	GLuint depthTexture;
};

void create_reflection_frame(reflection_frame_t& frame, int width, int height, bool is_reshape = false)
{
	if (!is_reshape) {
		glGenFramebuffers(1, &frame.fbo);
	}
	else {
		glDeleteTextures(1, &frame.texture);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
	// Color Texture
	glGenTextures(1, &frame.texture);
	glBindTexture(GL_TEXTURE_2D, frame.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame.texture, 0);
	// Depth Buffer
	glGenRenderbuffers(1, &frame.depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, frame.depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frame.depthBuffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void create_refraction_frame(refraction_frame_t& frame, int width, int height, bool is_reshape = false)
{
	if (!is_reshape) {
		glGenFramebuffers(1, &frame.fbo);
	}
	else {
		glDeleteTextures(1, &frame.depthTexture);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
	// Color Texture
	glGenTextures(1, &frame.texture);
	glBindTexture(GL_TEXTURE_2D, frame.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame.texture, 0);
	// Depth Texture
	glGenTextures(1, &frame.depthTexture);
	glBindTexture(GL_TEXTURE_2D, frame.depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 1280, 720, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frame.depthTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
