
## `xMqttEsp8266Rtos3.1Lib`

- 本仓库为 `esp8266 rtos 3.1` 环境的代码使用，如需拉取`esp32`代码请访问 https://github.com/xuhongv/xLibEsp32Rtos3.2Mqtt

----------
## 维护日志
----------

|  版本   |   更新日志  | 更新时间    |     
| --- | --- | --- | 
|  v1.0   |    初次提交 | 2019.04.10  |     
|  v1.1 |    修复接收数据流程bug |  2019.04.23   |    
|  v2.0 |    全新的 API 接口，使用更清晰！ |  2019.04.25   |    
|  v2.1 |    优化了上报数据速度！ |  2019.05.06  |    

### 一、简介；

----------

 - 对于2018年乐鑫出来的`esp-idf`框架的`rtos 3.0`版本全面改革，很多接口已经大大的修改了！这是个好事，说明官方在努力地做适配，的确，这个版本的内存足足省下了 10k 左右！对于 `esp8266` 来说，无疑是一个里程碑！

 - 本人半颗心脏在总结一些以往 `esp8266`的开发经验，把 这个 `MQTT`连接的断线重连不泄露内存等问题的框架弄好了！**下面是这个框架的一周多运行时间的状态上报的可视化界面！** 具体的上报看使用demo代码！
 
 - 如有问题技术或者讨论，请加付费QQ群：434878850 
 - 本仓库的核心代码是以静态库存在，**但不影响使用！** 如需此库的源码联系：870189248@qq.com
 - 本库支持 `esp8266 rtos3.1` 和 `esp32 rtos3.2`使用，**其使用提供的 API接口说明方法一样**，但具体细节使用有所不一样，链接如下:
   - `esp8266 ` : https://github.com/xuhongv/xLibEsp8266Rtos3.1Mqtt
   - `esp32 `:https://github.com/xuhongv/xLibEsp32Rtos3.2Mqtt


----------


<p align="center">
  <img src="png/header.png" width="950px" height="680px" alt="Banner" />
</p>


----------
### 二、如何使用；

- 第一步 先初始化连接参数

```
  xMQTT_CONFIG xMqttConfig =
        {
            .MQTTVersion = 4,
            .borkerHost = "193.112.51.78", //服务器地址
            .borkerPort = 1883,            //端口号
            .mqttCommandTimeout = 6000,  // 超时
            .username = "admin",       //用户名
            .password = "xuhongv2019", //用户密码
            .clientID = "521331497",   //设备Id
            .keepAliveInterval = 60, //心跳
            .cleansession = true, //清理会话
        };
    xMqttInit(&xMqttConfig);
  ```  
  - 第二步 创建接收任务，死循环读取 ```xMqttReceiveMsg```接口 ，此方法在 ```xmqtt.h``` 已经提供！请提供足够大的任务栈创建不断死循环读取，方法使用见代码示范的```TaskXMqttRecieve```方法！
  - 第三步 创建 ```TaskMainMqtt``` 任务，此方法在 ```xmqtt.h``` 已经提供！请提供足够大的任务栈创建！
  - 其他使用：
    - 发布消息：
    ```
                strcpy((char *)sMsg.topic, MQTT_DATA_PUBLISH);
                sprintf((char *)sMsg.payload, "{\"xMqttVersion\":%s,\"freeHeap\":%d}", getXMqttVersion(), esp_get_free_heap_size());
                sMsg.payloadlen = strlen((char *)sMsg.payload);
                sMsg.qos = 1;
                sMsg.retained = 0;
                sMsg.dup = 0;
                xMqttPublicMsg(&sMsg);
    ```
   - 订阅主题，请在连接成功后开始订阅主题：
     ```
                strcpy((char *)rMsg.topic, MQTT_DATA_SUBLISH);
                rMsg.qos = 1;
                xMqttSubTopic(&rMsg);
     ```
   - 切记当宿主程序网络发生改变，请调用 ```xMqttConnectWifiNotify()``` 方法通知 ```xmqtt``` ，以便迅捷地重连服务器！
     ```
      xMqttConnectWifiNotify(WIFI_CONNECTED); //通知wifi连接AP成功
      xMqttConnectWifiNotify(WIFI_DISCONNECTED);//通知wifi断开AP成功
     ```
   
  ----------
  
