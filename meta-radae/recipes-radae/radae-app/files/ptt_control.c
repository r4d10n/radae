/*
 * PTT (Push-to-Talk) Control via GPIO
 *
 * Manages PTT control using Linux GPIO interface
 * Supports both active-high and active-low configurations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef ENABLE_GPIO
#include <gpiod.h>
#endif

#include "ptt_control.h"
#include "config_parser.h"

int ptt_init(ptt_control_t *ptt, radae_config_t *config) {
#ifdef ENABLE_GPIO
    memset(ptt, 0, sizeof(ptt_control_t));

    ptt->active_low = config->ptt_active_low;
    ptt->delay_ms = config->ptt_delay_ms;

    // Open GPIO chip
    ptt->chip = gpiod_chip_open_by_name(config->ptt_gpio_chip);
    if (!ptt->chip) {
        fprintf(stderr, "Failed to open GPIO chip %s\n", config->ptt_gpio_chip);
        return -1;
    }

    // Get GPIO line
    ptt->line = gpiod_chip_get_line(ptt->chip, config->ptt_gpio_line);
    if (!ptt->line) {
        fprintf(stderr, "Failed to get GPIO line %d\n", config->ptt_gpio_line);
        gpiod_chip_close(ptt->chip);
        return -1;
    }

    // Request line as output
    int ret = gpiod_line_request_output(ptt->line, "radae-ptt", 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to request GPIO line as output\n");
        gpiod_chip_close(ptt->chip);
        return -1;
    }

    ptt->state = 0;
    ptt_set(ptt, 0);  // Ensure PTT is off initially

    printf("PTT initialized on %s:%d (active %s)\n",
           config->ptt_gpio_chip, config->ptt_gpio_line,
           ptt->active_low ? "low" : "high");

    return 0;
#else
    fprintf(stderr, "PTT support not compiled in (ENABLE_GPIO not defined)\n");
    return -1;
#endif
}

int ptt_set(ptt_control_t *ptt, int active) {
#ifdef ENABLE_GPIO
    if (!ptt->line) {
        return -1;
    }

    // Convert logical state to physical GPIO value
    int gpio_value = ptt->active_low ? !active : active;

    int ret = gpiod_line_set_value(ptt->line, gpio_value);
    if (ret < 0) {
        fprintf(stderr, "Failed to set PTT state\n");
        return -1;
    }

    // Apply delay if activating PTT
    if (active && !ptt->state && ptt->delay_ms > 0) {
        usleep(ptt->delay_ms * 1000);
    }

    ptt->state = active;
    return 0;
#else
    return -1;
#endif
}

int ptt_get(ptt_control_t *ptt) {
    return ptt->state;
}

int ptt_is_active(ptt_control_t *ptt) {
    return ptt->state != 0;
}

void ptt_cleanup(ptt_control_t *ptt) {
#ifdef ENABLE_GPIO
    if (ptt->line) {
        // Ensure PTT is off before cleanup
        ptt_set(ptt, 0);
        gpiod_line_release(ptt->line);
        ptt->line = NULL;
    }

    if (ptt->chip) {
        gpiod_chip_close(ptt->chip);
        ptt->chip = NULL;
    }
#endif
}
