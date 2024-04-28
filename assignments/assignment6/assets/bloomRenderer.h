#include "bloomBuffer.h"
#include <ew/shader.h>






class BloomRenderer
{
public:
    BloomRenderer();
    ~BloomRenderer();
    bool Init(unsigned int windowWidth, unsigned int windowHeight);
    void Destroy();
    void RenderBloomTexture(unsigned int srcTexture, float filterRadius);
    unsigned int BloomTexture();
    void configureQuad();

    

private:
    void RenderDownsamples(unsigned int srcTexture);
    void RenderUpsamples(float filterRadius);

    
    unsigned int quadVBO, quadVAO;

    bool mInit;
    bloomFBO mFBO;
    glm::ivec2 mSrcViewportSize;
    glm::vec2 mSrcViewportSizeFloat;
    ew::Shader* mDownsampleShader;
    ew::Shader* mUpsampleShader;
};