#ifndef I420RENDER_H
#define I420RENDER_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLWidget>

#include "ffmpegdecoder.h"

class I420Render : public QOpenGLWidget,public QOpenGLFunctions
{
    Q_OBJECT
public:
    I420Render(QWidget *parent =nullptr);
    ~I420Render();

    void setUrl(QString url);

    void startVideo();

    void initializeGL();
    void resizeGL(int w,int h);
    void paintGL();

private:
    //shader程序
    QOpenGLShaderProgram m_program;
    //shader中yuv变量地址
    GLuint m_textureUniformY, m_textureUniformU , m_textureUniformV;
    //创建纹理
    GLuint m_idy , m_idu , m_idv;

    int width,height;

    FFmpegDecoder *decoder;

    uchar* ptr;
};

#endif // I420RENDER_H
