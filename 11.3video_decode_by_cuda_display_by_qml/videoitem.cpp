#include "videoitem.h"
#include <QOpenGLFramebufferObject>
#include <QDebug>

#include "nv12render.h"

class VideoFboItem
        :public QQuickFramebufferObject::Renderer
{
public:
    ///
    /// \brief VideoFboItem 构造函数，调用成员变量的构造函数
    ///
    VideoFboItem()
    {
        //qDebug()<<"VideoFboItem";
    }

    ///
    /// \brief ~VideoFboItem 析构函数，释放资源
    ///
    ~VideoFboItem() override{
    }

    ///
    /// \brief synchronize 数据同步函数，获取界面解码后的帧数据和宽度高度
    /// \param item 父界面的对象指针
    ///
    void synchronize(QQuickFramebufferObject *item) override{
        VideoItem* pItem= dynamic_cast<VideoItem*>(item);
        if(pItem){
            //qDebug()<<"synchroinze: "<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            pItem->getFrame(&ptr,&videoW,&videoH);
        }
    }

    ///
    /// \brief render 调用OpenGL的纹理绘制函数绘制图片到界面
    ///
    void render() override{
        //qDebug()<<"render: "<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        m_nv12Render.render(ptr,videoW,videoH);
    }

    ///
    /// \brief createFramebufferObject 创建framebuffer格式
    /// \param size 需要创建的格式的尺寸
    /// \return 基于制动格式和尺寸的新的framebuffer对象
    ///
    QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) override{
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size,format);
    }

private:
    NV12Render m_nv12Render;//OpenGL渲染对象
    int videoW,videoH;//视频的宽度高度

    uchar *ptr=nullptr;//视频图像数据指针
};

VideoItem::VideoItem(QQuickItem *parent)
    :QQuickFramebufferObject (parent)
    ,m_decoder(new FFmpegDecoder)
{
    connect(m_decoder,&FFmpegDecoder::sigNewFrame,this,&VideoItem::update);
}

VideoItem::~VideoItem()
{
    m_timer->stop();
    delete m_timer;
    stop();
    delete m_decoder;
}

QQuickFramebufferObject::Renderer *VideoItem::createRenderer() const
{
    //qDebug()<<"Renderer";
    return new VideoFboItem;
}

void VideoItem::getFrame(uchar **ptr, int *w, int *h)
{
    //qDebug()<<"get frame ptr: "<<QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    *ptr = m_decoder->getFrame();
    *w = (int)m_decoder->width();
    *h = (int)m_decoder->height();

    return;
}

void VideoItem::setUrl(QString url)
{
    if(m_url != url){
        stop();
    }

    m_url=url;
    m_decoder->setUrl(url);
}

void VideoItem::start()
{
    stop();
    m_decoder->start();
}

void VideoItem::stop()
{
    if(m_decoder->isRunning()){
        m_decoder->requestInterruption();
        m_decoder->quit();
        m_decoder->wait();
    }
}
