
#include <stdint.h>
#include <stdbool.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_drv_twi.h"
#include "ble_types.h"

#include "panasonic.h"

extern uint16_t m_conn_handle;
#define IS_CONNECTED() (BLE_CONN_HANDLE_INVALID != m_conn_handle)

static int relay_states[] = { 0, 0, 0 };

static int led_pins[] = {
#if (B_LED_COUNT > 0)
        B_PIN_LED0
#endif
#if (B_LED_COUNT > 1)
        , B_PIN_LED1
#endif
#if (B_LED_COUNT > 2)
        , B_PIN_LED2
#endif
#if (B_LED_COUNT > 3)
        , B_PIN_LED3
#endif
};

static int relay_pins[] = {
#if (B_RELAY_COUNT > 0)
        B_PIN_RELAY0
#endif
#if (B_RELAY_COUNT > 1)
        , B_PIN_RELAY1
#endif
#if (B_RELAY_COUNT > 2)
        , B_PIN_RELAY2
#endif
#if (B_RELAY_COUNT > 3)
        , B_PIN_RELAY1R
#endif
};

static char outq[QSIZE][24] = { 0 };
static int qhead = 0, qtail = 0, qsize = 0;
static int grideye_count = 0, particle_count = 0, relay_count = 0, led_count = 0;

static int enqueue(char *data) {
	if (qsize >= QSIZE) return -1;

	memmove(outq[qhead], data, 20);
	outq[qhead][20] = '\0';
	qhead = (qhead + 1) % QSIZE;
	qsize += 1;

	return 0;
}

extern uint32_t ble_send_str(char *);

void service_output_queue(void) {
        if (0 == qsize || (! IS_CONNECTED())) return;

        uint32_t err_code = ble_send_str(outq[qtail]);
        NRF_LOG_INFO("Sent: (%d) %s", err_code, outq[qtail]);

        if (NRF_SUCCESS == err_code) {
                qtail = (qtail + 1) % QSIZE;
                qsize -= 1;
        }
	NRF_LOG_INFO("-Q: %d", qsize);
}

#if (BOARD_particle)

static volatile int m_xfer_state = 0;
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(B_I2C_BUS);
static uint8_t m_samples[132];
static uint8_t m_reg[4];

static void read_grid_data() {
	if (nrf_drv_twi_is_busy(&m_twi)) {
		NRF_LOG_INFO("Busy");
		return;
	}
	m_reg[0] = 0x80;
	m_xfer_state = 1;
	uint32_t err_code = nrf_drv_twi_tx(&m_twi, B_GRID_I2C_ADDR, m_reg, 1, false);
	NRF_LOG_INFO("%d: %d", __LINE__, err_code);
}

#define CONVERT_TO_F(x) (32 + (((x) * 9) / 20))
static char hexdigits[] = "0123456789ABCDEF";

static void send_grid_data(void) {
	static char reply[24];
	static int gdata[64];

	if (! IS_CONNECTED()) return;

	for (int i = 0; i < 64; i += 1) {
		gdata[i] = CONVERT_TO_F((int16_t)((m_samples[2 * i + 1] << 8) | m_samples[2 * i]));
		if (gdata[i] < 0) gdata[i] = 0;
		if (gdata[i] > 255) gdata[i] = 255;
	}
	for (int i = 0; i < 8; i += 1) {
		reply[0] = 'A' + i;
		reply[1] = '=';
		for (int j = 0; j < 8; j += 1) {
			int v = gdata[8 * i + j];
			reply[2 * j + 2] = hexdigits[0xF & (v >> 4)];
			reply[2 * j + 3] = hexdigits[0xF & v];
		}
		reply[18] = '\0';
		enqueue(reply);
	}
	NRF_LOG_INFO("+Q: %d", qsize);
}

static void read_particle_data() {
	if (nrf_drv_twi_is_busy(&m_twi)) {
		NRF_LOG_INFO("Busy");
		return;
	}
	m_reg[0] = 0x00;
	m_xfer_state = 3;
	ret_code_t err_code = nrf_drv_twi_tx(&m_twi, B_PART_I2C_ADDR, m_reg, 1, true);
	NRF_LOG_INFO("%d: %d", __LINE__, err_code);
}

