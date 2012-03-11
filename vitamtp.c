//
//  vitamtp.c
//  VitaMTP
//
//  Created by Yifan Lu on 2/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#define _FILE_OFFSET_BITS 64

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

uint16_t VitaMTP_SendData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, void* data, unsigned int len){
	PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
	ptp.Code = code;
	ptp.Nparam = 1;
	ptp.Param1 = event_id;
    
	uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_SENDDATA, len, (unsigned char**)&data, 0);
	return ret;
}

uint16_t VitaMTP_GetData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, void* data, unsigned int* len){
	PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
	ptp.Code = code;
	ptp.Nparam = 1;
	ptp.Param1 = event_id;
    
	uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, (unsigned char**)&data, len);
    
	return ret;
}

uint16_t VitaMTP_SendInt32(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, uint32_t value){
	PTPParams *params = (PTPParams*)device->params;
	PTPContainer ptp;
    
	PTP_CNT_INIT(ptp);
	ptp.Code = code;
	ptp.Nparam = 2;
	ptp.Param1 = event_id;
	ptp.Param2 = value;
    
	return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
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
    unsigned int len;
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetSettingInfo, &info->raw_xml, &len);
    // TODO: Parse XML
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
    unsigned int len = 0;
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetUrl, &data, &len);
    // TODO: Parse XML
}

uint16_t VitaMTP_SendHttpObjectFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, int len, unsigned char *data)
{
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendHttpObjectFromURL, data, len);
}

uint16_t VitaMTP_SendStorageSize(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint64_t storage_size, uint64_t available_size){
    static const unsigned char padding[] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	static const int len = 25;
    uint16_t ret;
	uint64_t* data = malloc(len);
    data[0] = storage_size;
    data[1] = available_size;
    memcpy(&data[2], padding, sizeof(padding));
    ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendStorageSize, data, len);
	free(data);
	return ret;
}

uint16_t VitaMTP_SendNumOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t num){
	return VitaMTP_SendInt32(device, event_id, PTP_OC_VITA_SendNumOfObject, num);
}

uint16_t VitaMTP_GetBrowseInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, browse_info_t* info){
    unsigned int len = 0; // unused for now
	return VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetBrowseInfo, &info, &len);
}

uint16_t VitaMTP_SendObjectMetadata(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* metas){
    // TODO: Create XML
	uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectMetadata, metas->raw_xml, metas->raw_len);
	return ret;
}

uint16_t VitaMTP_SendObjectThumb(LIBMTP_mtpdevice_t *device, uint32_t event_id, thumbnail_t* thumb){
    // TODO: Create XML
	uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectThumb, thumb->raw_xml, thumb->raw_len);
	return ret;
}

uint16_t VitaMTP_SendObjectStatus(LIBMTP_mtpdevice_t *device, uint32_t event_id, object_status_t* status){
	int* data;
	unsigned int len = 0;
	uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendObjectStatus, &data, &len);
    status->p_data = data;
	status->type = data[0];
    status->size = data[1];
    status->file = (char*)&data[2];
	return ret;
}

uint16_t VitaMTP_SendPartOfObjectInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* send_init){
	return VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendPartOfObjectInit, &send_init, NULL);
}

uint16_t VitaMTP_SendPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init, metadata_t* meta){
	FILE* fp = fopen(meta->path, "rb");
    if(init->size > UINT32_MAX) // Because of libptp's length limits, we cannot be bigger than an int
        return PTP_RC_OperationNotSupported;
	if(!fp)
        return PTP_RC_AccessDenied;
	uint8_t* data = (uint8_t*)malloc((uint32_t)init->size + 8);
	fseeko(fp, init->offset, SEEK_SET);
	fread(&data[8], sizeof(char), init->size, fp);
	((uint64_t*)data)[0] = init->size;
	fclose(fp);
	uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendPartOfObject, data, (uint32_t)init->size + 8);
	free(data);
	return ret;
}
