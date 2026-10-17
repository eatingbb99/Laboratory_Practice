/* ============================================================================
 * Лабораторная работа №1. Вариант 4.
 * Плата: Nucleo-144 (STM32F429ZI).
 *
 * Схема: 4 светодиода, 3 кнопки.
 *   - PB14 (LD3, красный)   - индикация кнопки 1;
 *   - PB7  (LD2, синий)     - индикация кнопки 2;
 *   - PB0  (LD1, зелёный)   - индикация кнопки 3 в режиме "вход"
 *                             (в задании указан жёлтый светодиод,
 *                              на плате его нет - используется зелёный);
 *   - PA5  (управляемый порт) - к нему параллельно подключены
 *                             кнопка 3 и внешний светодиод.
 *
 * Требование задания: один светодиод настраивается прямым обращением
 * к памяти, второй - собственными директивами/макросами, остальные - CMSIS.
 *   PB7  -> прямой доступ к памяти (GPIOB_BSRR);
 *   PB0  -> собственные макросы (LED_GREEN_ON/OFF);
 *   PB14, PA5 -> CMSIS (GPIOB/GPIOA, RCC).
 * ========================================================================== */

#include "stm32f4xx.h"

/* ---------------------------- пины ---------------------------------------- */
#define LED_RED_PIN     14u   /* PB14, LD3 */
#define LED_BLUE_PIN     7u   /* PB7,  LD2 */
#define LED_GREEN_PIN    0u   /* PB0,  LD1 */
#define PORT_PIN         5u   /* PA5,  управляемый порт */

#define BTN1_PIN         0u   /* PA0, кнопка "режим вход"    */
#define BTN2_PIN         1u   /* PA1, кнопка "режим выход"   */

/* --------- светодиод PB7: прямой доступ к памяти --------------------------- */
#define GPIOB_BSRR_ADDR  (0x40020400u + 0x18u)
#define GPIOB_BSRR       (*(volatile uint32_t *)GPIOB_BSRR_ADDR)

#define LED_BLUE_ON()    (GPIOB_BSRR = (1u << LED_BLUE_PIN))
#define LED_BLUE_OFF()   (GPIOB_BSRR = (1u << (LED_BLUE_PIN + 16u)))

/* --------- светодиод PB0: собственные директивы ---------------------------- */
#define LED_GREEN_ON()   (GPIOB->BSRR = (1u << LED_GREEN_PIN))
#define LED_GREEN_OFF()  (GPIOB->BSRR = (1u << (LED_GREEN_PIN + 16u)))

/* ---------------------------- утилиты ------------------------------------- */
static void delay(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

/* PB14 через CMSIS */
static void led_red(int on)
{
    if (on) {
        GPIOB->BSRR = (1u << LED_RED_PIN);
    } else {
        GPIOB->BSRR = (1u << (LED_RED_PIN + 16u));
    }
}

/* PA5 -> вход с подтяжкой к питанию */
static void port_set_input(void)
{
    GPIOA->MODER &= ~(3u << (PORT_PIN * 2u));
    GPIOA->PUPDR &= ~(3u << (PORT_PIN * 2u));
    GPIOA->PUPDR |=  (1u << (PORT_PIN * 2u));
}

/* PA5 -> выход, на выходе лог. 1 (внешний светодиод горит) */
static void port_set_output(void)
{
    GPIOA->MODER &= ~(3u << (PORT_PIN * 2u));
    GPIOA->MODER |=  (1u << (PORT_PIN * 2u));
    GPIOA->BSRR   =  (1u << PORT_PIN);
}

/* ---------------------------- инициализация -------------------------------- */
static void gpio_init(void)
{
    /* тактирование портов A и B */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* светодиоды PB0, PB7, PB14 - выход */
    GPIOB->MODER &= ~((3u << (LED_RED_PIN * 2u)) |
                      (3u << (LED_BLUE_PIN * 2u)) |
                      (3u << (LED_GREEN_PIN * 2u)));
    GPIOB->MODER |=  ((1u << (LED_RED_PIN * 2u)) |
                      (1u << (LED_BLUE_PIN * 2u)) |
                      (1u << (LED_GREEN_PIN * 2u)));

    /* кнопки PA0, PA1 - вход с подтяжкой к питанию */
    GPIOA->MODER &= ~((3u << (BTN1_PIN * 2u)) | (3u << (BTN2_PIN * 2u)));
    GPIOA->PUPDR &= ~((3u << (BTN1_PIN * 2u)) | (3u << (BTN2_PIN * 2u)));
    GPIOA->PUPDR |=  ((1u << (BTN1_PIN * 2u)) | (1u << (BTN2_PIN * 2u)));

    /* управляемый порт PA5 - по умолчанию вход */
    port_set_input();

    /* всё выключено */
    led_red(0);
    LED_BLUE_OFF();
    LED_GREEN_OFF();
}

/* ---------------------------- основная программа --------------------------- */
int main(void)
{
    gpio_init();

    for (;;) {
        /* кнопка 1: удержание -> красный светодиод, порт PA5 на вход */
        if (!(GPIOA->IDR & (1u << BTN1_PIN))) {
            led_red(1);
            port_set_input();
        } else {
            led_red(0);
        }

        /* кнопка 2: удержание -> синий светодиод, порт PA5 на выход */
        if (!(GPIOA->IDR & (1u << BTN2_PIN))) {
            LED_BLUE_ON();
            port_set_output();
        } else {
            LED_BLUE_OFF();
        }

        /* кнопка 3 (на PA5): работает только когда порт в режиме входа */
        if ((GPIOA->MODER & (3u << (PORT_PIN * 2u))) == 0u) {
            if (!(GPIOA->IDR & (1u << PORT_PIN))) {
                LED_GREEN_ON();      /* кнопка удерживается - зелёный (жёлтый) */
            } else {
                LED_GREEN_OFF();
            }
        } else {
            LED_GREEN_OFF();         /* режим выхода - внешний светодиод горит */
        }

        delay(200000u);              /* простейшая защита от дребезга */
    }
}
