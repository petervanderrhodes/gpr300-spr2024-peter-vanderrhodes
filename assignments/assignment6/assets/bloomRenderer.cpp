#include "bloomRenderer.h"

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
    mDownsampleShader = new ew::Shader("bloomSampling.vert", "downsample.frag");
    mUpsampleShader = new ew::Shader("bloomSampling.vert", "upsample.frag");

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

void BloomRenderer::RenderBloomTexture(unsigned int srcTexture, float filterRadius)
{
    mFBO.BindForWriting();

    this->RenderDownsamples(srcTexture);
    this->RenderUpsamples(filterRadius);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Restore viewport
    glViewport(0, 0, mSrcViewportSize.x, mSrcViewportSize.y);
}

GLuint BloomRenderer::BloomTexture()
{
    return mFBO.MipChain()[0].texture;
}