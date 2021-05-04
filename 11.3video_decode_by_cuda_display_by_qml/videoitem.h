#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QQuickFramebufferObject>
#include <QTimer>
#include <QString>

#include "ffmpegdecoder.h"

class VideoItem : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    ///
    /// \brief VideoItem qml界面对象构造函数
    /// \param parent qml界面父指针
    ///
    explicit VideoItem(QQuickItem* parent = nullptr);

    ///
    /// \brief ~VideoItem 释放所有资源
    ~VideoItem() override;

    ///
    /// \brief createRenderer 创建一个渲染器对象
    /// \return 创建的对象指针
    ///
    Renderer *createRenderer() const override;

    ///
    /// \brief getFrame 从缓冲区里面获取一帧数据
    /// \param ptr 数据指针
    /// \param w 图像宽度
    /// \param h 图像高度
    ///
    void getFrame(uchar **ptr, int *w, int *h);

    ///
    /// \brief setUrl 设置解码视频流地址，可在qml中调用
    /// \param url 新的视频流地址
    ///
    Q_INVOKABLE void setUrl(QString url);

    ///
    /// \brief start 启动视频解码线程，开始显示视频
    ///
    Q_INVOKABLE void start();
    ///
    /// \brief start 停止视频解码线程，不显示视频
    ///
    Q_INVOKABLE void stop();

private:
    FFmpegDecoder *m_decoder;//解码对象

    QTimer *m_timer=nullptr;//界面刷新定时器

    QString m_url;//视频流地址变量
};

#endif // VIDEORENDER_H
