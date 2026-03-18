/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int start_btn_pin = 11;
const int btn_pins[] = {12, 13, 14, 15};
const int led_pins[] = {16, 17, 18, 19};
// Azul, Vermelho, Verde, Amarelo

volatile int start_f = 0;
volatile int f_btn_y = 0;
volatile int f_btn_w = 0;
volatile int f_btn_p = 0;
volatile int f_btn_b = 0;
volatile int pressed = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        switch (gpio) {
        case 12:
            f_btn_y = 1;
            break;
        case 13:
            f_btn_w = 1;
            break;
        case 14:
            f_btn_p = 1;
            break;
        case 15:
            f_btn_b = 1;
            break;
        case 11:
            start_f = 1;
            break;
        default:
            break;
        }

        pressed = 1;
    }
}

int main() {
    stdio_init_all();

    int sequence[5] = {19, 17, 17, 18, 16};
    int level = 0;

    gpio_init(start_btn_pin);
    gpio_set_dir(start_btn_pin, GPIO_IN);
    gpio_pull_up(start_btn_pin);

    gpio_set_irq_enabled_with_callback(start_btn_pin, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    for (int i = 0; i < 4; i++) {
        gpio_init(btn_pins[i]);
        gpio_set_dir(btn_pins[i], GPIO_IN);
        gpio_pull_up(btn_pins[i]);

        gpio_set_irq_enabled(btn_pins[i], GPIO_IRQ_EDGE_FALL, true);
    }

    for (int i = 0; i < 4; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
    }

    while (true) {

        if (start_f) {
            int acertos = 0;

            if (level < 5) {
                for (int i = 0; i <= level; i++) {
                    gpio_put(sequence[i], 1);
                    sleep_ms(250);
                    gpio_put(sequence[i], 0);
                }

                for (int i = 0; i <= level; i++) {
                    while (!pressed) {
                        sleep_ms(1);
                    }

                    if (sequence[i] == 16 & f_btn_y ||
                        sequence[i] == 17 & f_btn_w ||
                        sequence[i] == 18 & f_btn_p ||
                        sequence[i] == 19 & f_btn_b) {
                        if (acertos == level + 1) {
                            acertos = 0;
                            level++;
                        } else {
                            acertos++;
                        }

                        f_btn_y = 0;
                        f_btn_w = 0;
                        f_btn_p = 0;
                        f_btn_b = 0;
                    } else {
                        level = 0;
                        printf("perdeu otario do krl\n");
                        start_f = 0;
                    }
                }
            } else {
                printf("Ganhou");
                level = 0;
                start_f = 0;
            }
        }
    }
}
