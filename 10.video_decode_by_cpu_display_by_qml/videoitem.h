#pragma once
#include <memory>
#include <QQuickItem>
#include <QQuickFramebufferObject>
#include <QTimerEvent>
#include "ffmpegdecoder.h"

class VideoItem : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    VideoItem(QQuickItem *parent = nullptr);
    void timerEvent(QTimerEvent *event) override;

    YUVData getFrame();
    bool infoDirty() const
    {
        return m_infoChanged;
    }
    void makeInfoDirty(bool dirty)
    {
        m_infoChanged = dirty;
    }
    int videoWidth() const
    {
        return m_videoWidth;
    }
    int videoHeght() const
    {
        return m_videoHeight;
    }
public slots:
    void setUrl(const QString &url);
    void start();
    void stop();

protected slots:
    void onVideoInfoReady(int width, int height);
public:
    Renderer *createRenderer() const override;

    FFmpegDecoder *m_decoder = nullptr;

    int m_videoWidth;
    int m_videoHeight;
    bool m_infoChanged = false;
};

