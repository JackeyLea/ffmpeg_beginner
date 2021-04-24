#include "i420render.h"

I420Render::I420Render()
{
    mTexY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    mTexY->setFormat(QOpenGLTexture::LuminanceFormat);
    mTexY->setMinificationFilter(QOpenGLTexture::Nearest);
    mTexY->setMagnificationFilter(QOpenGLTexture::Nearest);
    mTexY->setWrapMode(QOpenGLTexture::ClampToEdge);

    mTexU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    mTexU->setFormat(QOpenGLTexture::LuminanceFormat);
    mTexU->setMinificationFilter(QOpenGLTexture::Nearest);
    mTexU->setMagnificationFilter(QOpenGLTexture::Nearest);
    mTexU->setWrapMode(QOpenGLTexture::ClampToEdge);

    mTexV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    mTexV->setFormat(QOpenGLTexture::LuminanceFormat);
    mTexV->setMinificationFilter(QOpenGLTexture::Nearest);
    mTexV->setMagnificationFilter(QOpenGLTexture::Nearest);
    mTexV->setWrapMode(QOpenGLTexture::ClampToEdge);
}

I420Render::~I420Render()
{}

void I420Render::init()
{
    initializeOpenGLFunctions();
    const char *vsrc =
            "attribute vec4 vertexIn; \
             attribute vec2 textureIn; \
             varying vec2 textureOut;  \
             void main(void)           \
             {                         \
                 gl_Position = vertexIn; \
                 textureOut = textureIn; \
             }";

    const char *fsrc =
            "varying mediump vec2 textureOut;\n"
            "uniform sampler2D textureY;\n"
            "uniform sampler2D textureU;\n"
            "uniform sampler2D textureV;\n"
            "void main(void)\n"
            "{\n"
            "vec3 yuv; \n"
            "vec3 rgb; \n"
            "yuv.x = texture2D(textureY, textureOut).r; \n"
            "yuv.y = texture2D(textureU, textureOut).r - 0.5; \n"
            "yuv.z = texture2D(textureV, textureOut).r - 0.5; \n"
            "rgb = mat3( 1,       1,         1, \n"
                        "0,       -0.3455,  1.779, \n"
                        "1.4075, -0.7169,  0) * yuv; \n"
            "gl_FragColor = vec4(rgb, 1); \n"
            "}\n";

    m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,vsrc);
    m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,fsrc);
    m_program.bindAttributeLocation("vertexIn",0);
    m_program.bindAttributeLocation("textureIn",1);
    m_program.link();
    m_program.bind();

    vertices << QVector2D(-1.0f,1.0f)
             << QVector2D(1.0f,1.0f)
             << QVector2D(1.0f,-1.0f)
             << QVector2D(-1.0f,-1.0f);

    textures << QVector2D(0.0f,1.f)
             << QVector2D(1.0f,1.0f)
             << QVector2D(1.0f,0.0f)
             << QVector2D(0.0f,0.0f);
}

void I420Render::updateTextureInfo(int w, int h)
{
    mTexY->setSize(w,h);
    mTexY->allocateStorage(QOpenGLTexture::Red,QOpenGLTexture::UInt8);

    mTexU->setSize(w/2,h/2);
    mTexU->allocateStorage(QOpenGLTexture::Red,QOpenGLTexture::UInt8);

    mTexV->setSize(w/2,h/2);
    mTexV->allocateStorage(QOpenGLTexture::Red,QOpenGLTexture::UInt8);

    mTextureAlloced=true;
}

void I420Render::updateTextureData(const YUVData &data)
{
    if(data.Y.size()<=0 || data.U.size()<=0 || data.V.size()<=0) return;

    QOpenGLPixelTransferOptions options;
    options.setImageHeight(data.height);

    options.setRowLength(data.yLineSize);
    mTexY->setData(QOpenGLTexture::Luminance,QOpenGLTexture::UInt8,data.Y.data(),&options);

    options.setRowLength(data.uLineSize);
    mTexU->setData(QOpenGLTexture::Luminance,QOpenGLTexture::UInt8,data.U.data(),&options);

    options.setRowLength(data.vLineSize);
    mTexV->setData(QOpenGLTexture::Luminance,QOpenGLTexture::UInt8,data.V.data(),&options);
}

void I420Render::paint()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    if(!mTextureAlloced) return;

    m_program.bind();
    m_program.enableAttributeArray("vertexIn");
    m_program.setAttributeArray("vertexIn",vertices.constData());
    m_program.enableAttributeArray("textureIn");
    m_program.setAttributeArray("textureIn",textures.constData());

    glActiveTexture(GL_TEXTURE0);
    mTexY->bind();

    glActiveTexture(GL_TEXTURE1);
    mTexU->bind();

    glActiveTexture(GL_TEXTURE2);
    mTexV->bind();

    m_program.setUniformValue("textureY",0);
    m_program.setUniformValue("textureU",1);
    m_program.setUniformValue("textureV",2);
    glDrawArrays(GL_QUADS,0,4);
    m_program.disableAttributeArray("vertexIn");
    m_program.disableAttributeArray("textureIn");
    m_program.release();
}

void I420Render::resize(int w,int h)
{
    glViewport(0,0,w,h);
}
