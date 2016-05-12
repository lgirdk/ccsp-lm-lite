/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef  _COSA_REPORTS_INTERNAL_H
#define  _COSA_REPORTS_INTERNAL_H


#include "ansc_platform.h"
#include "ansc_string_util.h"

#define COSA_DATAMODEL_REPORTS_OID                                                        1

/* Collection */

#define  COSA_DATAMODEL_REPORTS_CLASS_CONTENT                       \
    /* start of WIFI object class content */                        \
    BOOLEAN                         bNDSEnabled;                   \
    BOOLEAN                         bNDSEnabledChanged;            \
    ULONG                           uNDSPollingPeriod;             \
    BOOLEAN                         bNDSPollingPeriodChanged;      \
    ULONG                           uNDSReportingPeriod;           \
    BOOLEAN                         bNDSReportingPeriodChanged;    \
    ULONG                           uNDSPollingPeriodDefault;      \
    ULONG                           uNDSReportingPeriodDefault;    \
    ULONG                           uNDSOverrideTTL;                \
    BOOLEAN                         bNDTEnabled;                   \
    BOOLEAN                         bNDTEnabledChanged;            \
    ULONG                           uNDTPollingPeriod;             \
    BOOLEAN                         bNDTPollingPeriodChanged;      \
    ULONG                           uNDTReportingPeriod;           \
    BOOLEAN                         bNDTReportingPeriodChanged;    \
    ULONG                           uNDTPollingPeriodDefault;      \
    ULONG                           uNDTReportingPeriodDefault;    \
    ULONG                           uNDTOverrideTTL;


typedef  struct
_COSA_DATAMODEL_REPORTS                                               
{
	COSA_DATAMODEL_REPORTS_CLASS_CONTENT
}
COSA_DATAMODEL_REPORTS,  *PCOSA_DATAMODEL_REPORTS;

/*
    Standard function declaration 
*/
ANSC_HANDLE
CosaReportsCreate
    (
        VOID
    );

ANSC_STATUS
CosaReportsInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CosaReportsRemove
    (
        ANSC_HANDLE                 hThisObject
    );
    
#endif 
