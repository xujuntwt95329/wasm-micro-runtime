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

uint32_t count = 0;
char count_str[11] = { 0 };
int label_count1_value = 1;
char label_count1_str[11] = { 0 };

void on_init()
{
lv_obj_t *dev_label = lv_label_create(NULL, NULL);
    lv_label_set_text(dev_label, "洗衣机");
    lv_obj_align(dev_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 5);

    lv_obj_t *seg_label = lv_label_create(NULL, NULL);
    lv_label_set_text(seg_label, "_____________________________________________");
    lv_obj_align(seg_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 16);

    /*Create a label on the button*/
    lv_obj_t *name_label = lv_label_create(NULL, NULL);
    lv_label_set_text(name_label, "建设银行设备子钱包");
    lv_obj_align(name_label, NULL, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *prompt_label = lv_label_create(NULL, NULL);
    lv_label_set_text(prompt_label, "正在安装 ……");
    lv_obj_align(prompt_label, NULL, LV_ALIGN_CENTER, 0, 0);
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
