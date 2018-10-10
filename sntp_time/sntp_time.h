#ifndef __SNTP_TIME_H
#define __SNTP_TIME_H

#include <stdbool.h>

/**
 * \brief Initializes SNTP time keeping structures
 */
void sntp_time_init(void);

/**
 * \brief Queries whether system time has been set from an NTP source
 * \return true if system clock is set
 */
bool sntp_time_is_set(void);


#endif