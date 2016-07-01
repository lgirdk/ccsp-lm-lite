/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef _HARVESTER_AVRO_H
#define _HARVESTER_AVRO_H

#include <sys/time.h>
#include <pthread.h>

#if (defined SIMULATION)
#define INTERFACE_DEVICES_WIFI_AVRO_FILENAME			"InterfaceDevicesWifi.avsc"
#else
#define INTERFACE_DEVICES_WIFI_AVRO_FILENAME			"/usr/ccsp/lm/InterfaceDevicesWifi.avsc"
#endif
#define CHK_AVRO_ERR (strlen(avro_strerror()) > 0)

//Please do not edit the elements for this data structure 
typedef struct _wifi_associated_dev
{
     //UCHAR cli_devMacAddress[6];
     //CHAR  cli_devIPAddress[64];
     //BOOL  cli_devAssociatedDeviceAuthentiationState;
     //INT   cli_devSignalStrength;
     //INT   cli_devTxRate;
     //INT   cli_devRxRate;
	 
	 UCHAR cli_MACAddress[6];		// The MAC address of an associated device.
	 CHAR  cli_IPAddress[64];		// IP of the associated device
	 BOOL  cli_AuthenticationState; // Whether an associated device has authenticated (true) or not (false).
	 UINT  cli_LastDataDownlinkRate; //The data transmit rate in kbps that was most recently used for transmission from the access point to the associated device.
	 UINT  cli_LastDataUplinkRate; 	// The data transmit rate in kbps that was most recently used for transmission from the associated device to the access point.
	 INT   cli_SignalStrength; 		//An indicator of radio signal strength of the uplink from the associated device to the access point, measured in dBm, as an average of the last 100 packets received from the device.
	 UINT  cli_Retransmissions; 	//The number of packets that had to be re-transmitted, from the last 100 packets sent to the associated device. Multiple re-transmissions of the same packet count as one.
	 BOOL  cli_Active; 				//	boolean	-	Whether or not this node is currently present in the WiFi AccessPoint network.
	
	 CHAR  cli_OperatingStandard[64];	//Radio standard the associated Wi-Fi client device is operating under. Enumeration of:
	 CHAR  cli_OperatingChannelBandwidth[64];	//The operating channel bandwidth of the associated device. The channel bandwidth (applicable to 802.11n and 802.11ac specifications only). Enumeration of:
	 INT   cli_SNR;		//A signal-to-noise ratio (SNR) compares the level of the Wi-Fi signal to the level of background noise. Sources of noise can include microwave ovens, cordless phone, bluetooth devices, wireless video cameras, wireless game controllers, fluorescent lights and more. It is measured in decibels (dB).
	 CHAR  cli_InterferenceSources[64]; //Wi-Fi operates in two frequency ranges (2.4 Ghz and 5 Ghz) which may become crowded other radio products which operate in the same ranges. This parameter reports the probable interference sources that this Wi-Fi access point may be observing. The value of this parameter is a comma seperated list of the following possible sources: eg: MicrowaveOven,CordlessPhone,BluetoothDevices,FluorescentLights,ContinuousWaves,Others
	 ULONG cli_DataFramesSentAck;	//The DataFramesSentAck parameter indicates the total number of MSDU frames marked as duplicates and non duplicates acknowledged. The value of this counter may be reset to zero when the CPE is rebooted. Refer section A.2.3.14 of CableLabs Wi-Fi MGMT Specification.
	 ULONG cli_DataFramesSentNoAck;	//The DataFramesSentNoAck parameter indicates the total number of MSDU frames retransmitted out of the interface (i.e., marked as duplicate and non-duplicate) and not acknowledged, but does not exclude those defined in the DataFramesLost parameter. The value of this counter may be reset to zero when the CPE is rebooted. Refer section A.2.3.14 of CableLabs Wi-Fi MGMT Specification.
	 ULONG cli_BytesSent;	//The total number of bytes transmitted to the client device, including framing characters.
	 ULONG cli_BytesReceived;	//The total number of bytes received from the client device, including framing characters.
	 INT   cli_RSSI;	//The Received Signal Strength Indicator, RSSI, parameter is the energy observed at the antenna receiver for transmissions from the device averaged over past 100 packets recevied from the device.
	 INT   cli_MinRSSI;	//The Minimum Received Signal Strength Indicator, RSSI, parameter is the minimum energy observed at the antenna receiver for past transmissions (100 packets).
	 INT   cli_MaxRSSI;	//The Maximum Received Signal Strength Indicator, RSSI, parameter is the energy observed at the antenna receiver for past transmissions (100 packets).
	 UINT  cli_Disassociations;	//This parameter  represents the total number of client disassociations. Reset the parameter evey 24hrs or reboot
	 UINT  cli_AuthenticationFailures;	//This parameter indicates the total number of authentication failures.  Reset the parameter evey 24hrs or reboot
	 
} wifi_associated_dev_t;	//~COSA_DML_WIFI_AP_ASSOC_DEVICE

struct associateddevicedata
{
struct timeval timestamp;
char* sSidName;
char* bssid;
char* radioOperatingFrequencyBand; //Possible value 2.4Ghz and 5.0 Ghz
ULONG radioChannel;  // Possible Value between 1-11
ULONG numAssocDevices;
wifi_associated_dev_t* devicedata;
char* parent;

struct associateddevicedata *next;
};

extern void extender_report_associateddevices(struct associateddevicedata *head, char* ServiceType);

#endif /* !_HARVESTER_AVRO_H */
