#include "videoitem.h"
#include "i420render.h"
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>

//************VideoItemRender************//
class VideoFboItem : public QQuickFramebufferObject::Renderer
{
public:
    VideoFboItem(){
        m_render.init();
    }

    void render() override{
        m_render.paint();
        m_window->resetOpenGLState();
    }
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override{
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        m_render.resize(size.width(), size.height());
        return new QOpenGLFramebufferObject(size, format);
    }
    void synchronize(QQuickFramebufferObject *item) override{
        VideoItem *pItem = qobject_cast<VideoItem *>(item);
        if (pItem)
        {
            if (!m_window)
            {
                m_window = pItem->window();
            }
            if (pItem->infoDirty())
            {
                m_render.updateTextureInfo(pItem->videoWidth(), pItem->videoHeght());
                pItem->makeInfoDirty(false);
            }
            ba = pItem->getFrame();
            m_render.updateTextureData(ba);
        }
    }
private:
    I420Render m_render;
    QQuickWindow *m_window = nullptr;

    YUVData ba;
};

//************VideoItem************//
VideoItem::VideoItem(QQuickItem *parent) : QQuickFramebufferObject (parent)
{
    m_decoder = new FFmpegDecoder;
    connect(m_decoder,&FFmpegDecoder::videoInfoReady,this,&VideoItem::onVideoInfoReady);

    startTimer(24);
}

void VideoItem::timerEvent(QTimerEvent *)
{
    update();
}

YUVData VideoItem::getFrame()
{
    return m_decoder->getFrame();
}

void VideoItem::setUrl(const QString &url)
{
    m_decoder->setUrl(url);
}

void VideoItem::start()
{
    m_decoder->start();
}

void VideoItem::stop()
{
    if(m_decoder->isRunning()){
        m_decoder->quit();
        m_decoder->wait(1000);
    }
}

void VideoItem::onVideoInfoReady(int width, int height)
{
    if (m_videoWidth != width)
    {
        m_videoWidth = width;
        makeInfoDirty(true);
    }
    if (m_videoHeight != height)
    {
        m_videoHeight = height;
        makeInfoDirty(true);
    }
}

QQuickFramebufferObject::Renderer *VideoItem::createRenderer() const
{
    return new VideoFboItem;
}

