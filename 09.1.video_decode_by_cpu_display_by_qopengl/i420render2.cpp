#include "i420render2.h"

I420Render2::I420Render2(QWidget *parent)
    :QOpenGLWidget(parent)
{
    decoder = new FFmpegDecoder;
    connect(decoder,&FFmpegDecoder::sigFirst,[=](uchar* p,int w,int h){
        ptr = p;
        width = w;
        height = h;
    });
    connect(decoder,&FFmpegDecoder::newFrame,[=](){
        update();
    });
}

I420Render2::~I420Render2()
{

}

void I420Render2::setUrl(QString url)
{
    decoder->setUrl(url);
}

void I420Render2::startVideo()
{
    decoder->start();
}

void I420Render2::initializeGL()
{
    initializeOpenGLFunctions();
    const char *vsrc =
            "attribute vec4 vertexIn; \
             attribute vec4 textureIn; \
             varying vec4 textureOut;  \
             void main(void)           \
             {                         \
                 gl_Position = vertexIn; \
                 textureOut = textureIn; \
             }";

    const char *fsrc =
            "varying vec4 textureOut;\n"
            "uniform sampler2D textureY;\n"
            "uniform sampler2D textureU;\n"
            "uniform sampler2D textureV;\n"
            "void main(void)\n"
            "{\n"
            "vec3 yuv; \n"
            "vec3 rgb; \n"
            "yuv.x = texture2D(textureY, textureOut.st).r; \n"
            "yuv.y = texture2D(textureU, textureOut.st).r - 0.5; \n"
            "yuv.z = texture2D(textureV, textureOut.st).r - 0.5; \n"
            "rgb = mat3( 1,       1,         1, \n"
                        "0,       -0.39465,  2.03211, \n"
                        "1.13983, -0.58060,  0) * yuv; \n"
            "gl_FragColor = vec4(rgb, 1); \n"
            "}\n";

    m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,vsrc);
    m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,fsrc);
    m_program.link();

    GLfloat points[]{
        -1.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f,

        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
        0.0f,1.0f
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(points,sizeof(points));

    GLuint ids[3];
    glGenTextures(3,ids);
    idY = ids[0];
    idU = ids[1];
    idV = ids[2];
}

void I420Render2::resizeGL(int w, int h)
{
    if(h<=0) h=1;

    glViewport(0,0,w,h);
}

void I420Render2::paintGL()
{
    if(!ptr) return;

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_program.bind();
    vbo.bind();
    m_program.enableAttributeArray("vertexIn");
    m_program.enableAttributeArray("textureIn");
    m_program.setAttributeBuffer("vertexIn",GL_FLOAT, 0, 2, 2*sizeof(GLfloat));
    m_program.setAttributeBuffer("textureIn",GL_FLOAT,2 * 4 * sizeof(GLfloat),2,2*sizeof(GLfloat));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,idY);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width,height,0,GL_RED,GL_UNSIGNED_BYTE,ptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,idU);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width >> 1,height >> 1,0,GL_RED,GL_UNSIGNED_BYTE,ptr + width*height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,idV);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width >> 1,height >> 1,0,GL_RED,GL_UNSIGNED_BYTE,ptr + width*height*5/4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_program.setUniformValue("textureY",0);
    m_program.setUniformValue("textureU",1);
    m_program.setUniformValue("textureV",2);
    glDrawArrays(GL_QUADS,0,4);
    m_program.disableAttributeArray("vertexIn");
    m_program.disableAttributeArray("textureIn");
    m_program.release();
}
