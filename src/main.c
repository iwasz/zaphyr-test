#include "config.h"
#include "dhcp.h"
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/video.h>
#include <errno.h>
#include <logging/log.h>
#include <net/socket.h>
#include <string.h>
#include <zephyr.h>

LOG_MODULE_REGISTER (main, LOG_LEVEL_DBG);

#define VIDEO_CAPTURE_DEV "VIDEO_SW_GENERATOR"
#define MY_PORT 5000
#define MAX_CLIENT_QUEUE 1

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS (led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0 DT_GPIO_LABEL (LED0_NODE, gpios)
#define PIN DT_GPIO_PIN (LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS (LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0 ""
#define PIN 0
#define FLAGS 0
#endif

#define GREEN_LED_1_NODE DT_NODELABEL (green_led_1)

#if DT_NODE_HAS_STATUS(GREEN_LED_1_NODE, okay)
#define GREEN_LED_1 DT_GPIO_LABEL (GREEN_LED_1_NODE, gpios)
#define GREEN_LED_1_PIN DT_GPIO_PIN (GREEN_LED_1_NODE, gpios)
#define GREEN_LED_1_FLAGS DT_GPIO_FLAGS (GREEN_LED_1_NODE, gpios)
#else
#error "Unsupported board: green_led_1 devicetree label is not defined"
#define GREEN_LED_1 ""
#define GREEN_LED_1_PIN 0
#define GREEN_LED_1_FLAGS 0
#endif

#define RED_LED_1_NODE DT_NODELABEL (red_led_1)

#if DT_NODE_HAS_STATUS(RED_LED_1_NODE, okay)
#define RED_LED_1 DT_GPIO_LABEL (RED_LED_1_NODE, gpios)
#define RED_LED_1_PIN DT_GPIO_PIN (RED_LED_1_NODE, gpios)
#define RED_LED_1_FLAGS DT_GPIO_FLAGS (RED_LED_1_NODE, gpios)
#else
#error "Unsupported board: red_led_1 devicetree label is not defined"
#define RED_LED_1 ""
#define RED_LED_1_PIN 0
#define RED_LED_1_FLAGS 0
#endif

typedef struct sockaddr_in sockaddr_in_t;

static ssize_t sendall (int sock, const void *buf, size_t len, sockaddr_in_t *address, socklen_t addrLen)
{
        while (len) {
                // ssize_t out_len = send (sock, buf, len, 0);
                ssize_t out_len = sendto (sock, buf, len, 0, (const struct sockaddr *)address, addrLen);

                if (out_len < 0) {
                        LOG_ERR ("sendto error");
                }

                if (out_len < 0) {
                        return out_len;
                }
                buf = (const char *)buf + out_len;
                len -= out_len;
        }

        return 0;
}

void main ()
{
        {
                const struct device *dev = device_get_binding (RED_LED_1);

                if (dev == NULL) {
                        return;
                }

                int ret = gpio_pin_configure (dev, RED_LED_1_PIN, GPIO_OUTPUT_ACTIVE | RED_LED_1_FLAGS);

                if (ret < 0) {
                        return;
                }

                bool led_is_on = true;

                while (1) {
                        gpio_pin_set (dev, RED_LED_1_PIN, (int)led_is_on);
                        led_is_on = !led_is_on;
                        k_msleep (500);
                }
        }

        /********************************************/

        // dhcpInit ();

        // while (!dhcpIsnitializded ()) {
        //         k_sleep (K_MSEC (2000));
        // }

        struct sockaddr_in addr, client_addr;
        socklen_t client_addr_len = sizeof (client_addr);
        struct video_buffer *buffers[2], *vbuf;
        int i, ret, sock;
        struct video_format fmt;

        /* Prepare Network */
        (void)memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons (MY_PORT);

        sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sock < 0) {
                LOG_ERR ("Failed to create TCP socket: %d", errno);
                return;
        }

        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons (MY_PORT);
        inet_pton (AF_INET, "192.168.0.29", &client_addr.sin_addr);

        /* Prepare Video Capture */
        const struct device *video = device_get_binding (VIDEO_CAPTURE_DEV);

        if (video == NULL) {
                LOG_ERR ("Video device %s not found. Aborting test.", VIDEO_CAPTURE_DEV);
                return;
        }

        struct video_format vf = {VIDEO_PIX_FMT_RGB565, 160, 80, 160 * 2};
        if (video_set_format (video, VIDEO_EP_OUT, &vf)) {
                LOG_ERR ("Unable to set video format");
                return;
        }

        /* Get default/native format */
        if (video_get_format (video, VIDEO_EP_OUT, &fmt)) {
                LOG_ERR ("Unable to retrieve video format");
                return;
        }

        LOG_INF ("Video device detected, format: %c%c%c%c %ux%u", (char)fmt.pixelformat, (char)(fmt.pixelformat >> 8),
                 (char)(fmt.pixelformat >> 16), (char)(fmt.pixelformat >> 24), fmt.width, fmt.height);

        /* Alloc Buffers */
        for (i = 0; i < ARRAY_SIZE (buffers); i++) {
                buffers[i] = video_buffer_alloc (fmt.pitch * fmt.height);
                if (buffers[i] == NULL) {
                        LOG_ERR ("Unable to alloc video buffer");
                        return;
                }
        }

        /* Connection loop */
        do {
                /* Enqueue Buffers */
                for (i = 0; i < ARRAY_SIZE (buffers); i++) {
                        video_enqueue (video, VIDEO_EP_OUT, buffers[i]);
                }

                /* Start video capture */
                if (video_stream_start (video)) {
                        LOG_ERR ("Unable to start video");
                        return;
                }

                LOG_INF ("Stream started");

                /* Capture loop */
                i = 0;
                do {
                        ret = video_dequeue (video, VIDEO_EP_OUT, &vbuf, K_FOREVER);

                        if (ret) {
                                LOG_ERR ("Unable to dequeue video buf");
                                return;
                        }

                        // LOG_INF ("Sending frame %d", i++);
                        // k_sleep (K_MSEC (100));

                        ret = sendall (sock, vbuf->buffer, vbuf->bytesused, &client_addr, client_addr_len);
                        // static uint8_t buf[4800];

                        // ret = sendall (client, buf, 480); // Random data
                        // k_sleep (K_MSEC (1000));

                        if (ret && ret != -EAGAIN) {
                                /* client disconnected */
                                LOG_INF ("TCP: Client disconnected %d", ret);
                                close (sock);
                        }

                        (void)video_enqueue (video, VIDEO_EP_OUT, vbuf);

                } while (!ret);

                /* stop capture */
                // if (video_stream_stop (video)) {
                //         LOG_ERR ("Unable to stop video");
                //         return;
                // }

                /* Flush remaining buffers */
                // do {
                //         ret = video_dequeue (video, VIDEO_EP_OUT, &vbuf, K_NO_WAIT);
                // } while (!ret);

        } while (1);

        while (true) {
                k_sleep (K_MSEC (200));
        }

        // exit (r);
}
