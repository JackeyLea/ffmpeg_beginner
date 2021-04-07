#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLShader>

QT_FORWARD_DECLARE_CLASS(VideoData)

#include "ffmpegvideo.h"

class Widget : public QOpenGLWidget,protected QOpenGLFunctions
{
    Q_OBJECT
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
 
public slots:
    void play(QString);
    void stop();
 
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w,int h);
 
private:
    uchar *m_ptr;
    int m_width,m_height;
    //VideoData *m_videoData;
    FFmpegVideo *m_videoData;

    QOpenGLShaderProgram program;
    GLuint idY,idUV;
    QOpenGLBuffer vbo;
};
 
#endif // WIDGET_H
