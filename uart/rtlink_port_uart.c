/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-09     xiangxistu   the first version
 */

#define DBG_TAG              "rtlink_hw"
#define DBG_LVL              DBG_INFO
#include <rtdbg.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <rtlink_port.h>

#ifdef PKG_PKG_RT_LINK_HW_DEVICE_NAME
    #define RT_LINK_HW_DEVICE_NAME PKG_PKG_RT_LINK_HW_DEVICE_NAME
#else
    #error "PKG_PKG_RT_LINK_HW_DEVICE_NAME must be defined."
#endif

#ifdef RT_SERIAL_RB_BUFSZ
    #define RT_LINK_HW_RB_BUFSZ     (RT_SERIAL_RB_BUFSZ)
#else
    #define RT_LINK_HW_RB_BUFSZ 64
#endif

static struct rt_device *hw_device = RT_NULL;
static rt_uint8_t buffer[RT_LINK_HW_RB_BUFSZ] = {0};
rt_err_t rt_link_port_rx_ind(rt_device_t device, rt_size_t size)
{
    RT_ASSERT(device != RT_NULL);
    rt_size_t length = rt_device_read(device, 0, buffer, sizeof(buffer));
    rt_link_hw_write_cb(&buffer, length);
    return RT_EOK;
}

rt_err_t rt_link_port_reconnect(void)
{
    return RT_EOK;
}

rt_size_t rt_link_port_send(void *data, rt_size_t length)
{
    rt_size_t size = 0;
    size = rt_device_write(hw_device, 0, data, length);
    return size;
}

rt_err_t rt_link_port_init(void)
{
    hw_device = rt_device_find(RT_LINK_HW_DEVICE_NAME);
    if (hw_device)
    {
        rt_device_open(hw_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        rt_device_set_rx_indicate(hw_device, rt_link_port_rx_ind);
    }
    else
    {
        LOG_E("Not find device %s", RT_LINK_HW_DEVICE_NAME);
        return -RT_ERROR;
    }
    return RT_EOK;
}

rt_err_t rt_link_port_deinit(void)
{
    hw_device = rt_device_find(RT_LINK_HW_DEVICE_NAME);
    if (hw_device)
    {
        rt_device_close(hw_device);
        rt_device_set_rx_indicate(hw_device, RT_NULL);
    }
    else
    {
        LOG_E("Not find device %s", RT_LINK_HW_DEVICE_NAME);
        return -RT_ERROR;
    }
    return RT_EOK;
}
