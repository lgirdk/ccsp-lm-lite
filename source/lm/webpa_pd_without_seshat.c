#if defined(PARODUS_ENABLE) && !defined(ENABLE_SESHAT)
#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "webpa_interface.h"
#include "webpa_pd.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void get_parodus_url(char *url)
{
    FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

    if( NULL != fp ) {
        char str[255] = {'\0'};
        while( fscanf(fp,"%s", str) != EOF) {
            char *value = NULL;
            if( value = strstr(str, "PARODUS_URL=") ) {
                value = value + strlen("PARODUS_URL=");
                strncpy(url, value, (strlen(str) - strlen("PARODUS_URL="))+1);
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parodus url is %s\n", url));
            }
        }
    } else {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE));
    }
    fclose(fp);

    if( 0 == url[0] ) {
        CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, parodus url is not present in device.properties file:%s\n", url));
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parodus url formed is %s\n", url));
}
#endif
