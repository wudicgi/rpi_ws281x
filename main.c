/*
 * newtest.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


static char VERSION[] = "XX.YY.ZZ";

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>


#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"
#include "version.h"

#include "ws2811.h"


#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                12  // 18
#define DMA                     10
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE              WS2811_STRIP_GBR		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)
#define STRIP_TYPE              WS2811_STRIP_GRB

#define WIDTH                   12  // 8
#define HEIGHT                  6   // 8
#define LED_COUNT               (WIDTH * HEIGHT)

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

int clear_on_exit = 0;

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

ws2811_led_t *matrix;

static uint8_t running = 1;

void matrix_render(void)
{
    int x, y;

    for (x = 0; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            ledstring.channel[0].leds[(y * width) + x] = matrix[y * width + x];
        }
    }
}

void matrix_raise(void)
{
    int x, y;

    for (y = 0; y < (height - 1); y++)
    {
        for (x = 0; x < width; x++)
        {
            // This is for the 8x8 Pimoroni Unicorn-HAT where the LEDS in subsequent
            // rows are arranged in opposite directions
            matrix[y * width + x] = matrix[(y + 1)*width + width - x - 1];
        }
    }
}

void matrix_clear(void)
{
    int x, y;

    for (y = 0; y < (height ); y++)
    {
        for (x = 0; x < width; x++)
        {
            matrix[y * width + x] = 0;
        }
    }
}

int dotspos[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
ws2811_led_t dotcolors[] =
{
    0x00200000,  // red
    0x00201000,  // orange
    0x00202000,  // yellow
    0x00002000,  // green
    0x00002020,  // lightblue
    0x00000020,  // blue
    0x00100010,  // purple
    0x00200010,  // pink
};

ws2811_led_t dotcolors_rgbw[] =
{
    0x00200000,  // red
    0x10200000,  // red + W
    0x00002000,  // green
    0x10002000,  // green + W
    0x00000020,  // blue
    0x10000020,  // blue + W
    0x00101010,  // white
    0x10101010,  // white + W

};

void matrix_bottom(void)
{
    int i;

    for (i = 0; i < (int)(ARRAY_SIZE(dotspos)); i++)
    {
        dotspos[i]++;
        if (dotspos[i] > (width - 1))
        {
            dotspos[i] = 0;
        }

        if (ledstring.channel[0].strip_type == SK6812_STRIP_RGBW) {
            matrix[dotspos[i] + (height - 1) * width] = dotcolors_rgbw[i];
        } else {
            matrix[dotspos[i] + (height - 1) * width] = dotcolors[i];
        }
    }
}

static void ctrl_c_handler(int signum)
{
	(void)(signum);
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


void parseargs(int argc, char **argv, ws2811_t *ws2811)
{
	int index;
	int c;

	static struct option longopts[] =
	{
		{"help", no_argument, 0, 'h'},
		{"dma", required_argument, 0, 'd'},
		{"gpio", required_argument, 0, 'g'},
		{"invert", no_argument, 0, 'i'},
		{"clear", no_argument, 0, 'c'},
		{"strip", required_argument, 0, 's'},
		{"height", required_argument, 0, 'y'},
		{"width", required_argument, 0, 'x'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while (1)
	{

		index = 0;
		c = getopt_long(argc, argv, "cd:g:his:vx:y:", longopts, &index);

		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* handle flag options (array's 3rd field non-0) */
			break;

		case 'h':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			fprintf(stderr, "Usage: %s \n"
				"-h (--help)    - this information\n"
				"-s (--strip)   - strip type - rgb, grb, gbr, rgbw\n"
				"-x (--width)   - matrix width (default 8)\n"
				"-y (--height)  - matrix height (default 8)\n"
				"-d (--dma)     - dma channel to use (default 10)\n"
				"-g (--gpio)    - GPIO to use\n"
				"                 If omitted, default is 18 (PWM0)\n"
				"-i (--invert)  - invert pin output (pulse LOW)\n"
				"-c (--clear)   - clear matrix on exit.\n"
				"-v (--version) - version information\n"
				, argv[0]);
			exit(-1);

		case 'D':
			break;

		case 'g':
			if (optarg) {
				int gpio = atoi(optarg);
/*
	PWM0, which can be set to use GPIOs 12, 18, 40, and 52.
	Only 12 (pin 32) and 18 (pin 12) are available on the B+/2B/3B
	PWM1 which can be set to use GPIOs 13, 19, 41, 45 and 53.
	Only 13 is available on the B+/2B/PiZero/3B, on pin 33
	PCM_DOUT, which can be set to use GPIOs 21 and 31.
	Only 21 is available on the B+/2B/PiZero/3B, on pin 40.
	SPI0-MOSI is available on GPIOs 10 and 38.
	Only GPIO 10 is available on all models.

	The library checks if the specified gpio is available
	on the specific model (from model B rev 1 till 3B)

*/
				ws2811->channel[0].gpionum = gpio;
			}
			break;

		case 'i':
			ws2811->channel[0].invert=1;
			break;

		case 'c':
			clear_on_exit=1;
			break;

		case 'd':
			if (optarg) {
				int dma = atoi(optarg);
				if (dma < 14) {
					ws2811->dmanum = dma;
				} else {
					printf ("invalid dma %d\n", dma);
					exit (-1);
				}
			}
			break;

		case 'y':
			if (optarg) {
				height = atoi(optarg);
				if (height > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					printf ("invalid height %d\n", height);
					exit (-1);
				}
			}
			break;

		case 'x':
			if (optarg) {
				width = atoi(optarg);
				if (width > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					printf ("invalid width %d\n", width);
					exit (-1);
				}
			}
			break;

		case 's':
			if (optarg) {
				if (!strncasecmp("rgb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RGB;
				}
				else if (!strncasecmp("rbg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RBG;
				}
				else if (!strncasecmp("grb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GRB;
				}
				else if (!strncasecmp("gbr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GBR;
				}
				else if (!strncasecmp("brg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BRG;
				}
				else if (!strncasecmp("bgr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BGR;
				}
				else if (!strncasecmp("rgbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_RGBW;
				}
				else if (!strncasecmp("grbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_GRBW;
				}
				else {
					printf ("invalid strip %s\n", optarg);
					exit (-1);
				}
			}
			break;

		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(-1);

		case '?':
			/* getopt_long already reported error? */
			exit(-1);

		default:
			exit(-1);
		}
	}
}


#include <pthread.h>

typedef void * (*ql_thread_fn)(void *);

typedef struct ql_thread_t {
    pthread_t thread;
} ql_thread_t;

ql_thread_t *ql_thread_create(int priority, int stack_size, ql_thread_fn fn, void* arg)
{
    int ret = 0;

    ql_thread_t *ql_thread = (ql_thread_t *)malloc(sizeof(ql_thread_t));
    if (NULL == ql_thread) {
        return NULL;
    }

    ret = pthread_create(&(ql_thread->thread), NULL, fn, arg);
    if (ret != 0) {
        free(ql_thread);
        return NULL;
    }

    return ql_thread;
}

void ql_thread_destroy(ql_thread_t **thread)
{
    if (thread && *thread) {
        free(*thread);
        *thread = NULL;
    }
}

void ql_thread_schedule(void)
{
    return;
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <errno.h>

typedef struct ql_udp_socket {
    int sockfd;
    unsigned int local_ip;
    unsigned int local_port;
    unsigned int remote_ip;
    unsigned int remote_port;
} ql_udp_socket_t;


ql_udp_socket_t *ql_udp_socket_create(unsigned int local_port)
{
    ql_udp_socket_t *socket = NULL;

    socket = (ql_udp_socket_t *)malloc(sizeof(ql_udp_socket_t));
    memset(socket, 0, sizeof(ql_udp_socket_t));
    socket->sockfd = -1;
    socket->local_port = local_port;

    return socket;
}

void ql_udp_socket_destroy(ql_udp_socket_t **sock)
{
    if (sock && *sock) {
        free(*sock);
    }

    *sock = NULL;
}

int ql_udp_bind(ql_udp_socket_t *sock)
{
    if (sock == NULL) {
        return -1;
    }

    sock->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock->sockfd == -1) {
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(sock->local_port);

    int re_flag = 1;
    int re_len=sizeof(int);
    setsockopt(sock->sockfd, SOL_SOCKET, SO_REUSEADDR, &re_flag, re_len);

    if(bind(sock->sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return -1;
    }
    return 0;
}
int ql_udp_close(ql_udp_socket_t *sock)
{
    if (sock && sock->sockfd != -1) {
        close(sock->sockfd);
        sock->sockfd = -1;
    }
    return 0;
}

int ql_udp_send(ql_udp_socket_t *sock, unsigned char *buf, unsigned int len, unsigned int timeout_ms)
{
    int ret = -1;
    int sentlen = 0;
    int totallen = 0;

    if (sock == NULL || sock->sockfd == -1) {
        return ret;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = sock->remote_ip;
    addr.sin_port = sock->remote_port;

    struct timeval tv;
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock->sockfd, &wfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if(ret = select(sock->sockfd + 1, NULL, &wfds, NULL, &tv) >= 0) {
        if(FD_ISSET(sock->sockfd, &wfds))
        {
            totallen = len;
            while (sentlen < totallen) {
                ret = sendto(sock->sockfd, buf + sentlen, totallen - sentlen, 0, (struct sockaddr *)&addr, sizeof(addr));
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                        continue;
                    }else {
                        break;
                    }
                }
                sentlen += ret;
            }
            if(ret > 0) {
                ret = totallen;
            }
        }
    } else {
        printf("udp_s select err:%d\n", ret);
    }

    return ret;
}

int ql_udp_recv(ql_udp_socket_t *sock, unsigned char *buf, unsigned int len, unsigned int timeout_ms)
{
    struct timeval tv;
    fd_set readfds;
    int n=0;
    int ret = -1;

    struct sockaddr_in remote_addr;
    bzero(&remote_addr, sizeof(remote_addr));
    int addr_len = sizeof(remote_addr);

    FD_ZERO(&readfds);
    FD_SET(sock->sockfd, &readfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if(ret = select(sock->sockfd + 1, &readfds, NULL, NULL, &tv) >= 0) {
        if(FD_ISSET(sock->sockfd, &readfds))
        {
           if((n = recvfrom(sock->sockfd, buf, len, 0, (struct sockaddr*)&remote_addr, &addr_len )) >= 0)
           {
               sock->remote_port = remote_addr.sin_port;
               sock->remote_ip   = remote_addr.sin_addr.s_addr;
               ret = n;
           }
        } else {
            ret = 0;
        }
    } else {
        printf("udp_r select err:%d\n", ret);
    }
    return ret;
}



#define OSI_STACK_SIZE_ (32 * 1024)

static ql_thread_t *g_qlcloud_back_logic = NULL;
static ql_thread_t *g_qlcloud_back_time = NULL;

#define UDP_SERVER_PORT                 1025

#define QL_UDP_FRAME_MAX_SIZE           (8 * 1024)

unsigned char g_udp_buf[QL_UDP_FRAME_MAX_SIZE];

#include "Packet.h"
#include "PacketProcessor.h"
#include "FrameBuffer.h"

#define RESPONSE_PACKET_BUFFER_SIZE     512
static uint8_t _responsePacketBuffer[RESPONSE_PACKET_BUFFER_SIZE] = {0};

#define RGB_DATA_BUFFER_SIZE            (3 * 256)
static uint8_t _rgbDataBuffer[RGB_DATA_BUFFER_SIZE] = {0};

void *thread_udp(void *para) {
	int length;
    unsigned char *data = g_udp_buf;
	static ql_udp_socket_t *udp_socket = NULL;

    udp_socket = ql_udp_socket_create(UDP_SERVER_PORT);
    if (udp_socket == NULL) {
        return;
    }

    if(ql_udp_bind(udp_socket)){
        printf("udp bind err.\n");
        return;
    }

    while (1) {
        length = ql_udp_recv(udp_socket, data, QL_UDP_FRAME_MAX_SIZE, 200);

        if (length < 0) {
            printf("udp recv err:%d\n", length);
        } else if (length) {
            data[length] = 0;
//            printf("ql_udp_recv length:%d\r\n", length);
//            platform_data_parser(udp_socket, data, length);

            if ((data == NULL) || (length < PACKET_MIN_LENGTH)) {
                continue;
            }

            unsigned short response_length = PacketProcessor_processPacket((uint8_t *)data, length, _responsePacketBuffer);
            if (response_length == 0) {
                continue;
            }

            ql_udp_send(udp_socket, _responsePacketBuffer, response_length, 20);
        }
    }
}

void *thread_display(void *para) {
    while (1) {
        usleep((1000000 / 25) / 4);
    }
}

void test_func(void) {
    g_qlcloud_back_logic = ql_thread_create(1, OSI_STACK_SIZE_, (ql_thread_fn)thread_udp, NULL);

//    g_qlcloud_back_time = ql_thread_create(1, OSI_STACK_SIZE_, (ql_thread_fn)thread_display, NULL);
}

#include <sys/time.h>

static uint32_t _getTimeStampInUs() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (uint32_t)(tv.tv_sec*(uint64_t)1000000+tv.tv_usec);
}

void Ws2812_convertRgbData(uint8_t *srcData, int srcLength) {
    int ledCount = srcLength / 3;

    if (ledCount > (RGB_DATA_BUFFER_SIZE / 3)) {
        ledCount = (RGB_DATA_BUFFER_SIZE / 3);
    }

    uint8_t r, g, b;
    for (int i = 0; i < ledCount; i++) {
        r = *(srcData++);
        g = *(srcData++);
        b = *(srcData++);

        r = ((uint32_t)r * 3) / 4;
        g = ((uint32_t)g * 3) / 4;
        b = ((uint32_t)b * 3) / 4;

//        matrix[i] = (uint32_t)(((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);

        ledstring.channel[0].leds[i] = (uint32_t)(((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);
    }
}

int main(int argc, char *argv[])
{
    ws2811_return_t ret;

    sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

    /*
    while (1) {
        usleep(1000000 / 15);
    }
    */

    parseargs(argc, argv, &ledstring);

    matrix = malloc(sizeof(ws2811_led_t) * width * height);

    setup_handlers();

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    printf("aaa111\n");

    if (g_qlcloud_back_logic == NULL) {
        test_func();
    }

    printf("aaa222\n");

    while (running)
    {
        bool hasNewData = false;
        {
            uint32_t deviceTime = _getTimeStampInUs();
//            printf("deviceTime = %u\n", deviceTime);

            if (FrameBuffer_canRead(deviceTime)) {
                uint8_t *data;
                int length;

                if (FrameBuffer_read(&data, &length)) {
                    memcpy(_rgbDataBuffer, data, length);
                    hasNewData = true;
                }
            }
        }

//        printf("hasNewData = %s\n", (hasNewData ? "true" : "false"));

        if (hasNewData) {
            Ws2812_convertRgbData(_rgbDataBuffer, (width * height * 3));
//            matrix_render();

            if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
            {
                fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
                break;
            }
        }

        usleep((1000000 / 25) / 4);

        /*
        matrix_raise();
        matrix_bottom();
        matrix_render();

        if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
        {
            fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
            break;
        }

        // 15 frames /sec
        usleep(1000000 / 15);
        */
    }

    if (clear_on_exit) {
	matrix_clear();
	matrix_render();
	ws2811_render(&ledstring);
    }

    ws2811_fini(&ledstring);

    printf ("\n");
    return ret;
}
