/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include "wasm_app.h"
#include "wa-inc/lvgl/lvgl.h"
#include "wa-inc/timer_wasm_app.h"

extern char g_widget_text[];

static void btn_event_cb(lv_obj_t *btn, lv_event_t event);

lv_obj_t *model_label;
lv_obj_t *cond_label;
lv_obj_t *addr_label;

user_timer_t buy_timer;

void timer2_update(user_timer_t timer1)
{
    lv_label_set_text(model_label, "洗衣液型号: 111B");
    lv_label_set_text(cond_label, "激活条件: 每月1日");
    lv_label_set_text(addr_label, "配送地址: 北京市");
}

void buy_handler(request_t *request)
{
    lv_label_set_text(model_label, "");
    lv_label_set_text(cond_label, "");
    lv_label_set_text(addr_label, "");

    lv_label_set_text(cond_label, "洗衣液购买成功");

    
    if (buy_timer)
        api_timer_restart(buy_timer, 5000);
    else
        printf("Fail to create timer.\n");
}

void on_init()
{
    lv_obj_t *dev_label = lv_label_create(NULL, NULL);
    lv_label_set_text(dev_label, "洗衣机 2");
    lv_obj_align(dev_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 5);

    lv_obj_t *seg_label = lv_label_create(NULL, NULL);
    lv_label_set_text(seg_label, "_____________________________________________");
    lv_obj_align(seg_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 16);

    /*Create a label on the button*/
    lv_obj_t *name_label = lv_label_create(NULL, NULL);
    lv_label_set_text(name_label, "奥妙洗衣液机器合约");
    lv_obj_align(name_label, NULL, LV_ALIGN_CENTER, 0, -40);

    model_label = lv_label_create(NULL, NULL);
    lv_label_set_text(model_label, "洗衣液型号: 111B");
    lv_obj_align(model_label, NULL, LV_ALIGN_CENTER, 0, 0);

    cond_label = lv_label_create(NULL, NULL);
    lv_label_set_text(cond_label, "激活条件: 每月1日");
    lv_obj_align(cond_label, NULL, LV_ALIGN_CENTER, 0, 20);

    addr_label = lv_label_create(NULL, NULL);
    lv_label_set_text(addr_label, "配送地址: 北京市");
    lv_obj_align(addr_label, NULL, LV_ALIGN_CENTER, 0, 40);

    buy_timer = api_timer_create(5000, false, false, timer2_update);
    api_subscribe_event("buy", buy_handler);

    /* set up a timer */
    // user_timer_t timer;
    // timer = api_timer_create(10000, false, false, timer1_update);
    // if (timer)
    //     api_timer_restart(timer, 10000);
    // else
    //     printf("Fail to create timer.\n");
}

// static void btn_event_cb(lv_obj_t *btn, lv_event_t event)
// {
//     if(event == LV_EVENT_RELEASED) {
//         label_count1_value++;
//         snprintf(label_count1_str, sizeof(label_count1_str),
//                  "%d", label_count1_value);
//         lv_label_set_text(label_count1, label_count1_str);
//         if (label_count1_value == 100)
//             label_count1_value = 0;
//     }

// }
