/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "bh_platform.h"
#include "runtime_lib.h"
#include "native_interface.h"
#include "app_manager_export.h"
#include "board_config.h"
#include "bh_common.h"
#include "bh_queue.h"
#include "runtime_sensor.h"
#include "bi-inc/attr_container.h"
#include "module_wasm_app.h"
#include "wasm_export.h"
#include "display.h"
#include "lvgl.h"

extern void init_sensor_framework();
extern void exit_sensor_framework();
extern int aee_host_msg_callback(void *msg, uint32_t msg_len);
extern bool touchscreen_read(lv_indev_data_t * data);
extern int ili9340_init();
extern void xpt2046_init(void);
extern void wgl_init();

#include <zephyr.h>
#include <drivers/uart.h>
#include <device.h>
#include <drivers/gpio.h>

/*
 * Devicetree helper macro which gets the 'flags' cell from a 'gpios'
 * property, or returns 0 if the property has no 'flags' cell.
 */

#define FLAGS_OR_ZERO(node)						\
	COND_CODE_1(DT_PHA_HAS_CELL(node, gpios, flags),		\
		    (DT_GPIO_FLAGS(node, gpios)),			\
		    (0))

/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */

#define SW0_NODE	DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | FLAGS_OR_ZERO(SW0_NODE))
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL	""
#define SW0_GPIO_PIN	0
#define SW0_GPIO_FLAGS	0
#endif

static struct gpio_callback button_cb_data;

/* CoAP request method codes */
typedef enum {
    COAP_GET = 1,
    COAP_POST,
    COAP_PUT,
    COAP_DELETE,
    COAP_EVENT = (COAP_DELETE + 2)
} coap_method_t;

void button_pressed(struct device *dev, struct gpio_callback *cb,
		    u32_t pins)
{
	request_t request[1];
    init_request(request, (char *)"buy", COAP_EVENT, FMT_APP_RAW_BINARY, "", 0);
    am_publish_event(request);
}

struct device *button;

int uart_char_cnt = 0;

static void uart_irq_callback(struct device *dev)
{
    unsigned char ch;

    while (uart_poll_in(dev, &ch) == 0) {
        uart_char_cnt++;
        aee_host_msg_callback(&ch, 1);
    }
}

struct device *uart_dev = NULL;

static bool host_init()
{
    uart_dev = device_get_binding(HOST_DEVICE_COMM_UART_NAME);
    if (!uart_dev) {
        printf("UART: Device driver not found.\n");
        return false;
    }
    uart_irq_rx_enable(uart_dev);
    uart_irq_callback_set(uart_dev, uart_irq_callback);
    return true;
}

int host_send(void * ctx, const char *buf, int size)
{
    if (!uart_dev)
        return 0;

    for (int i = 0; i < size; i++)
        uart_poll_out(uart_dev, buf[i]);

    return size;
}

void host_destroy()
{
}

host_interface interface = {
    .init = host_init,
    .send = host_send,
    .destroy = host_destroy
};

timer_ctx_t timer_ctx;

static char global_heap_buf[270 * 1024] = { 0 };

static uint8_t color_copy[320 * 10 * 3];

static void display_flush(lv_disp_drv_t *disp_drv,
                          const lv_area_t *area,
                          lv_color_t *color)
{
    u16_t w = area->x2 - area->x1 + 1;
    u16_t h = area->y2 - area->y1 + 1;
    struct display_buffer_descriptor desc;
    int i;
    uint8_t *color_p = color_copy;

    desc.buf_size = 3 * w * h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;

    for (i = 0; i < w * h; i++, color++) {
        color_p[i * 3] = color->ch.red;
        color_p[i * 3 + 1] = color->ch.green;
        color_p[i * 3 + 2] = color->ch.blue;
    }

    display_write(NULL, area->x1, area->y1, &desc, (void *) color_p);

    lv_disp_flush_ready(disp_drv); /* in v5.3 is lv_flush_ready */
}

static bool display_input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    return touchscreen_read(data);
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    /* init button */
    int ret;
    button = device_get_binding(SW0_GPIO_LABEL);
	if (button == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}

    ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	ret = gpio_pin_interrupt_configure(button,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button, &button_cb_data);
    /* finish init button */

    xpt2046_init();
    ili9340_init();
    display_blanking_off(NULL);

    /*Create a display buffer*/
    static lv_disp_buf_t disp_buf1;
    static lv_color_t buf1_1[320*10];
    lv_disp_buf_init(&disp_buf1, buf1_1, NULL, 320*10);

    /*Create a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.buffer = &disp_buf1;
    disp_drv.flush_cb = display_flush;
    //    disp_drv.hor_res = 200;
    //    disp_drv.ver_res = 100;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = display_input_read;
    lv_indev_drv_register(&indev_drv);
}

int iwasm_main()
{
    RuntimeInitArgs init_args;
    host_init();

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    wgl_init();
    hal_init();

    // timer manager
    init_wasm_timer();

    // TODO:
    app_manager_startup(&interface);

    wasm_runtime_destroy();
    return -1;
}