static void send_particle_data(void) {
	static char reply[24];

	if (! IS_CONNECTED()) return;

	for (int i = 0; i < 3; i += 1) {
		int32_t md = m_samples[4 * i + 0] | (m_samples[4 * i + 1] << 8) | (m_samples[4 * i + 2] << 16);
		int d1 = md / 1000, d2 = md % 1000;

		snprintf(reply, sizeof reply, "%c%d.%03d", 'l' + i, d1, d2);
		enqueue(reply);

		int c1 = m_samples[4 * i + 12] | (m_samples[4 * i + 13]),
			c2 = m_samples[4 * i + 14] | (m_samples[4 * i + 15]);
		
		snprintf(reply, sizeof reply, "%c%u,%u", 'o' + i, c1, c2);
		enqueue(reply);
	}
	NRF_LOG_INFO("+Q: %d", qsize);
}

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context) {
	uint32_t err_code;

	switch (p_event->type) {
	case NRF_DRV_TWI_EVT_DONE:
		NRF_LOG_INFO("Xfer: %d", m_xfer_state);
		switch (m_xfer_state) {
		case 0:
			break;
		case 1:
			memset(m_samples, 0x5A, 128);
			m_xfer_state = 2;
			err_code = nrf_drv_twi_rx(&m_twi, B_GRID_I2C_ADDR, m_samples, 128);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 2:
			m_xfer_state = 0;
			send_grid_data();
			break;
		case 3:
			memset(m_samples, 0xA5, 128);
			m_xfer_state = 4;
			err_code = nrf_drv_twi_rx(&m_twi, B_PART_I2C_ADDR, m_samples, 12);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 4:
			m_reg[0] = 0x0C;
			m_xfer_state = 5;
			err_code = nrf_drv_twi_tx(&m_twi, B_PART_I2C_ADDR, m_reg, 1, true);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 5:
			m_xfer_state = 6;
			err_code = nrf_drv_twi_rx(&m_twi, B_PART_I2C_ADDR, m_samples + 12, 12);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 6:
			m_xfer_state = 0;
			send_particle_data();
			break;
		case 101:
			m_samples[0] = 0xFF;
			m_xfer_state = 102;
			err_code = nrf_drv_twi_rx(&m_twi, B_GRID_I2C_ADDR, m_samples, 1);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 102:
			if (0xFF != m_samples[0]) { grideye_count = 1; }
			NRF_LOG_INFO("GStat: %d", m_samples[0]);

			m_reg[0] = 0x26;
			m_xfer_state = 103;
			err_code = nrf_drv_twi_tx(&m_twi, B_PART_I2C_ADDR, m_reg, 1, true);
			break;
		case 103:
			m_samples[0] = 0xFF;
			m_xfer_state = 104;
			err_code = nrf_drv_twi_rx(&m_twi, B_PART_I2C_ADDR, m_samples, 1);
			NRF_LOG_INFO("%d: %d", __LINE__, err_code);
			break;
		case 104:
			m_xfer_state = 0;
			if (0xFF != m_samples[0]) { particle_count = 1; }
			NRF_LOG_INFO("PStat: %d", m_samples[0]);
			break;
		default:
			m_xfer_state = 0;
			APP_ERROR_CHECK(-1);
			break;
		}
		if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX) {
			NRF_LOG_INFO("%02x %02x %02x %02x", m_samples[0], m_samples[1], m_samples[2], m_samples[3]);
            	}
            	break;
	case NRF_DRV_TWI_EVT_ADDRESS_NACK:
		NRF_LOG_INFO("Addr nack");
		break;
	case NRF_DRV_TWI_EVT_DATA_NACK:
		NRF_LOG_INFO("Data nack");
		break;
        default:
            	break;
    	}
}

void twi_init(void) {
	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_lm75b_config = {
		.scl                = B_PIN_I2C_SCL,
		.sda                = B_PIN_I2C_SDA,
		.frequency          = NRF_DRV_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init     = true
	};

	err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);
	nrf_drv_twi_enable(&m_twi);
}

void twi_reset(void) {
	nrf_drv_twi_disable(&m_twi);
	nrf_drv_twi_uninit(&m_twi);
	twi_init();
	NRF_LOG_INFO("Reset TWI");
}

