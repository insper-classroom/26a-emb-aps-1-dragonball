/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"

#include "tft_lcd_ili9341/gfx/gfx_ili9341.h"
#include "tft_lcd_ili9341/ili9341/ili9341.h"
#include "tft_lcd_ili9341/touch_resistive/touch_resistive.h"

#include "image_bitmap.h"
#include "amarelo.h"
#include "verde.h"
#include "vermelho.h"
#include "azul.h"
#include "errou.h"
#include "vitoria.h"
#include "acertou.h"

// Propriedades do LCD
#define SCREEN_ROTATION 1
#define ANIM_FRAME_COUNT 8

const int AUDIO_OUT = 14;

int wav_position = 0;
int tamanho_audio;
int *p_tamanho_audio = &tamanho_audio;
uint8_t *p_audio;

const int width = 320;
const int height = 240;

const int img_height = 120;
const int img_width = 220;
const int banana_height = 100;
const int banana_width = 95;
const int img_x = (width - img_width) / 2;
const int img_y = (height - img_height) / 2;
const int banana_x = (width - banana_width) / 2;
const int banana_y = (height - banana_height) / 2;

const int start_btn_pin = 11;
// verde, azul, vermelho, amarelo
const int btn_pins[] = {2, 3, 4, 5};
const int led_pins[] = {6, 7, 8, 9};

volatile int start_f = 0;
volatile int pressed_btn = -1;
volatile int timer_f = 0;
volatile int g_timer_0 = 0;
volatile bool failed = 0;

const uint8_t *banana_frames[] = {banana1, banana2, banana3, banana4, banana5, banana6, banana7, banana8};

void pwm_interruption_handler() {
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_OUT));
    if (wav_position < (*p_tamanho_audio << 3) - 1) {
        pwm_set_gpio_level(AUDIO_OUT, p_audio[wav_position >> 3]);
        wav_position++;
    }
}

