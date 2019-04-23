#include <stdio.h>
#include "esp_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "internal/esp_wifi_internal.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/pwm.h"
#include "app_mqtt_handle.h"
#include "rom/ets_sys.h"
#include "driver/uart.h"

#define MQTT_DATA_PUBLISH "devPub"
#define MQTT_DATA_SUBLISH "devSub"
static const char *TAG = "XuHongLog";
static xQueueHandle userEventQueue = NULL;
xTaskHandle xHandlerMqtt = NULL;

int NetReconnect = 0, RunTime = 0;
static uint8_t receiveEventEn = 0;
typedef enum
{
    EVENT_MQTT_NULL = 0,
    EVENT_MQTT_FISRT_RUN,
    EVENT_MQTT_CONNECT_SUCC,
    EVENT_MQTT_CONNECTED,
    EVENT_MQTT_RECONNECT,
    EVENT_MQTT_HAS_NEW_MSG,
    EVENT_MQTT_UPLOAD_NETINFO,
} USER_EVENT;

void TaskXMqttStartConnect(void *p)
{

    xMQTT_Msg rMsg;
    uint8_t userEvent;
    ESP_LOGI(TAG, "xMqttReceiveMsg start");

    userEventQueue = xQueueCreate(10, sizeof(uint8_t));

    while (1)
    {
        if (xQueueReceive(userEventQueue, &userEvent, 10))
        {
            ESP_LOGI(TAG, "xQueueReceive userEventQueue %d \n", userEvent);
            switch (userEvent)
            {
            case EVENT_MQTT_CONNECTED:
                strcpy((char *)rMsg.topic, MQTT_DATA_SUBLISH);
                rMsg.qos = 1;
                xMqttSubTopic(&rMsg);
                break;
            case EVENT_MQTT_HAS_NEW_MSG:
                while (xMqttReceiveMsg(&rMsg))
                {
                    ESP_LOGI(TAG, "xMqttReceiveMsg clouds");
                    ESP_LOGI(TAG, "topic:\"%s\"", rMsg.topic);
                    ESP_LOGI(TAG, "payload[%d] : %s", rMsg.payloadlen, rMsg.payload);
                    ESP_LOGI(TAG, "free heap size = %d\n", esp_get_free_heap_size());

                    xMQTT_Msg msg;
                    strcpy((char *)msg.topic, MQTT_DATA_PUBLISH);
                    sprintf((char *)msg.payload, "{\"RSSI\":%d,\"xMqttVersion\":%s,\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
                            esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());

                    msg.payloadlen = msg.payload[2] + 3;
                    msg.qos = 1;
                    msg.retained = 0;
                    msg.dup = 0;
                    xMqttPublicMsg(&msg);
                }
                receiveEventEn = 1;
                break;

            default:
                break;
            };
        }
    }
}
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
    {
        xMqttConnectWifiNotify(WIFI_CONNECTED);
        esp_err_t ret = 0;
        if (xHandlerMqtt == NULL)
        {
            ret = xTaskCreate(TaskXMqttStartConnect, "TaskXMqttStart", 1024 * 2, NULL, 8, NULL); // 创建任务
            vTaskDelay(500 / portTICK_RATE_MS);
            ret = xTaskCreate(TaskMainMqtt, "TaskMainMqtt", 1024 * 6, NULL, 10, &xHandlerMqtt);
        }

        if (!ret)
        {
            ESP_LOGI(TAG, "xTaskCreate xMqttStartConnect1 fail");
        }
    }
    break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        xMqttConnectWifiNotify(WIFI_DISCONNECTED);
        esp_wifi_connect();
        break;

    default:
        break;
    }

    return ESP_OK;
}

