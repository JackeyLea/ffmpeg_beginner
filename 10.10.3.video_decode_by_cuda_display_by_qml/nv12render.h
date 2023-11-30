#ifndef NV12RENDER_H
#define NV12RENDER_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLWidget>

class NV12Render : public QOpenGLFunctions
{
public:
    ///
    /// \brief NV12Render 构造函数
    ///
    NV12Render();

    ///
    /// \brief render 渲染绘制纹理
    /// \param p 纹理数据指针
    /// \param width 纹理图片的宽度
    /// \param height 纹理图片的高度
    ///
    void render(uchar *p, int width, int height);

private:
    QOpenGLShaderProgram program;//着色程序对象
    GLuint idY,idUV;//纹理分量ID
    QOpenGLBuffer vbo;//纹理buffer
};

#endif // NV12RENDER_H