void draw_state(int state, int anim_frame, int pontuacao, int level) {
    if (state == 0) {
        gfx_fillRect(0, 0, width, height, 0x0000);
        gfx_drawBitmap(img_x, img_y, logo_bitmap, img_width, img_height, 0xFFFF);
    } else if (state == 1) {
        gfx_fillRect(0, 0, width, height, 0x0000);
        gfx_drawBitmap(banana_x, banana_y, banana_frames[anim_frame], banana_width, banana_height, 0xFFFF);
    } else {
        gfx_fillRect(0, 0, width, height, 0x0000);
        gfx_setTextSize(2);

        if (state == 3) {
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

bool timer_1_callback(repeating_timer_t *rt) {
    g_timer_0 = 1;
    return true; // keep repeating
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    failed = true;

    // Can return a value here in us to fire in the future
    return 0;
}

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == start_btn_pin) {
        start_f = 1;
        return;
    }

    if ((int)gpio == btn_pins[0]) {
        pressed_btn = 0;
    } else if ((int)gpio == btn_pins[1]) {
        pressed_btn = 1;
    } else if ((int)gpio == btn_pins[2]) {
        pressed_btn = 2;
    } else if ((int)gpio == btn_pins[3]) {
        pressed_btn = 3;
    }
}

void core1_entry() {
    int state = multicore_fifo_pop_blocking();
    int anim_frame = 0;
    repeating_timer_t timer_0;

    if (!add_repeating_timer_ms(100,
                                timer_1_callback,
                                NULL,
                                &timer_0)) {
        printf("Failed to add timer\n");
    }

    // Draw the first state received at startup (e.g. logo screen).
    if (state == 0) {
        g_timer_0 = 0;
        draw_state(state, 0, 0, 0);
    } else if (state == 1) {
        draw_state(state, anim_frame, 0, 0);
    }

    while (true) {
        tight_loop_contents();

        if (multicore_fifo_rvalid()) {
            state = multicore_fifo_pop_blocking();
            anim_frame = 0;
            if (state == 0) {
                g_timer_0 = 0;
                draw_state(state, 0, 0, 0);
            } else if (state == 1) {
                draw_state(state, anim_frame, 0, 0);
            } else {
                int level = multicore_fifo_pop_blocking();
                int pontuacao = multicore_fifo_pop_blocking();
                draw_state(state, 0, pontuacao, level);
            }
        }

        if (g_timer_0) {
            g_timer_0 = 0;
            anim_frame = (anim_frame + 1) % ANIM_FRAME_COUNT;

            if (state == 1) {
                draw_state(state, anim_frame, 0, 0);
            }
        }
    }
}

int main() {
    stdio_init_all();
    set_sys_clock_khz(176000, true);
    gpio_set_function(AUDIO_OUT, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_OUT);
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);

    irq_set_enabled(PWM_IRQ_WRAP, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interruption_handler);

    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv(&config, 8.0f);
    pwm_config_set_wrap(&config, 250);

    int sequence[10] = {0};
    int max_level = sizeof(sequence) / sizeof(sequence[0]);

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

    multicore_launch_core1(core1_entry);
    multicore_fifo_push_blocking(0);

    pwm_init(audio_pin_slice, &config, true);

    while (true) {

        if (start_f) {

            multicore_fifo_push_blocking(1);

            for (int i = 0; i < 4; i++) {
                gpio_put(led_pins[i], 1);
                sleep_ms(120);
                gpio_put(led_pins[i], 0);
            }

            sleep_ms(500);

            int pontuacao = 0;
            int level = 1;

            // Gera primeiro elemento aleatorio.
            sequence[0] = rand() % 4;

            while (level) {
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

                        // Escolhe o som aleatoriamente
                        int som = rand() % 4;

                        gpio_put(led_pins[sequence[i]], 1);
                        switch (som) {
                        case (0):
                            tamanho_audio = VERDE_LENGTH;
                            p_audio = VERDE_DATA;
                            break;
                        case (1):
                            tamanho_audio = AZUL_LENGTH;
                            p_audio = AZUL_DATA;
                            break;
                        case (2):
                            tamanho_audio = VERMELHO_LENGTH;
                            p_audio = VERMELHO_DATA;
                            break;
                        case (3):
                            tamanho_audio = AMARELO_LENGTH;
                            p_audio = AMARELO_DATA;
                            break;
                        default:
                            break;
                        }
                        wav_position = 0;
                        sleep_ms(1300);
                        gpio_put(led_pins[sequence[i]], 0);
                        sleep_ms(120);
                    }

                    alarm_id_t alarm = add_alarm_in_ms(5000, alarm_callback, NULL, false);
                    failed = 0;

                    if (!alarm) {
                        printf("Failed to add timer\n");
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

                            if (failed) {
                                break;
                            }

                            sleep_ms(10);
                        }

                        if (sequence[i] == pressed_btn) {

                            cancel_alarm(alarm);

                            gpio_put(led_pins[pressed_btn], 1);
                            wav_position = 0;
                            tamanho_audio = ACERTOU_LENGTH;
                            p_audio = ACERTOU_DATA;
                            sleep_ms(250);
                            gpio_put(led_pins[pressed_btn], 0);

                            if (i == level - 1) {
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

                            p_audio = ERROU_DATA;
                            tamanho_audio = ERROU_LENGTH;
                            wav_position = 0;

                            printf("perdeu otario do krl\n");

                            multicore_fifo_push_blocking(2);
                            multicore_fifo_push_blocking(level);
                            multicore_fifo_push_blocking(pontuacao);
                            level = 0;
                            start_f = 0;
                            break;
                        }
                    }

                    sleep_ms(500);
                    cancel_repeating_timer(&timer_pontuacao);
                } else {
                    printf("Ganhou");
                    wav_position = 0;
                    tamanho_audio = VITORIA_LENGTH;
                    p_audio = VITORIA_DATA;
                    multicore_fifo_push_blocking(3);
                    multicore_fifo_push_blocking(max_level);
                    multicore_fifo_push_blocking(pontuacao);

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