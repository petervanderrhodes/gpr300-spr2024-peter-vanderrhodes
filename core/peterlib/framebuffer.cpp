#include "framebuffer.h"
#include <stdio.h>

peter::Framebuffer peter::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	//Create framebuffer object
	Framebuffer framebuffer;
	glGenFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
	//Setting framebuffer variables
	framebuffer.width = width;
	framebuffer.height = height;

	//Create 8 bit RGBA color buffer
	glGenTextures(1, &framebuffer.colorBuffers[0]);
	glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffers[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, framebuffer.width, framebuffer.height);
	//Attach color buffer to framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer.colorBuffers[0], 0);

	glGenTextures(1, &framebuffer.colorBuffers[1]);
	glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffers[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, framebuffer.width, framebuffer.height);
	//Attach color buffer to framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, framebuffer.colorBuffers[1], 0);

	const GLenum drawBuffers[2] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
	};
	glDrawBuffers(2, drawBuffers);

	//Create depth buffer
	glGenTextures(1, &framebuffer.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, framebuffer.width, framebuffer.height);
	//Attach depth buffer to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer.depthBuffer, 0);
	
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	return framebuffer;
}

peter::Framebuffer peter::createGBuffer(unsigned int width, unsigned height)
{
	Framebuffer framebuffer;
	framebuffer.width = width;
	framebuffer.height = height;

	glCreateFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	int formats[3] = {
		GL_RGB32F, //0 = World Position 
		GL_RGB16F, //1 = World Normal
		GL_RGB16F  //2 = Albedo
	};
	//Create 3 color textures
	for (size_t i = 0; i < 3; i++)
	{
		glGenTextures(1, &framebuffer.colorBuffers[i]);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffers[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);
		//Clamp to border so we don't wrap when sampling for post processing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Attach each texture to a different slot.
	//GL_COLOR_ATTACHMENT0 + 1 = GL_COLOR_ATTACHMENT1, etc
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, framebuffer.colorBuffers[i], 0);
	}

	//Explicitly tell OpenGL which color attachments we will draw to
	const GLenum drawBuffers[3] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, drawBuffers);
	//TODO: Add texture2D depth buffer
	//Create depth buffer
	glGenTextures(1, &framebuffer.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, framebuffer.width, framebuffer.height);
	//Attach depth buffer to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer.depthBuffer, 0);
	//TODO: Check for completeness

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}


	//Clean up global state
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return framebuffer;

}
