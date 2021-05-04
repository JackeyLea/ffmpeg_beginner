#include "nv12render.h"

NV12Render::NV12Render()
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
            "varying mediump vec4 textureOut;\n"
            "uniform sampler2D textureY;\n"
            "uniform sampler2D textureUV;\n"
            "void main(void)\n"
            "{\n"
            "highp vec3 yuv; \n"
            "highp vec3 rgb; \n"
            "yuv.x = texture2D(textureY, textureOut.st).r - 0.0625; \n"
            "yuv.y = texture2D(textureUV, textureOut.st).r - 0.5; \n"
            "yuv.z = texture2D(textureUV, textureOut.st).g - 0.5; \n"
            "rgb = mat3( 1,       1,         1, \n"
                        "0,       -0.39465,  2.03211, \n"
                        "1.13983, -0.58060,  0) * yuv; \n"
            "gl_FragColor = vec4(rgb, 1); \n"
            "}\n";

    program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,vsrc);
    program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,fsrc);
    program.link();

    GLfloat points[]{
        -1.0f,1.0f,
        -1.0f,-1.0f,
        1.0f,1.0f,
        1.0f,-1.0f,

        0.0f,1.0f,
        0.0f,0.0f,
        1.0f,1.0f,
        1.0f,0.0f
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(points,sizeof(points));

    GLuint ids[2];
    glGenTextures(2,ids);
    idY = ids[0];
    idUV = ids[1];
}

void NV12Render::render(uchar *p, int width, int height)
{
    if(p==nullptr) {
        //qDebug()<<"No data in buffer.";
        return;
    }

    //qDebug()<<"opengl render: "<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.bind();
    vbo.bind();
    program.enableAttributeArray("vertexIn");
    program.enableAttributeArray("textureIn");
    program.setAttributeBuffer("vertexIn",GL_FLOAT, 0, 2, 2*sizeof(GLfloat));
    program.setAttributeBuffer("textureIn",GL_FLOAT,2 * 4 * sizeof(GLfloat),2,2*sizeof(GLfloat));

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D,idY);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width,height,0,GL_RED,GL_UNSIGNED_BYTE,p);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D,idUV);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RG,width >> 1,height >> 1,0,GL_RG,GL_UNSIGNED_BYTE,p + width*height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    program.setUniformValue("textureUV",0);
    program.setUniformValue("textureY",1);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    program.disableAttributeArray("vertexIn");
    program.disableAttributeArray("textureIn");
    program.release();

    //qDebug()<<"opengl render finished: "<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}
