/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "tft_lcd_ili9341/gfx/gfx_ili9341.h"
#include "tft_lcd_ili9341/ili9341/ili9341.h"
#include "tft_lcd_ili9341/touch_resistive/touch_resistive.h"

#include "image_bitmap.h"

// Propriedades do LCD
#define SCREEN_ROTATION 1
const int width = 320;
const int height = 240;

const int img_height = 120;
const int img_width = 220;
const int img_x = (width - img_width) / 2;
const int img_y = (height - img_height) / 2;

const int start_btn_pin = 11;
// verde, azul, vermelho, amarelo
const int btn_pins[] = {2, 3, 4, 5};
const int led_pins[] = {6, 7, 8, 9};

volatile int start_f = 0;
volatile int pressed_btn = -1;
volatile int timer_f = 0;

void draw_state(int state, int pontuacao, int level) {
    if (state == 0) {
        gfx_fillRect(0, 0, width, height, 0x0000);
        gfx_drawBitmap(img_x, img_y, logo_bitmap, img_width, img_height, 0xFFFF);
    } else {
        gfx_fillRect(0, 0, width, height, 0x0000);
        gfx_setTextSize(2);

        if (state == 1) {
            gfx_setTextColor(0x07E0);
            gfx_drawText(
                width / 6,
                10,
                "Voce venceu!");
        } else {
            gfx_setTextColor(0xF800);
            gfx_drawText(
                width / 6,
                10,
                "Voce perdeu!");
        }

        char buffer[30];
        sprintf(buffer, "Pontuacao: %d", pontuacao);
        gfx_setTextColor(0xFFFF);
        gfx_drawText(
            width / 6,
            40,
            buffer);
        sprintf(buffer, "Nivel: %d", level);
        gfx_drawText(
            width / 6,
            70,
            buffer);
    }
}

bool timer_0_callback(repeating_timer_t *rt) {
    timer_f = 1;
    return true; // keep repeating
}

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
}int main() {
    stdio_init_all();

    int sequence[10] = {0};
    int max_level = sizeof(sequence);
    int level = 0;
    int state = 0;

    srand(time_us_32());

    LCD_initDisplay();
    LCD_setRotation(SCREEN_ROTATION);

    gfx_init();
    gfx_clear();

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

    draw_state(state, 0, 0);

    while (true) {

        if (start_f) {

            draw_state(state, 0, 0);

            for (int i = 0; i < 4; i++) {
                gpio_put(led_pins[i], 1);
                sleep_ms(120);
                gpio_put(led_pins[i], 0);
            }

            sleep_ms(500);

            int pontuacao = 0;

            level = 1;

            // Gera primeiro elemento aleatorio.
            sequence[0] = rand() % 4;

            while (level) {
                int acertos = 0;
                printf("nivel %d\n", level);

                if (level <= max_level) {
                    int pontos = level * 100;

                    repeating_timer_t timer_pontuacao;
                    timer_f = 0;

                    if (!add_repeating_timer_ms(500,
                                                timer_0_callback,
                                                NULL,
                                                &timer_pontuacao)) {
                        printf("Failed to add timer\n");
                    }

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
                            if (timer_f) {
                                timer_f = 0;
                                if (pontos >= 10) {
                                    pontos -= 10;
                                } else {
                                    pontos = 0;
                                }
                            }

                            sleep_ms(10);
                        }

                        if (sequence[i] == pressed_btn) {
                            acertos++;

                            gpio_put(led_pins[pressed_btn], 1);
                            sleep_ms(250);
                            gpio_put(led_pins[pressed_btn], 0);

                            if (acertos == level) {
                                acertos = 0;
                                pontuacao += pontos;
                                printf("pontos rodada: %d | pontuacao total: %d\n", pontos, pontuacao);

                                if (level < max_level) {
                                    // Gera novo elemento aleatorio para proximo nivel.
                                    sequence[level] = rand() % 4;
                                }

                                level++;
                                break;
                            }

                            sleep_ms(250);
                        } else {
                            for (int j = 0; j < 4; j++) {
                                gpio_put(led_pins[j], 1);
                            }

                            sleep_ms(500);

                            for (int j = 0; j < 4; j++) {
                                gpio_put(led_pins[j], 0);
                            }

                            printf("perdeu otario do krl\n");
                            draw_state(2, pontuacao, level);
                            level = 0;
                            start_f = 0;
                            break;
                        }
                    }

                    sleep_ms(500);
                    cancel_repeating_timer(&timer_pontuacao);
                } else {
                    printf("Ganhou");
                    draw_state(1, pontuacao, max_level);

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
