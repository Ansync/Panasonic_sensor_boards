#ifndef _PANASONIC_H_
#define _PANASONIC_H_

#undef DEVICE_NAME
#define DEVICE_NAME                     "Pan-XXXXXX"                                /**< Name of device. Will be included in the advertising data. */
#define BLINK_INTERVAL                  250
#define QSIZE                           32
    
#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#if (BOARD_relay)
#       define B_LED_COUNT              4
#       define B_PIN_LED0               29
#       define B_PIN_LED1               28
#       define B_PIN_LED2               31
#       define B_PIN_LED3               34
#       define B_RELAY_COUNT            3
#       define B_PIN_RELAY0             2
#       define B_PIN_RELAY1             5
#       define B_PIN_RELAY1R            3
#       define B_PIN_RELAY2             30
#       define B_PIN_UART_RX            38
#       define B_PIN_UART_TX            36
#elif (BOARD_particle)
#       define B_LED_COUNT              4
#       define B_PIN_LED0               22
#       define B_PIN_LED1               13
#       define B_PIN_LED2               4
#       define B_PIN_LED3               34
#       define B_RELAY_COUNT            3
#       define B_PIN_RELAY0             20
#       define B_PIN_RELAY1             24
#       define B_PIN_RELAY1R            32
#       define B_PIN_RELAY2             26
#       define B_PIN_UART_RX            38
#       define B_PIN_UART_TX            36
#       define B_I2C_BUS                0
#       define B_PIN_I2C_SCL            2
#       define B_PIN_I2C_SDA            29
#       define B_GRID_I2C_ADDR          0x68
#       define B_PART_I2C_ADDR          0x33
#else // Assume pca10056
#       define B_LED_COUNT              4
#       define B_PIN_LED0               13
#       define B_PIN_LED1               14
#       define B_PIN_LED2               15
#       define B_PIN_LED3               16
#       define B_RELAY_COUNT            0
#       define B_PIN_UART_RX            8
#       define B_PIN_UART_TX            6
#endif

#define B_CLOCK_SOURCE                  2
#define B_LED_ON		        1
#define B_LED_OFF		        0
#define B_RELAY_ON		        1
#define B_RELAY_OFF		        0

extern void hardware_init(void);
extern void blink_handler(void *);
extern void service_output_queue(void);
extern void send_counts(char *);
extern void send_relay_states(char *);
extern void app_start(void);

#endif // _PANASONIC_H_