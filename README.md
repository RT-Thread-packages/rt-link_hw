# rt-link_hw
## 简介

rt-link_hw 软件包是 rt-link 组件的底层通信端口适配层。由不同端口类型的移植文件组成，用于适配不同类型的通信端口，目前支持的通信端口类型有：UART、TCP、UDP 等。

## 目录结构
| 名称 | 说明                                              |
| ---- | ------------------------------------------------- |
| uart | 用于 rt-link 组件适配 UART 端口连接，实现数据收发 |
| tcp  | 用于 rt-link 组件适配 TCP 端口连接，实现数据收发  |
| udp  | 用于 rt-link 组件适配 UDP 端口连接，实现数据收发  |

## 依赖

- rt-link 组件
- UART 依赖 rtdevice框架
- TCP、UDP 依赖 SAL 组件

## 如何添加新的端口类型

可以根据 [rt-link 组件文档](https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/programming-manual/rtlink/rtlink?id=底层链路对接接口介绍)，了解需要对接的 rt-link 接口，并参考现有的实现方式来完成对新的端口类型的移植对接。

## 配置说明

- 选择使用的端口类型，默认使用 UART

	```
    Select the underlying transport (use UART)  ---> 
        (x) use UART
        ( ) use UDP
        ( ) use TCP
	```

- UART

  ```
  (uart2) the name of base actual device
  ```
  
  选择使用 UART 需要配置使用的串口设备名称，此名称是串口设备注册到 rtdevice 框架的名称，需要按照实际使用的串口号更改。配置类型是 string，默认配置是 uart2。

- UDP

	```
    (8080) local udp port  //配置本地 UDP 端口号
    (8080) remote udp port //配置远端 UDP 端口号
    (192.168.12.109) the other side IP address for rtlink	//配置对端 IP
  ```
  
- TCP

	```
    (8080) local tcp port  //配置本地 TCP 端口号
    (8080) remote tcp port //配置远端 TCP 端口号
    (192.168.12.109) the other side IP address for rtlink	//配置对端 IP
    [*]   config rtlink in server mode	//是否为 TCP-server，选中则是 TCP server
  ```
	
	选择使用 TCP 需要注意设备是 TCP 的 server 端还是 client 端，选中`config rtlink in server mode` 选项代表此设备是 TCP server。