void funXMqtt_event_callback(xMQTT_EVENT event)
{

    uint8_t userEvent = EVENT_MQTT_NULL;

    //ESP_LOGI(TAG, "funXMqtt_event_callback: %d", event);
    switch (event)
    {
    //连接成功回调
    case XP_MQTT_EVENT_MQTT_CONN_SUCC:

        receiveEventEn = 1;
        ESP_LOGI(TAG, "funXMqtt_event_callback connect success");
        userEvent = EVENT_MQTT_CONNECTED;
        if (userEventQueue != NULL)
        {
            xQueueReset(userEventQueue);
            xQueueSend(userEventQueue, &userEvent, 0);
        }
        break;
    //接收到新的消息下发
    case XP_MQTT_EVENT_RECEIVE_SUCC:
        if (receiveEventEn == 1)
        {
            receiveEventEn = 0;
            userEvent = EVENT_MQTT_HAS_NEW_MSG;
            xQueueSend(userEventQueue, &userEvent, 0);
        }
        break;
    //心跳回复
    case XP_MQTT_EVENT_HEART_BEAT:
        ESP_LOGI(TAG, "funXMqtt_event_callback heartBeat");
        {
            // xMQTT_Msg msg;
            // strcpy((char *)msg.topic, MQTT_DATA_PUBLISH);
            // sprintf((char *)msg.payload, "{\"RSSI\":%d,\"xMqttVersion\":\"%s\",\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
            //         esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());
            // msg.payloadlen = strlen((char *)msg.payload);
            // msg.qos = 1;
            // msg.retained = 0;
            // msg.dup = 0;
            // xMqttPublicMsg(&msg);

            xMQTT_Msg msg;
            strcpy((char *)msg.topic, MQTT_DATA_PUBLISH);

            uint8_t *tempS = &msg.payload[3];
            sprintf((char *)tempS, "{\"RSSI\":%d,\"xMqttVersion\":\"%s\",\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
                    esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());

            msg.payload[0] = 0x03;
            msg.payload[1] = 0x00;
            msg.payload[2] = strlen((char *)tempS);

            // sprintf((char *)msg.payload, "{\"RSSI\":%d,\"xMqttVersion\":%s,\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
            //         esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());

            msg.payloadlen = msg.payload[2] + 3;
            msg.qos = 1;
            msg.retained = 0;
            msg.dup = 0;
            xMqttPublicMsg(&msg);
        }
        break;
    //重连中
    case XP_MQTT_EVENT_MQTT_CONNECTING:
        ESP_LOGI(TAG, "funXMqtt_event_callback reConnect");
        NetReconnect++;
        break;
    case XP_MQTT_EVENT_SUBMSG_SUCC:
    {

        xMQTT_Msg msg;
        strcpy((char *)msg.topic, MQTT_DATA_PUBLISH);

        uint8_t *tempS = &msg.payload[3];
        sprintf((char *)tempS, "{\"RSSI\":%d,\"xMqttVersion\":\"%s\",\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
                esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());

        msg.payload[0] = 0x03;
        msg.payload[1] = 0x00;
        msg.payload[2] = strlen((char *)tempS);

        // sprintf((char *)msg.payload, "{\"RSSI\":%d,\"xMqttVersion\":%s,\"NetReconnect\":%d,\"RunTime\":%d,\"freeHeap\":%d}",
        //         esp_wifi_get_ap_rssi(), getXMqttVersion(), NetReconnect, RunTime, esp_get_free_heap_size());

        msg.payloadlen = msg.payload[2] + 3;
        msg.qos = 1;
        msg.retained = 0;
        msg.dup = 0;
        xMqttPublicMsg(&msg);
    }
    break;
    default:
        break;
    }
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "HUAWEI-APPT",
            .password = "xlinyun#123456",
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_LOGI(TAG, "Setting WiFi configuration password %s...", wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

os_timer_t os_timer;
void Task_pwm_blank(void)
{
    RunTime++;
    //ESP_LOGI(TAG, "runtime : %d", RunTime);
}
/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
    //获取IDF版本
    printf("     SDK version:%s\n", esp_get_idf_version());
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取芯片的内存分布，返回值具体见结构体 flash_size_map
    printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
    //获取mac地址（station模式）
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    printf("esp_read_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("--------------------------------------------------------------------------\n\n");
    initialise_wifi();
    xMQTT_CONFIG xMqttConfig =
        {
            .MQTTVersion = 4,
            .borkerHost = "www.xuhonys.cn", //固定
            .borkerPort = 1883,             //固定
            .mqttCommandTimeout = 6000,
            .username = "admin",     //产品id
            .password = "xuhong123", //鉴权信息
            .clientID = "521331497", //设备Id
            .keepAliveInterval = 60,
            .cleansession = true,
            .xmqtt_event_callback = funXMqtt_event_callback,
        };
    xMqttInit(&xMqttConfig);

    os_timer_disarm(&os_timer);
    os_timer_setfn(&os_timer, (os_timer_func_t *)(Task_pwm_blank), NULL);
    os_timer_arm(&os_timer, 1000, 1);
}
