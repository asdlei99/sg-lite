#include "sgdynimagematerial.h"
#include <algorithm>


class SGDynImageMaterialShader : public SGMaterialShader
{
public:
    SGDynImageMaterialShader() { build(); }
    ~SGDynImageMaterialShader() {}

    virtual void updateState(const float *mat4, const SGNodeState *stat) override
    {
        m_program.setMat4(m_matrixUniform, mat4);

        m_program.setInt(m_texUniform, 0);
        m_program.setFloat(m_opacityUniform, stat->opacity);
    }

    virtual const char* vertexShader() const override
    {
        return  "attribute vec4 posAttr;\n"
            "attribute vec4 texAttr;\n"
            "varying vec4 tex;\n"
            "uniform mat4 matrix;\n"
            "void main() {\n"
            "   tex = texAttr;\n"
            "   gl_Position = matrix * posAttr;\n"
            "}\n";
    }

    virtual const char* fragmentShader() const override
    {
        return  "varying vec4 tex;\n"
            "uniform sampler2D uSampler;\n"
            "uniform float opacity;\n"
            "void main() {\n"
            "   vec4 color = texture2D(uSampler, vec2(tex.x, tex.y)); \n"
            "   gl_FragColor = vec4(color.rgb, color.a*opacity); \n"
            "}\n";
    }

    virtual char const* const* attributeNames() const override
    {
        static const char* const attr[] = { "posAttr", "texAttr", 0 };
        return attr;
    }

    bool build()
    {
        compile();
        m_matrixUniform = m_program.uniformLocation("matrix");
        m_texUniform = m_program.uniformLocation("uSampler");
        m_opacityUniform = m_program.uniformLocation("opacity");

        return true;
    }
private:
    unsigned m_matrixUniform;
    unsigned m_texUniform;
    unsigned m_opacityUniform;
};

SGDynImageMaterial::SGDynImageMaterial()
{
    setSize(1920, 1080);
}

E_MaterialType SGDynImageMaterial::type() const
{
    return E_MaterialType::DynImageMaterialType;
}

SGMaterialShader * SGDynImageMaterial::createShader() const
{
    return new SGDynImageMaterialShader;
}

void SGDynImageMaterial::setSize(int wid, int hei)
{
    std::lock_guard<std::mutex> guard(m_yuvLock);

    m_width = wid;
    m_height = hei;
    int oldsize = m_yuvBufSize;
    m_yuvBufSize = m_width*m_height * 3;

    //realloc
    if (oldsize < m_yuvBufSize)
    {
        delete[]m_yuvBuf;
        m_yuvBuf = new unsigned char[m_yuvBufSize];
    }
}

void SGDynImageMaterial::setRgb(const unsigned char * src)
{
    std::lock_guard<std::mutex> guard(m_yuvLock);

    memcpy(m_yuvBuf, src, m_yuvBufSize);
    m_yuvChanged = 1;
}

SGTexture * SGDynImageMaterial::texture() const
{
    return &m_tex;
}

void SGDynImageMaterial::syncState()
{
    m_tex.bind();
    if (!m_yuvChanged) return;

    std::lock_guard<std::mutex> guard(m_yuvLock);
    m_yuvChanged = 0;

    m_tex.update(0, Image(m_width, m_height, m_yuvBuf, Image::Format_RGB));
}
