# ffmpeg_beginner

ffmpeg入门系列教程代码

<a href="https://feater.top/series/ffmpeg/78/">FFmpeg4入门系列教程索引</a>

## 源码

### 3.get_lib_version

获取库版本信息并解析输出可读信息

### 4.get_stream_info

输出视频的基本信息（时长、码率、编码方式等等）

### 5.video_decode_flow

视频解码的基本流程

### 6.video_decode_frame_save

解码视频并保存其中的50帧画面为ppm格式图片

### 7.video_decode_mp42yuv

视频解码的基本流程并输出视频信息，将解码后的视频数据保存为YUV格式文件

### 8.video_decode_by_cpu_display_by_qwidget 

使用CPU解码视频，然后使用Qt的QWidget显示画面

### 9.video_decode_by_cpu_display_by_qopengl

使用CPU解码视频，然后使用Qt的QOpenGL显示画面

### 10.video_decode_by_cpu_display_by_qml

使用CPU解码视频，然后使用QML显示画面

### 11

使用CUDA解码视频并显示

#### 11.1video_decode_by_cuda_display_qwidget

使用CUDA解码视频，然后使用Qt的QWidget显示视频

#### 11.2video_decode_by_cuda_display_qopengl

使用CUDA解码视频，然后使用Qt的QOpenGL显示视频

#### 11.3video_decode_by_cuda_display_qml

使用CUDA解码视频，然后使用QML显示视频

### 12.video_encode_yuv2h264

将yuv源视频文件编码为h264格式的文件

### 13.video_encode_h2642mp4

将h264编码为mp4格式文件

### 14.video_encode_camera2h264

将摄像头捕获的视频直接编码为H264格式

### 15.audio_decode_mp32pcm

将mp3文件解码为pcm文件

### 16.audio_decode_swr_mp32pcm

将mp3音频重采样解码为pcm

### 17.audio_player_decode_by_ffmpeg_play_by_qt

使用FFmpeg解码音频，使用Qt播放音频

### 18.audio_encode_pcm2mp3

将pcm格式文件编码为mp3格式

### 20.video_decode_add_filter_display_by_qwidget

使用CPU解码视频，并添加滤镜，然后使用QWidget显示画面