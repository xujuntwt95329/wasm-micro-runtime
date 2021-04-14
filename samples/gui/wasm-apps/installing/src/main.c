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
lv_obj_t *hello_world_label;
lv_obj_t *count_label;
lv_obj_t *btn1;
lv_obj_t *label_count1;
int label_count1_value = 1;
char label_count1_str[11] = { 0 };

void on_init()
{
    char *text;

    // hello_world_label = lv_label_create(NULL, NULL);
    // lv_label_set_text(hello_world_label, "正在安装 ……");
    // text = lv_label_get_text(hello_world_label);
    // printf("Label text %lu %s \n", strlen(text), text);
    // lv_obj_align(hello_world_label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    // count_label = lv_label_create(NULL, NULL);
    // lv_obj_align(count_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

    label_count1 = lv_label_create(NULL, NULL);
    lv_label_set_text(label_count1, "正在安装 ……");
    lv_obj_align(label_count1, NULL, LV_ALIGN_CENTER, 0, 0);
}
