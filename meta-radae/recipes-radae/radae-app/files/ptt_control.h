/*
 * PTT Control Header
 */

#ifndef PTT_CONTROL_H
#define PTT_CONTROL_H

#ifdef ENABLE_GPIO
#include <gpiod.h>
#endif

typedef struct radae_config radae_config_t;

typedef struct {
#ifdef ENABLE_GPIO
    struct gpiod_chip *chip;
    struct gpiod_line *line;
#else
    void *chip;
    void *line;
#endif
    int state;
    int active_low;
    int delay_ms;
} ptt_control_t;

/**
 * Initialize PTT control
 * @param ptt PTT control structure
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int ptt_init(ptt_control_t *ptt, radae_config_t *config);

/**
 * Set PTT state
 * @param ptt PTT control
 * @param active 1 to activate PTT, 0 to deactivate
 * @return 0 on success, -1 on error
 */
int ptt_set(ptt_control_t *ptt, int active);

/**
 * Get PTT state
 * @param ptt PTT control
 * @return Current PTT state (0 or 1)
 */
int ptt_get(ptt_control_t *ptt);

/**
 * Check if PTT is active
 * @param ptt PTT control
 * @return 1 if active, 0 if not
 */
int ptt_is_active(ptt_control_t *ptt);

/**
 * Cleanup PTT control
 * @param ptt PTT control
 */
void ptt_cleanup(ptt_control_t *ptt);

#endif /* PTT_CONTROL_H */
