/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-7-7     songchao   the first version
 */

#include <rtthread.h>
#include <rtlink_port.h>
#include <string.h>

#if !defined(SAL_USING_POSIX)
#error "Please enable SAL_USING_POSIX!"
#else
#include <sys/time.h>
#include <sys/select.h>
#endif
#include <sys/socket.h> /* To use the BSD socket, you need to include the socket.h header file */
#include "netdb.h"
#define DBG_TAG              "rtlink_port"
#define DBG_LVL              DBG_INFO
#include <rtdbg.h>

#define BUFSZ   4096
static struct sockaddr_in local_addr;
static struct sockaddr_in remote_addr;
static uint8_t socket_buffer[BUFSZ];
static int32_t rtlink_socket = -1;

static void rtlink_socket_thread_entry(void *param)
{
    fd_set readset;
    uint32_t bytes_received;
    struct timeval timeout;
    socklen_t addr_len;
    struct sockaddr_in socket_addr;
    addr_len = sizeof(struct sockaddr);
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    while(1)
    {
        if(rtlink_socket == -1)
        {
            rt_thread_delay(100);
        }
        else
        {
            FD_ZERO(&readset);
            FD_SET(rtlink_socket, &readset);
            if (select(rtlink_socket + 1, &readset, RT_NULL, RT_NULL, &timeout) == 0)
            {
                continue;
            }
            else
            {
                bytes_received = recvfrom(rtlink_socket, socket_buffer, BUFSZ - 1, 0,
                              (struct sockaddr *)&socket_addr, &addr_len);
                if (bytes_received < 0)
                {
                    LOG_E("Received error, close the socket.");
                    closesocket(rtlink_socket);
                    rtlink_socket = -1;
                }
                else if (bytes_received == 0)
                {
                    LOG_W("Received warning, recv function return 0.");
                }
                else
                {
                    rt_link_hw_write_cb(&socket_buffer, bytes_received);
                }
            }
        }
    }
}

rt_err_t rt_link_port_reconnect(void)
{
    return RT_EOK;
}

rt_size_t rt_link_port_send(void *data, rt_size_t length)
{
    rt_size_t size = 0;
    ssize_t ret;
    static rt_size_t udp_send_count = 0;
    if(rtlink_socket != -1)
    {
        ret = sendto(rtlink_socket, data, length, 0,(struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
        if (ret < 0)
        {
            LOG_I("send error, close the socket.");
            closesocket(rtlink_socket);
            rtlink_socket = -1;
        }
        else if (ret == 0)
        {
            LOG_W("Send warning, send function return 0.");
        }
        else
        {
            udp_send_count++;
        }
        rt_thread_delay(1);
    }

    return size;
}

rt_err_t rt_link_port_init(void)
{
    if ((rtlink_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        LOG_E("Create socket error");
        return 0;
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(RTLINK_LOCAL_UDP_PORT);
    rt_memset(&(local_addr.sin_zero), 0, sizeof(local_addr.sin_zero));

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(RTLINK_REMOTE_UDP_PORT);
    remote_addr.sin_addr.s_addr = inet_addr(RTLINK_REMOTE_HOST_IP);
    rt_memset(&(remote_addr.sin_zero), 0, sizeof(remote_addr.sin_zero));

    if (bind(rtlink_socket, (struct sockaddr *)&local_addr,sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Unable to bind");
        closesocket(rtlink_socket);
        rtlink_socket = -1;
        return 0;
    }
    LOG_D("bind to port %d ip_addr %s success\n",RTLINK_LOCAL_UDP_PORT,RTLINK_REMOTE_HOST_IP);

    rt_thread_t rtlink_socket_tid;
    rtlink_socket_tid = rt_thread_create("rt_link_socket_thread",
                            rtlink_socket_thread_entry,
                            RT_NULL,
                            4096,
                            RT_THREAD_PRIORITY_MAX - 2,
                            2);
    if (rtlink_socket_tid != RT_NULL)
    {
        rt_thread_startup(rtlink_socket_tid);
    }

    return RT_EOK;
}

rt_err_t rt_link_port_deinit(void)
{
    closesocket(rtlink_socket);
    rtlink_socket = -1;
    return RT_EOK;
}