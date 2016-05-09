/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef  _COSA_LMLITE_INTERNAL_H
#define  _COSA_LMLITE_INTERNAL_H


#include "ansc_platform.h"
#include "ansc_string_util.h"

#define COSA_DATAMODEL_LMLITE_OID                                                        1

/* Collection */

#define  COSA_DATAMODEL_LMLITE_CLASS_CONTENT                    \
    /* start of WIFI object class content */                    \
    BOOLEAN                         bEnabled;                   \
    BOOLEAN                         bEnabledChanged;            \
    ULONG                           uPollingPeriod;             \
    BOOLEAN                         bPollingPeriodChanged;      \
    ULONG                           uReportingPeriod;           \
    BOOLEAN                         bReportingPeriodChanged;    \
    ULONG                           uPollingPeriodDefault;      \
    ULONG                           uReportingPeriodDefault;    \
    ULONG                           uOverrideTTL;



typedef  struct
_COSA_DATAMODEL_LMLITE                                               
{
	COSA_DATAMODEL_LMLITE_CLASS_CONTENT
}
COSA_DATAMODEL_LMLITE,  *PCOSA_DATAMODEL_LMLITE;

/*
    Standard function declaration 
*/
ANSC_HANDLE
CosaLMLiteCreate
    (
        VOID
    );

ANSC_STATUS
CosaLMLiteInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CosaLMLiteRemove
    (
        ANSC_HANDLE                 hThisObject
    );
    

#endif 
