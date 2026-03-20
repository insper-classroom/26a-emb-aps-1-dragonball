/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int start_btn_pin = 11;
const int btn_pins[] = {15, 14, 13, 12};
const int led_pins[] = {16, 17, 18, 19};

volatile int start_f = 0;
volatile int pressed_btn = -1;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == start_btn_pin) {
        start_f = 1;
        return;
    }

    for (int i = 0; i < 4; i++) {
        if ((int)gpio == btn_pins[i]) {
            pressed_btn = i;
            return;
        }
    }
}

int main() {
    stdio_init_all();

    // Sequencia dinamica com elementos aleatorios (ate 10 niveis).
    int sequence[10] = {0};
    int level = 0;

    // Inicializa gerador de numeros aleatorios.
    srand(time_us_32());

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

    for (int i = 0; i < 4; i++) {
        gpio_put(led_pins[i], 1);
        sleep_ms(120);
        gpio_put(led_pins[i], 0);
    }

    while (true) {

        if (start_f) {
            level = 1;

            // Gera primeiro elemento aleatorio.
            sequence[0] = rand() % 4;

            while (level) {
                int acertos = 0;
                printf("nivel %d\n", level);

                if (level < 11) {
                    for (int i = 0; i < level; i++) {
                        for (int j = 0; j < 4; j++) {
                            gpio_put(led_pins[j], 0);
                        }

                        gpio_put(led_pins[sequence[i]], 1);
                        sleep_ms(500);
                        gpio_put(led_pins[sequence[i]], 0);
                        sleep_ms(120);
                    }

                    for (int i = 0; i < level; i++) {
                        pressed_btn = -1;

                        while (pressed_btn < 0) {
                            sleep_ms(1);
                        }

                        if (sequence[i] == pressed_btn) {
                            acertos++;

                            gpio_put(led_pins[pressed_btn], 1);
                            sleep_ms(250);
                            gpio_put(led_pins[pressed_btn], 0);

                            if (acertos == level) {
                                acertos = 0;
                                if (level < 10) {
                                    // Gera novo elemento aleatorio para proximo nivel.
                                    sequence[level] = rand() % 4;
                                }
                                level++;
                                break;
                            }

                            sleep_ms(250);
                        } else {
                            level = 0;

                            for (int j = 0; j < 4; j++) {
                                gpio_put(led_pins[j], 1);
                            }

                            sleep_ms(500);

                            for (int j = 0; j < 4; j++) {
                                gpio_put(led_pins[j], 0);
                            }

                            printf("perdeu otario do krl\n");
                            start_f = 0;
                            break;
                        }
                    }
                } else {
                    printf("Ganhou");
                    level = 0;
                    start_f = 0;

                    for (int i = 0; i < 4; i++) {
                        gpio_put(led_pins[i], 1);
                        sleep_ms(200);
                    }

                    for (int i = 0; i < 4; i++) {
                        gpio_put(led_pins[i], 0);
                        sleep_ms(200);
                    }

                    for (int i = 0; i < 4; i++) {
                        gpio_put(led_pins[i], 1);
                        sleep_ms(200);
                    }

                    for (int i = 0; i < 4; i++) {
                        gpio_put(led_pins[i], 0);
                    }
                }
            }
        }
    }
}
