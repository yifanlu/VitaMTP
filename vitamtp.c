//
//  vitamtp.c
//  VitaMTP
//
//  Created by Yifan Lu on 2/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vitamtp.h"

/**
 * Get the first (as in "first in the list of") connected Vita MTP device.
 * @return a device pointer. NULL if error, no connected device, or no connected Vita
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBVitaMTP_Get_First_Vita(void)
{
    LIBMTP_mtpdevice_t *first_device = NULL;
    LIBMTP_raw_device_t *devices;
    int numdevs;
    int i;
    LIBMTP_error_number_t ret;
    
    ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
    if (ret != LIBMTP_ERROR_NONE) {
        return NULL;
    }
    
    if (devices == NULL || numdevs == 0) {
        return NULL;
    }
    
    for (i = 0; i < numdevs; i++)
    {
        if(devices[i].device_entry.vendor_id == VITA_VID &&
           devices[i].device_entry.product_id == VITA_PID)
        {
            first_device = LIBMTP_Open_Raw_Device(&devices[i]);
            break;
        }
        LIBMTP_Release_Device(first_device);
    }
    
    
    free(devices);
    return first_device; // NULL if no vita
}

uint16_t VitaMTP_GetVitaInfo(LIBMTP_mtpdevice_t *device, vita_info_t *info)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    int ret;
    
	PTP_CNT_INIT(ptp);
	ptp.Code = PTP_OC_VITA_GetVitaInfo;
	ptp.Nparam = 0;
	ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &info->raw_xml, 0);
    if(ret != 0)
    {
        return ret;
    }
    // TODO: Parse XML
    int len;
    memcpy(&len, info->raw_xml, sizeof(int));
    return 0;
}

uint16_t VitaMTP_SendInitiatorInfo(LIBMTP_mtpdevice_t *device, initiator_info_t *info)
{
    // TODO: Parse XML
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendInitiatorInfo;
    ptp.Nparam = 0;
    return ptp_transaction(params, &ptp, PTP_DP_SENDDATA, info->raw_len, &info->raw_xml, 0);
}

uint16_t VitaMTP_SendHostStatus(LIBMTP_mtpdevice_t *device, uint32_t status)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendHostStatus;
    ptp.Nparam = 1;
    ptp.Param1 = status;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

uint16_t VitaMTP_GetSettingInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, settings_info_t *info)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    int ret;
    
	PTP_CNT_INIT(ptp);
	ptp.Code = PTP_OC_VITA_GetSettingInfo;
	ptp.Nparam = 1;
    ptp.Param1 = event_id;
	ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &info->raw_xml, 0);
    if(ret != 0)
    {
        return ret;
    }
    // TODO: Parse XML
    return 0;
}

uint16_t VitaMTP_ReportResult(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint16_t result)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_ReportResult;
    ptp.Nparam = 2;
    ptp.Param1 = event_id;
    ptp.Param2 = result;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

uint16_t VitaMTP_GetUrl(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    int ret;
    
	PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_GetUrl;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
	ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &data, 0);
    if(ret != 0)
    {
        return ret;
    }
    // TODO: Parse XML
    return 0;
}

uint16_t VitaMTP_SendHttpObjectFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, int len, unsigned char *data)
{
    PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendHttpObjectFromURL;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    return ptp_transaction(params, &ptp, PTP_DP_SENDDATA, len, &data, 0);
}
