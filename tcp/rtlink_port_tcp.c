/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-7-13     songchao   the first version
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
#define DBG_LVL              DBG_LOG
#include <rtdbg.h>

#define BUFSZ   4096
static struct sockaddr_in remote_addr;
static uint8_t socket_buffer[BUFSZ];
static int32_t rtlink_socket = -1;
static int32_t connect_socket = -1;
static int32_t rtlink_tcp_delete_flag = 0;

#ifdef RTLINK_IN_SERVER_MODE
static struct sockaddr_in local_addr;
#endif

static void rtlink_socket_thread_entry(void *param)
{
    fd_set readset;
    uint32_t bytes_received;
    struct timeval timeout;
    struct sockaddr_in;
    uint32_t recv_count = 0;
#ifdef RTLINK_IN_SERVER_MODE    
    fd_set connectset;
    socklen_t addr_len;
    addr_len = sizeof(struct sockaddr);
#endif    
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    while(1)
    {
        if(rtlink_socket < 0)
        {
            rt_thread_delay(100);
            if(rtlink_tcp_delete_flag == 1)
            {
                LOG_D("rtlink tcp thread exit\n");
                return;
            }
        }
        else
        {
#ifdef RTLINK_IN_SERVER_MODE
            FD_ZERO(&connectset);
            FD_SET(rtlink_socket, &connectset);
            if (select(rtlink_socket + 1, &connectset, RT_NULL, RT_NULL, &timeout) == 0)
            {
                continue;
            }
            connect_socket = accept(rtlink_socket, (struct sockaddr *)&remote_addr, &addr_len);
            if (connect_socket < 0)
            {
                LOG_E("accept connection failed! errno = %d", errno);
                continue;
            }
            LOG_D("accept success\n");
#else
            if(connect_socket < 0)
            {
                if (connect(rtlink_socket, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
                {
                    LOG_D("Connect to IP %s port %d failed!",RTLINK_REMOTE_HOST_IP,RTLINK_REMOTE_TCP_PORT);
                    rt_thread_delay(1000);
                    continue;
                }
                else
                {
                    connect_socket = rtlink_socket;
                    LOG_D("Connect to IP %s port %d success",RTLINK_REMOTE_HOST_IP,RTLINK_REMOTE_TCP_PORT);
                }
            }
#endif
            while(connect_socket >= 0)
            {
                FD_ZERO(&readset);
                FD_SET(connect_socket, &readset);
                if (select(connect_socket + 1, &readset, RT_NULL, RT_NULL, &timeout) == 0)
                {
                    continue;
                }
                else
                {
                    if(connect_socket >= 0)
                    {
                        bytes_received = recv(connect_socket, socket_buffer, BUFSZ - 1, 0);
                        if (bytes_received < 0)
                        {
                            LOG_E("Received error, close the socket.");
                            closesocket(connect_socket);
                            connect_socket = -1;
                        }
                        else if (bytes_received == 0)
                        {
                            LOG_W("Received warning, recv function return 0.");
                        }
                        else
                        {
                            recv_count++;
                            rt_link_hw_write_cb(&socket_buffer, bytes_received);
                        }
                    }
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
    if(connect_socket >= 0)
    {
        ret = send(connect_socket, data, length, 0);
        if (ret < 0)
        {
            LOG_E("send error, close the socket.");
            closesocket(connect_socket);
            connect_socket = -1;
        }
        else if (ret == 0)
        {
            LOG_W("Send warning, send function return 0.");
        }
    }

    return size;
}

rt_err_t rt_link_port_init(void)
{
    if ((rtlink_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        LOG_E("Create socket error");
        goto __exit;
    }
#ifdef RTLINK_IN_SERVER_MODE
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(RTLINK_LOCAL_TCP_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(local_addr.sin_zero), 0, sizeof(local_addr.sin_zero));

    if (bind(rtlink_socket, (struct sockaddr *)&local_addr,sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Unable to bind");
        goto __exit;
    }
    LOG_D("bind to port %d ip_addr %s success\n",RTLINK_LOCAL_TCP_PORT,RTLINK_REMOTE_HOST_IP);
    if (listen(rtlink_socket, 10) == -1)
    {
        LOG_E("Listen error");
        goto __exit;
    }
#else
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(RTLINK_REMOTE_TCP_PORT);
        remote_addr.sin_addr.s_addr = inet_addr(RTLINK_REMOTE_HOST_IP);
        rt_memset(&(remote_addr.sin_zero), 0, sizeof(remote_addr.sin_zero));

#endif
    rtlink_tcp_delete_flag = 0;
    rt_thread_t rtlink_socket_tid;
    rtlink_socket_tid = rt_thread_create("rl_sock",
                            rtlink_socket_thread_entry,
                            RT_NULL,
                            4096,
                            RT_THREAD_PRIORITY_MAX - 2,
                            2);
    if (rtlink_socket_tid != RT_NULL)
    {
        rt_thread_startup(rtlink_socket_tid);
    }
    else
    {
        goto __exit;
    }
    return RT_EOK;

__exit:
    if(rtlink_socket >= 0)
    {
        closesocket(rtlink_socket);
        rtlink_socket = -1;
    }
    return RT_ERROR;

}

rt_err_t rt_link_port_deinit(void)
{
    rtlink_tcp_delete_flag = 1;
#ifdef RTLINK_IN_SERVER_MODE
    if(rtlink_socket >= 0)
    {
        closesocket(rtlink_socket);
        rtlink_socket = -1;
    }
    if(connect_socket >= 0)
    {
        closesocket(connect_socket);
        connect_socket = -1;
    }
#else
    if(rtlink_socket >= 0)
    {
        closesocket(rtlink_socket);
        rtlink_socket = -1;
        connect_socket = -1;
    }
#endif

    return RT_EOK;
}