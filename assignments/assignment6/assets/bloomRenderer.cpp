#include "bloomRenderer.h"

float quadVertices[] = {
    // upper-left triangle
    -1.0f, -1.0f, 0.0f, 0.0f, // position, texcoord
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
     // lower-right triangle
     -1.0f, -1.0f, 0.0f, 0.0f,
      1.0f,  1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, 1.0f, 0.0f
};

bool BloomRenderer::Init(unsigned int windowWidth, unsigned int windowHeight)
{
    if (mInit) return true;
    mSrcViewportSize = glm::ivec2(windowWidth, windowHeight);
    mSrcViewportSizeFloat = glm::vec2((float)windowWidth, (float)windowHeight);

    // Framebuffer
    const unsigned int num_bloom_mips = 5; // Experiment with this value
    bool status = mFBO.Init(windowWidth, windowHeight, num_bloom_mips);
    if (!status) {
        std::cerr << "Failed to initialize bloom FBO - cannot create bloom renderer!\n";
        return false;
    }

    // Shaders
    mDownsampleShader = new ew::Shader("assets/bloomSampling.vert", "assets/downsample.frag");
    mUpsampleShader = new ew::Shader("assets/bloomSampling.vert", "assets/upsample.frag");

    // Downsample
    mDownsampleShader->use();
    mDownsampleShader->setInt("srcTexture", 0);
    //mDownsampleShader->Deactivate();

    // Upsample
    mUpsampleShader->use();
    mUpsampleShader->setInt("srcTexture", 0);
    //mUpsampleShader->Deactivate();

    mInit = true;
    return true;
}

BloomRenderer::BloomRenderer() : mInit(false) {}
BloomRenderer::~BloomRenderer() {}

void BloomRenderer::Destroy()
{
    mFBO.Destroy();
    delete mDownsampleShader;
    delete mUpsampleShader;
    mInit = false;
}

void BloomRenderer::RenderBloomTexture(unsigned int srcTexture, float filterRadius, unsigned int dummyVAO)
{
    mFBO.BindForWriting();

    this->RenderDownsamples(srcTexture, dummyVAO);
    this->RenderUpsamples(filterRadius, dummyVAO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Restore viewport
    glViewport(0, 0, mSrcViewportSize.x, mSrcViewportSize.y);
}

GLuint BloomRenderer::BloomTexture()
{
    return mFBO.MipChain()[0].texture;
}

void BloomRenderer::RenderDownsamples(unsigned int srcTexture, unsigned int dummyVAO)
{
    const std::vector<bloomMip>& mipChain = mFBO.MipChain();

    mDownsampleShader->use();
    mDownsampleShader->setVec2("srcResolution", mSrcViewportSizeFloat);

    // Bind srcTexture (HDR color buffer) as initial texture input
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);

    // Progressively downsample through the mip chain
    for (int i = 0; i < mipChain.size(); i++)
    {
        const bloomMip& mip = mipChain[i];
        glViewport(0, 0, mip.size.x, mip.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, mip.texture, 0);

        // Render screen-filled quad of resolution of current mip
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        // Set current mip resolution as srcResolution for next iteration
        mDownsampleShader->setVec2("srcResolution", mip.size);
        // Set current mip as texture input for next iteration
        glBindTexture(GL_TEXTURE_2D, mip.texture);
    }

    //mDownsampleShader->Deactivate();
}

void BloomRenderer::RenderUpsamples(float filterRadius, unsigned int dummyVAO)
{
    const std::vector<bloomMip>& mipChain = mFBO.MipChain();

    mUpsampleShader->use();
    mUpsampleShader->setFloat("filterRadius", filterRadius);

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = mipChain.size() - 1; i > 0; i--)
    {
        const bloomMip& mip = mipChain[i];
        const bloomMip& nextMip = mipChain[i - 1];

        // Bind viewport and texture from where to read
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mip.texture);

        // Set framebuffer render target (we write to this texture)
        glViewport(0, 0, nextMip.size.x, nextMip.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, nextMip.texture, 0);

        // Render screen-filled quad of resolution of current mip
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    // Disable additive blending
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
    glDisable(GL_BLEND);

    //mUpsampleShader->Deactivate();
}

/*
void BloomRenderer::configureQuad() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coordinate
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
*/