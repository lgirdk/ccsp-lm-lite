#if defined(PARODUS_ENABLE) && defined(ENABLE_SESHAT)
#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "webpa_interface.h"
#include "webpa_pd.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PARODUS_SERVICE    "Parodus"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void get_seshat_url(char *url);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void get_parodus_url(char *url)
{
    char seshat_url[URL_SIZE] = {'\0'};
    char *discovered_url = NULL;
    size_t discovered_url_size = 0;

    get_seshat_url(seshat_url);
    if( 0 == seshat_url[0] ) {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, seshat_url is not present in device.properties file:%s\n", seshat_url));
        return;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_INFO, seshat_url formed is %s\n", seshat_url));
    if( 0 == init_lib_seshat(seshat_url) ) {
        CcspLMLiteConsoleTrace(("RDK_LOG_INFO, seshatlib initialized! (url %s)\n", seshat_url));

        discovered_url = seshat_discover(PARODUS_SERVICE);
        if( NULL != discovered_url ) {
            discovered_url_size = strlen(discovered_url);
            strncpy(url, discovered_url, discovered_url_size);
            url[discovered_url_size] = '\0';
            CcspLMLiteConsoleTrace(("RDK_LOG_INFO, seshatlib discovered url = %s, parodus url = %s\n", discovered_url, url));
            free(discovered_url);
        } else {
            CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, seshatlib registration error (url %s)!", discovered_url));
        }
    } else {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, seshatlib not initialized! (url %s)\n", seshat_url));
    }

    if( 0 == url[0] ) {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, parodus url (url %s) is not present in seshatlib (url %s)\n", url, seshat_url));
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parodus url formed is %s\n", url));

    shutdown_seshat_lib();
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 * @brief Helper function to retrieve seshat URL.
 * 
 * @param[out] URL.
 *
 */
static void get_seshat_url(char *url)
{
    FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

    if( NULL != fp ) {
        char str[255] = {'\0'};
        while( fscanf(fp,"%s", str) != EOF ) {
            char *value = NULL;
            if( value = strstr(str, "SESHAT_URL=") ) {
                value = value + strlen("SESHAT_URL=");
                strncpy(url, value, (strlen(str) - strlen("SESHAT_URL="))+1);
                CcspLMLiteConsoleTrace(("RDK_LOG_INFO, seshat_url is %s\n", url));
            }
        }
    } else {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE));
    }
    fclose(fp);
}
#endif

