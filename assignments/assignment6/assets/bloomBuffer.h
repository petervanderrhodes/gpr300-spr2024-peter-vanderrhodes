#include <glm/glm.hpp>
#include <ew/external/glad.h>
#include <vector>
#include <iostream>

struct bloomMip
{
    glm::vec2 size;
    glm::ivec2 intSize;
    unsigned int texture;
};

class bloomFBO
{
public:
    bloomFBO();
    ~bloomFBO();
    bool Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength);
    void Destroy();
    void BindForWriting();
    const std::vector<bloomMip>& MipChain() const;

private:
    bool mInit;
    unsigned int mFBO;
    std::vector<bloomMip> mMipChain;
};