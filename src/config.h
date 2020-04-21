/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// #define ZEPHYR_ADDR "192.168.1.101"
// #define SERVER_ADDR "192.168.0.29"
#define SERVER_PORT 1883

#define APP_SLEEP_MSECS 500
#define APP_TX_RX_TIMEOUT 300
#define APP_NET_INIT_TIMEOUT 10000

#define APP_CONNECT_TRIES 10

#define APP_MQTT_BUFFER_SIZE 128

#define MQTT_CLIENTID "zephyr_publisher"

/* Set the following to 1 to enable the Bluemix topic format */
#define APP_BLUEMIX_TOPIC 0

/* These are the parameters for the Bluemix topic format */
#if APP_BLUEMIX_TOPIC
#define BLUEMIX_DEVTYPE "sensor"
#define BLUEMIX_DEVID "carbon"
#define BLUEMIX_EVENT "status"
#define BLUEMIX_FORMAT "json"
#endif

#define CONFIG_NET_SAMPLE_APP_MAX_ITERATIONS 5
#define CONFIG_NET_SAMPLE_APP_MAX_CONNECTIONS 1

#endif
