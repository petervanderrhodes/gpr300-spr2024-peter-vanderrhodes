#include "framebuffer.h"

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
	glGenTextures(1, &framebuffer.colorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, framebuffer.width, framebuffer.height);
	//Attach color buffer to framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer.colorBuffer[0], 0);

	//Create depth buffer
	glGenTextures(1, &framebuffer.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, framebuffer.width, framebuffer.height);
	//Attach depth buffer to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer.depthBuffer, 0);
	

	return framebuffer;
}
