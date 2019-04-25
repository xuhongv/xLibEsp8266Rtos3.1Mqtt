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
#include "xmqtt.h"
#include "rom/ets_sys.h"
#include "driver/uart.h"

#define MQTT_DATA_PUBLISH "/a1EQVC8sNnR/48bf1822fde65a9a/UserOut"
#define MQTT_DATA_SUBLISH "/a1EQVC8sNnR/48bf1822fde65a9a/UserIn"
static const char *TAG = "XuHongLog";
xTaskHandle xHandlerMqtt = NULL;

void TaskXMqttRecieve(void *p)
{

    xMQTT_Msg rMsg;
    xMQTT_Msg sMsg;
    ESP_LOGI(TAG, "xMqttReceiveMsg start");
    while (1)
    {
        if (xMqttReceiveMsg(&rMsg))
        {
            switch (rMsg.type)
            {
                //接收到新的消息下发
            case xMQTT_TYPE_RECIEVE_MSG:
                ESP_LOGI(TAG, "xQueueReceive topic: %s ", rMsg.topic);
                ESP_LOGI(TAG, "xQueueReceive payload: %s", rMsg.payload);
                ESP_LOGI(TAG, "esp_get_free_heap_size : %d \n", esp_get_free_heap_size());

                strcpy((char *)sMsg.topic, MQTT_DATA_PUBLISH);
                sprintf((char *)sMsg.payload, "{\"xMqttVersion\":%s,\"freeHeap\":%d}", getXMqttVersion(), esp_get_free_heap_size());
                sMsg.payloadlen = strlen((char *)sMsg.payload);
                sMsg.qos = 1;
                sMsg.retained = 0;
                sMsg.dup = 0;
                xMqttPublicMsg(&sMsg);

                break;
                //连接Mqtt服务器成功
            case xMQTT_TYPE_CONNECTED:
                strcpy((char *)rMsg.topic, MQTT_DATA_SUBLISH);
                rMsg.qos = 1;
                xMqttSubTopic(&rMsg);
                strcpy((char *)rMsg.topic, MQTT_DATA_PUBLISH);
                rMsg.qos = 0;
                xMqttSubTopic(&rMsg);
                ESP_LOGI(TAG, "xMQTT : xMQTT_TYPE_CONNECTED");
                break;
                //断开Mqtt服务器成功
            case xMQTT_TYPE_DISCONNECTED:
                ESP_LOGI(TAG, "xMQTT : xMQTT_TYPE_DISCONNECTED");
                break;
                //正在连接Mqtt服务器
            case xMQTT_TYPE_CONNECTTING:
                ESP_LOGI(TAG, "xMQTT : xMQTT_TYPE_CONNECTTING");
                break;
                //订阅主题成功
            case xMQTT_TYPE_SUB_SUCCESS:
                ESP_LOGI(TAG, "xMQTT : xMQTT_TYPE_SUB_SUCCESS");
                break;
                //ping心跳服务器
            case xMQTT_TYPE_SEND_PING:
                ESP_LOGI(TAG, "xMQTT : xMQTT_TYPE_SEND_PING");
                break;
            default:
                break;
            }
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
            ret = xTaskCreate(TaskXMqttRecieve, "TaskXMqttRecieve", 1024 * 2, NULL, 8, NULL); // 创建任务
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
            .borkerHost = "www.domino.cn", //固定
            .borkerPort = 1883,             //固定
            .mqttCommandTimeout = 6000,
            .username = "admin",     //产品id
            .password = "xuhong", //鉴权信息
            .clientID = "521331497", //设备Id
            .keepAliveInterval = 60,
            .cleansession = true,
        };
    xMqttInit(&xMqttConfig);
}
