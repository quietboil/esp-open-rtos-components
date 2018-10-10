/**
 * \file  sunriset.h
 * \brief Computes Sun rise/set times and start/end of twilight
 *        at any date and latitude
 */
#ifndef __SUNRISET_H
#define __SUNRISET_H

/**
 * \brief Sun's 24h position relative to the specified altitude
 */
typedef enum {
    ALWAYS_BELOW = -1,  ///< Sun is always below the altitude
    BELOW_AND_ABOVE,    ///< Sun rises/sets on the specified day
    ALWAYS_ABOVE        ///< Sun is always above the altitude
} sun_24h_pos_t;

/**
 * \brief Computes sunrise/sunset times
 *
 * \param year      Calendar year (1801-2099 only)
 * \param month     Calendar month
 * \param day       Calendar day. Day starts and ends at UT.
 * \param latitude  Latitude. Northern positive, Southern negative
 * \param longitude Longitude. Eastern positive, Western negative
 * \param altitude  The altitude which the Sun should cross. Set to
 *                  -35/60 degrees for rise/set,
 *                  -6 degrees for civil,
 *                  -12 degrees for nautical and
 *                  -18 degrees for astronomical twilight.
 * \param upper_limb  non-zero -> upper limb, zero -> center
 *                  Set to non-zero (e.g. 1) when computing rise/set times,
 *                  and to zero when computing start/end of twilight.
 * \param sunrise   where to store the rise time (in hours UT)
 * \param sunset    where to store the set time (in hours UT)
 * \return
 *      BELOW_AND_ABOVE = Sun rises/sets this day.
 *      ALWAYS_ABOVE = Sun above the specified "horizon" 24 hours.
 *          sunrise set to time when the sun is at south minus 12 hours while
 *          sunset is set to the south time plus 12 hours.
 *          "Day" length = 24 hours.
 *      ALWAYS_BELOW = Sun is below the specified "horizon" 24 hours.
 *          "Day" length = 0 hours,
 *          sunrise and sunset are both set to the time when the sun is at south.
 *
 * Both sunrise and sunset times are relative to the specified altitude, and
 * thus this function can be used to compute various twilight times, as
 * well as rise/set times.
 */
sun_24h_pos_t __sunriset__(int year, int month, int day,
        double latitude, double longitude, double altitude, int upper_limb,
        double * sunrise, double * sunset);

/**
 * \brief Computes times for sunrise/sunset.
 *
 * Sunrise/set is considered to occur when the Sun's upper limb is
 * 35 arc minutes below the horizon (this accounts for the refraction
 * of the Earth's atmosphere).
 */
static inline sun_24h_pos_t sunrise_sunset(int year, int month, int day,
        double latitude, double longitude, double * sunrise, double * sunset)
{
    return __sunriset__(year, month, day, latitude, longitude, -35.0/60.0, 1, sunrise, sunset);
}

/**
 * \brief Computes the start and end times of civil twilight.
 *
 * Civil twilight starts/ends when the Sun's center is 6 degrees below
 * the horizon.
 */
static inline sun_24h_pos_t civil_twilight(int year, int month, int day,
        double latitude, double longitude, double * sunrise, double * sunset)
{
    return __sunriset__(year, month, day, latitude, longitude, -6.0, 0, sunrise, sunset);
}

/**
 * \brief Computes the start and end times of nautical twilight.
 *
 * Nautical twilight starts/ends when the Sun's center is 12 degrees
 * below the horizon.
 */
static inline sun_24h_pos_t nautical_twilight(int year, int month, int day,
        double latitude, double longitude, double * sunrise, double * sunset)
{
    return __sunriset__(year, month, day, latitude, longitude, -12.0, 0, sunrise, sunset);
}

/**
 * \brief Computes the start and end times of astronomical twilight.
 *
 * Astronomical twilight starts/ends when the Sun's center is 18 degrees
 * below the horizon.
 */
static inline sun_24h_pos_t astronomical_twilight(int year, int month, int day,
        double latitude, double longitude, double * sunrise, double * sunset)
{
    return __sunriset__(year, month, day, latitude, longitude, -18.0, 0, sunrise, sunset);
}

/**
 * \brief Splits floating point number of hours into integer hours, minutes
 *        and seconds.
 * \param H  hours since midnight (double)
 * \param h  hours (int)
 * \param m  minuts (int)
 * \param s  seconds (int)
 */
#define H2T(H, h, m, s) do { \
    h = H; \
    int t = (H - h) * 3600; \
    m = t / 60; \
    s = t % 60; \
} while (0)

#endif