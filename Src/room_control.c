#include "room_control.h"

#include "gpio.h"    // Para controlar LEDs
#include "systick.h" // Para obtener ticks y manejar tiempos
#include "uart.h"    // Para enviar mensajes
#include "tim.h"     // Para controlar el PWM
#include <stdio.h>
#include <stdbool.h> 

// Estados de la sala
typedef enum {
    ROOM_IDLE,
    ROOM_OCCUPIED,
    ROOM_OVERRIDE
} room_state_t;

// Variable de estado global
room_state_t current_state = ROOM_IDLE;
static uint32_t led_on_time = 0;
static uint8_t duty_cycle_percent = 0;
static bool gradual_mode_active = false;
static uint32_t led_gradual_time = 0;
static bool door_open = false;

void room_control_app_init(void)
{
    // Inicializar PWM al duty cycle inicial (estado IDLE -> LED apagado)
    tim3_ch1_pwm_set_duty_cycle(PWM_INITIAL_DUTY);
    uart_send_string("Controlador de Sala v2.0\r\n");
    uart_send_string("Estado inicial:\r\n");
    uart_send_string("-Lámpara: 20%\r\n");
    uart_send_string("-Puerta: Cerrada\r\n");
    uart_send_string("'?': Ayuda\r\n");
}

void room_control_on_button_press(void)
{
    if (current_state == ROOM_IDLE) {
        current_state = ROOM_OVERRIDE;
        tim3_ch1_pwm_set_duty_cycle(100);  // PWM al 100%
        led_on_time = systick_get_ms();
        uart_send_string("Prende lampara 10s\r\n");
    } else {
        current_state = ROOM_IDLE;
        tim3_ch1_pwm_set_duty_cycle(0);  // PWM al 0%
        uart_send_string("Apaga lamapara\r\n");
    }
}

void room_control_on_uart_receive(char received_char)

{


    char buffer[32];

    switch (received_char) {
        case 'g':
        case 'G':
            uart_send_string("Iniciando transición gradual de brillo...\r\n");
            duty_cycle_percent = 0; // Empezar desde 0
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            current_state = ROOM_IDLE;
            gradual_mode_active = true;
            led_gradual_time = systick_get_ms(); // Iniciar el temporizador
            break;
        case 's':
        case 'S':
            sprintf(buffer, "- Puerta: %s\r\n", door_open ? "Abierta" : "Cerrada");
            uart_send_string(buffer);
            break;

        case '?':
            uart_send_string("Comandos disponibles:\r\n");
            uart_send_string("'1'-'5': Ajustar brillo lámpara (10%, 20%, 30%, 40%, 50%)\r\n");
            uart_send_string("'0': Apagar lámpara\r\n");
            uart_send_string("'o': Abrir puerta\r\n");
            uart_send_string("'c': Cerrar puerta\r\n");
            uart_send_string("'s': Estado del sistema\r\n");
            uart_send_string("'?': Ayuda\r\n");
            break;

        case 'O':
        case 'o':
            door_open=true;
            uart_send_string("Puerta abierta\r\n");
            break;
        case 'C':
        case 'c':
            door_open=false;
            uart_send_string("Puerta cerrada\r\n");
            break;
        case '0':
            duty_cycle_percent = 0;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("Lampara apagada\r\n");
            break;
        case '1':
            duty_cycle_percent = 10;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("PWM: 10%\r\n");
            break;
        case '2':
            duty_cycle_percent = 20;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("PWM: 20%\r\n");
            break;
        case '3':
            duty_cycle_percent = 30;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("PWM: 30%\r\n");
            break;
        case '4':
            duty_cycle_percent = 40;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("PWM: 40%\r\n");
            break;
        case '5':
            duty_cycle_percent = 50;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("PWM: 50%\r\n");
            break;
        default:
            uart_send_string("Comando desconocido: ");
            uart_send_string("'?': Ayuda\r\n");
            uart_send(received_char);
            uart_send_string("\r\n");
            break;
    }
}

void room_control_update(void)
{
    if (current_state == ROOM_OVERRIDE) {
        if (systick_get_ms() - led_on_time >= LED_TIMEOUT_MS) {
            current_state = ROOM_IDLE;
            tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
            uart_send_string("Han pasado 10 segundos\r\n");
            current_state = (duty_cycle_percent > 0) ? ROOM_OCCUPIED : ROOM_IDLE;
        }
    }

    if (gradual_mode_active) {
        
        if (systick_get_ms() - led_gradual_time >= 500) {
            if (duty_cycle_percent < 100) {
                duty_cycle_percent += 10;
                tim3_ch1_pwm_set_duty_cycle(duty_cycle_percent);
                current_state = ROOM_OCCUPIED; // La lámpara ya está encendida
                led_gradual_time = systick_get_ms(); // Reiniciar temporizador
            } else {
                gradual_mode_active = false;
                uart_send_string("Transición completada.\r\n");
            }
        }
    }
}