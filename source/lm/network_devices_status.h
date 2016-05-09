/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef  LMLITE_NETWORK_DEVICES_H
#define  LMLITE_NETWORK_DEVICES_H

#include "ansc_platform.h"

/**
 * @brief Set the Harvesting Status for Network Devices.
 *
 * @param[in] status New Harvesting Status.
 * @return status 0 for success and 1 for failure
 */
int SetNDHarvestingStatus(BOOL status);

/**
 * @brief Gets the Harvesting Status for Network Devices.
 *
 * @return status true if enabled and false if disabled
 */
BOOL GetNDHarvestingStatus();

/**
 * @brief Set the Reporting Period for Network Devices Scan.
 *
 * @param[in] period Time in Seconds.
 * @return status 0 for success and 1 for failure
 */
int SetNDReportingPeriod(ULONG period);

/**
 * @brief Gets the Network Devices Reporting Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDReportingPeriod();

/**
 * @brief Set the Polling Period for Network Devices Scan.
 *
 * @param[in] period Time in Seconds.
 * @return status 0 for success and 1 for failure
 */
int SetNDPollingPeriod(ULONG period);

/**
 * @brief Gets the Network Devices Polling Period
 *
 * @return period : The Current Polling Period
 */
ULONG GetNDPollingPeriod();

/**
 * @brief Gets the Default Network Devices Reporting Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDReportingPeriodDefault();

/**
 * @brief Gets the Default Network Devices Polling Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDPollingPeriodDefault();

/**
 * @brief Gets the Default timeout for Accelerated Scans
 *
 * @return period : The default timeout
 */
ULONG GetNDOverrideTTLDefault();

/**
 * @brief Validated the Period Values for ND Scan and makes sure they are 
 *        present in the valid range of Values.
 *
 * @param[in] period period to be validated.
 * @return status 0 for success and 1 for failure
 */
BOOL ValidateNDPeriod(ULONG period);

#endif 
