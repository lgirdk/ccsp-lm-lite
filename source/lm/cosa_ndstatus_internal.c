/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

/**************************************************************************

    module: cosa_wifi_dml.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CosaWifiCreate
        *  CosaWifiInitialize
        *  CosaWifiRemove
    -------------------------------------------------------------------

    environment:

        platform independent

**************************************************************************/

#include "cosa_ndstatus_internal.h"
#include "cosa_ndstatus_dml.h"
//extern void* g_pDslhDmlAgent;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CosaWifiCreate
            (
            );

    description:

        This function constructs cosa wifi object and return handle.

    argument:  

    return:     newly created wifi object.

**********************************************************************/

ANSC_HANDLE
CosaLMLiteCreate
    (
        VOID
    )
{
	PCOSA_DATAMODEL_LMLITE       pMyObject    = (PCOSA_DATAMODEL_LMLITE)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_LMLITE)AnscAllocateMemory(sizeof(COSA_DATAMODEL_LMLITE));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    return  (ANSC_HANDLE)pMyObject;
}


/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWifiInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa wifi object and return handle.

    argument:	ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
CosaLMLiteInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_LMLITE       pMyObject           = (PCOSA_DATAMODEL_LMLITE)hThisObject;
   

    returnStatus = CosaDmlLMLiteInit((ANSC_HANDLE)pMyObject);
    
    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {        
        return  returnStatus;
    }
    
    return returnStatus;
}


/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CosaWifiRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa wifi object and return handle.

    argument:	ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/
ANSC_STATUS
CosaLMLiteRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_LMLITE            pMyObject    = (PCOSA_DATAMODEL_LMLITE)hThisObject;    
        
    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

	return returnStatus;
}

