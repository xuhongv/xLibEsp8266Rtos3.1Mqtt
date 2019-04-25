#ifndef __XMQTT_H
#define __XMQTT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "string.h"
#include "esp_libc.h"

//接收数据缓存包设置
#define RECV_BUFF_SIZE 512
#define MQTT_PACK_SIZE 512

#define XP_MQTT_MAX_TOPICLEN 50
#define XP_MQTT_RCV_MSG_QUEUE_LEN 10
#define XP_MQTT_SEND_MSG_QUEUE_LEN 10

//错误代码
typedef enum
{
    XP_MQTT_SUCCESS = 0, //成功
    XP_MQTT_ERR_MALLOC_FAIL = -1,
    XP_MQTT_ERR_NO_START = -2,
} xMQTT_ERR_CODE;

typedef enum
{
    MQTT_DISCONNECTED = 0,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    MQTT_LINKLOST,
} mqtt_linkstate_t;

typedef struct
{
    uint8_t *Pack;             // buff use for mqtt pack
    uint8_t *Data;             // data stream p for mqtt data
    int DataLen;               // length of data steam
    uint8_t pingCnt;           // number of ping, clean after receive PINGRESP
    mqtt_linkstate_t LinkFlag; // linkstate
} mqtt_transfer_t;

typedef struct
{
    uint8_t *buff;
    int buff_len;
} transport_recv_t;

typedef enum
{
    WIFI_CONNECTED = 0,
    WIFI_DISCONNECTED,
} wifi_stauts_t;

typedef struct
{
    char *borkerHost;
    char *username;
    char *password;
    char *clientID;
    uint8_t MQTTVersion;
    uint8_t cleansession;
    uint16_t borkerPort;
    uint16_t keepAliveInterval;
    uint32_t mqttCommandTimeout;
} xMQTT_CONFIG;

typedef enum
{
    xMQTT_TYPE_CONNECTED = 0,
    xMQTT_TYPE_SYSTEM_SUB_TOPIC,
    xMQTT_TYPE_SYSTEM_PUB_TOPIC,

    xMQTT_TYPE_DISCONNECTED,
    xMQTT_TYPE_CONNECTTING,
    xMQTT_TYPE_SUB_SUCCESS,
    xMQTT_TYPE_RECIEVE_MSG,
    xMQTT_TYPE_SEND_PING,
} xMQTT_Msg_Type;

typedef struct
{
    uint8_t qos;
    uint8_t retained;
    uint8_t dup;
    uint8_t topic[XP_MQTT_MAX_TOPICLEN + 1];
    uint8_t payload[RECV_BUFF_SIZE + 1];
    uint16_t payloadlen;
    xMQTT_Msg_Type type;
} xMQTT_Msg;

/**
 * @description: 通知mqtt当前的网络连接状态
 * @param {type} 0:表示成功连接路由器；1表示断开连接路由器
 * @return: 
 */
void xMqttConnectWifiNotify(wifi_stauts_t state);

/**
 * @description: 连接服务器任务
 * @param {type} 
 * @return: 
 */
void TaskMainMqtt(void *pvParameters);

/**
 * @description: 服务器配置
 * @param {type} 
 * @return: 
 */
xMQTT_ERR_CODE xMqttInit(xMQTT_CONFIG *config);

/**
 * @description: 发布消息
 * @param {type} 
 * @return: 
 */
void xMqttPublicMsg(xMQTT_Msg *mqttMsg);

/**
 * @description: 订阅主题
 * @param {type} 
 * @return: 
 */
void xMqttSubTopic(xMQTT_Msg *mqttMsg);

/**
 * @description:  接收Mqtt信息
 * @param {type} 
 * @return: 
 */
xMQTT_ERR_CODE xMqttReceiveMsg(xMQTT_Msg *msg);

/**
 * @description: 获取版本
 * @param {type} 
 * @return: 
 */
char *getXMqttVersion();

#endif