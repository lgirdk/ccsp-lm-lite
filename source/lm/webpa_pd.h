/**
 * @file webpa_pd.h
 *
 * @description This header defines parodus log levels
 *
 */

#ifndef _WEBPA_PD_H_
#define _WEBPA_PD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEVICE_PROPS_FILE  "/etc/device.properties"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/**
 *  Returns parodus URL.
 *
 *  @note The caller should ensure that size of url is URL_SIZE or more.
 *
 *  @param url   [out] URL where parodus is listening.
 */
void get_parodus_url(char **url);

#ifdef __cplusplus
}
#endif

#endif

