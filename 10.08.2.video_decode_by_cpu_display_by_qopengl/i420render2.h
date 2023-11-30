#ifndef I420RENDER2_H
#define I420RENDER2_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLWidget>

#include "ffmpegdecoder.h"

class I420Render2 : public QOpenGLWidget,public QOpenGLFunctions
{
    Q_OBJECT
public:
    I420Render2(QWidget *parent =nullptr);
    ~I420Render2();

    void setUrl(QString url);

    void startVideo();

    void initializeGL();
    void resizeGL(int w,int h);
    void paintGL();

private:
    //shader程序
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer vbo;

    int idY,idU,idV;

    int width,height;

    FFmpegDecoder *decoder;

    uchar* ptr;
};

#endif // I420RENDER2_H
