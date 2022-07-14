#ifndef RTSPDATA_H
#define RTSPDATA_H

#include <QThread>
#include <QString>
#include <QObject>

#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netdb.h>    // gethostbyname
#include <arpa/inet.h> // htons/ntohs

#define RTSP_DEFAULT_PORT   (554)
#define RTSP_TIMEOUT        (3000)  // milliseconds

typedef struct RtspCntHeader {
    uint8_t  magic;        // 0x24
    uint8_t  channel;
    uint16_t length;
    uint8_t  payload[0];   // >> RtpHeader
} RtspCntHeader_st;

typedef struct RtpCntHeader {
#if 0
    uint8_t  version:2;
    uint8_t  padding:1;
    uint8_t  externsion:1;
    uint8_t  CSrcId:4;
    uint8_t  marker:1;
    uint8_t  pt:7;          // payload type
#else
    uint8_t  exts;
    uint8_t  type;
#endif
    uint16_t seqNo;         // Sequence number
    uint32_t ts;            // Timestamp
    uint32_t SyncSrcId;     // Synchronization source (SSRC) identifier
                            // Contributing source (SSRC_n) identifier
    uint8_t payload[0];     // Frame data
} RtpCntHeader_st;

#define RTSP_VERSIION       "RTSP/1.0"
#define RTSP_SESSION_NAME   "Session:"
#define RTSP_USERAGENT      "User-Agent: Darkise rtsp player 1.0\r\n"

class RTSPData : public QThread
{
    Q_OBJECT
public:
    RTSPData();
    // Init RTSP environment
    int rtspInit(char const* url);

    // Is the RTSP client started, if not try to start it
    int isStart();
    int rtsp_read();
    // Try to get a packet from socket
    int rtsp_packet();

    /// Operations:
    // RTSP operations
    int options(int to);
    int describe(int to);
    int setup(int to);
    int play(int to);
    int get_params(int to);
    int teardown(int to);
    int _setup_interleaved(int to);
    int _set_range(int to);
    //
    int _send_request(char const* req, int size);
    int _wait_response(int to, char resp[], size_t size);
    int _parse_session(char const* resp);   // SETUP
    int _parse_sdp(char const* resp);       // DESCRIBE

    // Rtp content operations
    int rtsp_init();

protected:
    void run();

private:
    // Command handler
    char _url[256];
    char host[64];
    int port = RTSP_DEFAULT_PORT;
    int rtspSocket = -1;
    int rtspTimeout = RTSP_TIMEOUT;
    // RTSP control sequeue number
    int CSeq;
    // Use only UDP protocol
    int clientPort[2] = { 37477, 37478 };
    int serverPort[2];
    // Get from SETUP's response, PLAY and TEARDOWN need it
    char sessionId[32];
    /* Media attributes:
     * a=x-dimensions:1920,1080
     * a=control:rtsp://192.168.199.30:554/h264/ch1/main/av_stream/trackID=1
     * a=rtpmap:96 H264/90000
     * a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z00AKY2NQDwBE/LCAAAOEAACvyAI,aO44gA==
     * a=Media_header:MEDIAINFO=494D4B48010100000400000100000000000000000000000000000000000000000000000000000000;
     * Concerned only
     */
    int video_width, video_height;
    char control[256];

    int rtp_size = 16*1024*1024;
    uint8_t* rtp_content; // Buffer for rtp
    int rtp_read, rtp_write;

    uint8_t         *packet_buffer;
    uint32_t        packet_wpos;
};

#endif // RTSPDATA_H
