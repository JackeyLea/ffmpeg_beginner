TEMPLATE = subdirs

SUBDIRS += 03.get_lib_version \
           04.get_stream_info \
           05.video_decode_flow \
           06.video_decode_frame_save \
           07.1.video_decode_mp42yuv420p \
           07.2.video_decode_mp42yuv420sp \
           08.video_decode_by_cpu_display_by_qwidget \
           09.1.video_decode_by_cpu_display_by_qopengl \
           09.2.video_decode_by_cpu_display_by_qopengl \
           10.video_decode_by_cpu_display_by_qml \
           11.1.video_decode_by_cuda_display_by_qwidget \
           11.2.video_decode_by_cuda_display_by_qopengl \
           11.3.video_decode_by_cuda_display_by_qml \
           12.video_encode_yuv2h264 \
           13.video_encode_h2642mp4 \
           14.video_encode_camera2h264 \
           15.audio_decode_mp32pcm \
           16.audio_decode_swr_mp32pcm \
           17.audio_player_decode_by_ffmpeg_play_by_qt \
           18.audio_player_decode_from_mem_play_by_qt \
           19.audio_encode_pcm2mp3 \
           20.audio_video_sync \
           21.video_decode_add_filter_display_by_qwidget \
           22.video_demuxer_mp42h264mp3 \
           23.video_demuxer_mp42yuvpcm \
           24.video_muxer_mp3h2642mp4