void app_start(void) {
	m_reg[0] = 0x04;	// status reg
	m_xfer_state = 101;
	uint32_t err_code = nrf_drv_twi_tx(&m_twi, B_GRID_I2C_ADDR, m_reg, 1, false);
	NRF_LOG_INFO("%d: %d", __LINE__, err_code);
}

#elif (BOARD_relay)

void app_start(void) {}

#endif // BOARD_particle


void hardware_init(void) {
        if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) != (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos)) {
                NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
                while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
                        ;
                NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
                        (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);

                NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
                while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
                        ;
                // System reset is needed to update UICR registers.
                NVIC_SystemReset();
        }
	for (int i = 0; i < B_RELAY_COUNT; i += 1) {
                nrf_gpio_cfg(relay_pins[i], NRF_GPIO_PIN_DIR_OUTPUT,
                        NRF_GPIO_PIN_INPUT_DISCONNECT,
                        NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1,
                        NRF_GPIO_PIN_NOSENSE);
                nrf_gpio_pin_write(relay_pins[i], B_RELAY_OFF);
        }
	for (int i = 0; i < B_LED_COUNT; i += 1) {
		nrf_gpio_cfg(led_pins[i], NRF_GPIO_PIN_DIR_OUTPUT,
                        NRF_GPIO_PIN_INPUT_DISCONNECT,
                        NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1,
                        NRF_GPIO_PIN_NOSENSE);
		nrf_gpio_pin_write(led_pins[i], B_LED_ON);
	}
	for (int i = 0; i < (B_LED_COUNT - 1); i += 1) {
		nrf_gpio_pin_write(led_pins[i], B_LED_OFF);
	}
	if (B_RELAY_COUNT > 3) {
		nrf_gpio_pin_write(relay_pins[0], B_RELAY_OFF);
                relay_states[0] = B_RELAY_OFF;

		nrf_gpio_pin_write(relay_pins[2], B_RELAY_OFF);
                relay_states[2] = B_RELAY_OFF;

		nrf_gpio_pin_write(relay_pins[1], B_RELAY_OFF);
		nrf_delay_ms(12);
		nrf_gpio_pin_write(relay_pins[1], B_RELAY_ON);
		nrf_gpio_pin_write(relay_pins[3], B_RELAY_ON);
                relay_states[1] = B_RELAY_OFF;
	}
#if (BOARD_particle)
        twi_init();
#endif    
}

#define IS_CONNECTED() (BLE_CONN_HANDLE_INVALID != m_conn_handle)

void blink_handler(void *p_context) {
	static int count = 0;

	if (IS_CONNECTED()) {
		nrf_gpio_pin_set(led_pins[3]);
	} else {
		nrf_gpio_pin_toggle(led_pins[3]);
	}
	count += 1;
	if (count >= 8) count = 0;

#if (BOARD_particle)
	if (0 == count) {
		read_particle_data();
	} else if (4 == count) {
		read_grid_data();
	}
#endif
}  

extern void send_counts(char *input) {
        char reply[24];
        snprintf(reply, 20, "L%d R%d G%d P%d", led_count, relay_count, grideye_count, particle_count);
        enqueue(reply);
}

extern void send_relay_states(char *input) {
        int r = input[1] - '1';
        if (r >= 0 && r < 3) {
                if ('1' == input[3]) relay_states[r] = 1;
                else relay_states[r] = 0;

                if (r < B_RELAY_COUNT) {
                        nrf_gpio_pin_write(led_pins[r], relay_states[r] ? B_LED_ON : B_LED_OFF);
                        if (1 == r) {
                                if (relay_states[1]) {
                                        nrf_gpio_pin_write(relay_pins[3], 0);
                                } else {
                                        nrf_gpio_pin_write(relay_pins[1], 0);
                                }
                                nrf_delay_ms(12);
                                nrf_gpio_pin_write(relay_pins[1], 1);
                                nrf_gpio_pin_write(relay_pins[3], 1);
                        } else {
                                nrf_gpio_pin_write(relay_pins[r], relay_states[r] ? B_RELAY_ON : B_RELAY_OFF);
                        }
                }
        }
}
