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

int log_mask = DEBUG_LOG;

/**
 * Get the first (as in "first in the list of") connected Vita MTP device.
 * @return a device pointer. NULL if error, no connected device, or no connected Vita
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBVitaMTP_Get_First_Vita(void){
    LIBMTP_mtpdevice_t *first_device = NULL;
    LIBMTP_raw_device_t *devices;
    int numdevs;
    int i;
    LIBMTP_error_number_t ret;
    
    ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
    if (ret != LIBMTP_ERROR_NONE){
        return NULL;
    }
    
    if (devices == NULL || numdevs == 0){
        return NULL;
    }
    
    for (i = 0; i < numdevs; i++){
        if(devices[i].device_entry.vendor_id == VITA_VID &&
           devices[i].device_entry.product_id == VITA_PID){
            first_device = LIBMTP_Open_Raw_Device(&devices[i]);
            break;
        }
        LIBMTP_Release_Device(first_device);
    }
    
    
    free(devices);
    return first_device; // NULL if no vita
}

uint16_t VitaMTP_SendData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int len){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = code;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_SENDDATA, len, data, 0);
    return ret;
}

uint16_t VitaMTP_GetData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int* len){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = code;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, data, len);
    
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

uint16_t VitaMTP_GetVitaInfo(LIBMTP_mtpdevice_t *device, vita_info_t *info){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    int ret;
    unsigned char *data;
    unsigned int len;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_GetVitaInfo;
    ptp.Nparam = 0;
    ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &data, &len);
    if(ret != PTP_RC_OK || len == 0){
        return ret;
    }
    if(vita_info_from_xml(info, (char*)data+sizeof(uint32_t), len-sizeof(uint32_t)) != 0){ // strip header
        return PTP_RC_GeneralError;
    }
    free(data);
    return ret;
}

uint16_t VitaMTP_SendNumOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t num){
    return VitaMTP_SendInt32(device, event_id, PTP_OC_VITA_SendNumOfObject, num);
}

uint16_t VitaMTP_GetBrowseInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, browse_info_t* info){
    unsigned int len = 0; // unused for now
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetBrowseInfo, (unsigned char**)&info, &len);
}

uint16_t VitaMTP_SendObjectMetadata(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* metas){
    char *data;
    int len = 0;
    if(metadata_to_xml(metas, &data, &len) < 0)
        return PTP_RC_GeneralError;
    
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectMetadata, (unsigned char**)&data, len);
    free(data);
    return ret;
}

uint16_t VitaMTP_SendObjectThumb(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* meta, unsigned char* thumb_data, uint64_t thumb_len){
    char *data;
    int len = 0;
    if(metadata_to_xml(meta, &data, &len) < 0)
        return PTP_RC_GeneralError;
    
    long new_length = len + sizeof(uint64_t) + thumb_len;
    char *new_data = malloc(new_length);
    memcpy(new_data, data, len);
    memcpy(new_data + len, &thumb_len, sizeof(uint64_t));
    memcpy(new_data + len + sizeof(uint64_t), thumb_data, thumb_len);
    free(data);
    
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectThumb, (unsigned char**)&new_data, (unsigned int)new_length); // TODO: Support huge thumbnails
    free(new_data);
    return ret;
}

uint16_t VitaMTP_ReportResult(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint16_t result){
    return VitaMTP_SendInt32(device, event_id, PTP_OC_VITA_ReportResult, result);
}

uint16_t VitaMTP_SendInitiatorInfo(LIBMTP_mtpdevice_t *device, initiator_info_t *info){
    char *data;
    int len = 0;
    if(initiator_info_to_xml(info, &data, &len) < 0)
        return PTP_RC_GeneralError;
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendInitiatorInfo;
    ptp.Nparam = 0;
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_SENDDATA, len, (unsigned char**)&data, 0); // plus one for null terminator, which is required on the vita's side
    free(data);
    return ret;
}

uint16_t VitaMTP_GetUrl(LIBMTP_mtpdevice_t *device, uint32_t event_id, char **url){
    unsigned char *data;
    unsigned int len = 0;
    uint8_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetUrl, &data, &len);
    if(ret != PTP_RC_OK || len == 0){
        return ret;
    }
    int url_len = ((int*)data)[2]; // TODO: Figure out what data[0], data[1] are. They are always zero.
    *url = malloc(url_len);
    memcpy(*url, &((int*)data)[3], url_len);
    return ret;
}

uint16_t VitaMTP_SendHttpObjectFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, void *data, unsigned int len){
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendHttpObjectFromURL, (unsigned char**)&data, len);
}

uint16_t VitaMTP_SendNPAccountInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len){ // TODO: Figure out data
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendNPAccountInfo, &data, len);
}

uint16_t VitaMTP_GetSettingInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, settings_info_t *info){
    unsigned char *data;
    unsigned int len;
    uint32_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetSettingInfo, &data, &len);
    if(ret != PTP_RC_OK || len == 0){
        return ret;
    }
    if(settings_info_from_xml(info, (char*)data+sizeof(uint32_t), len-sizeof(uint32_t)) != 0){ // strip header
        return PTP_RC_GeneralError;
    }
    free(data);
    return ret;
}

uint16_t VitaMTP_SendObjectStatus(LIBMTP_mtpdevice_t *device, uint32_t event_id, object_status_t* status){
    int* data;
    unsigned int len = 0;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendObjectStatus, (unsigned char**)&data, &len);
    status->p_data = data;
    status->type = data[0];
    status->size = data[1];
    status->file = (char*)&data[2];
    return ret;
}

uint16_t VitaMTP_SendHttpObjectPropFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, http_object_prop_t *prop){
    int header_len = sizeof(http_object_prop_t) - sizeof(char*);
    unsigned char *data = malloc(header_len + prop->timestamp_len);
    memcpy(data, prop, header_len);
    memcpy(data + header_len, prop->timestamp, prop->timestamp_len);
    int ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendHttpObjectPropFromURL, &data, header_len + prop->timestamp_len);
    free(data);
    return ret;
}

uint16_t VitaMTP_SendHostStatus(LIBMTP_mtpdevice_t *device, uint32_t status){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendHostStatus;
    ptp.Nparam = 1;
    ptp.Param1 = status;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

uint16_t VitaMTP_SendPartOfObjectInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init){
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendPartOfObjectInit, (unsigned char**)&init, NULL);
}

uint16_t VitaMTP_SendPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init, metadata_t* meta){
    FILE* fp = fopen(meta->path, "rb");
    if(init->size > UINT32_MAX) // Because of libptp's length limits, we cannot be bigger than an int
        return PTP_RC_OperationNotSupported; // TODO: Fix it so we can have large lengths
    if(!fp)
        return PTP_RC_AccessDenied;
    unsigned char* data = malloc((uint32_t)init->size + sizeof(uint64_t));
    fseeko(fp, init->offset, SEEK_SET);
    fread(&data[sizeof(uint64_t)], sizeof(char), init->size, fp);
    ((uint64_t*)data)[0] = init->size;
    fclose(fp);
    uint8_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendPartOfObject, &data, (uint32_t)init->size + sizeof(uint64_t));
    free(data);
    return ret;
}

uint16_t VitaMTP_OperateObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len){ // TODO: Figure out data
    // Remember to send ReportResult after calling this
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_OperateObject, data, len);
}

uint16_t VitaMTP_SendStorageSize(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint64_t storage_size, uint64_t available_size){
    static const unsigned char padding[] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static const int len = 25;
    uint16_t ret;
    unsigned char* data = malloc(len);
    ((uint64_t*)data)[0] = storage_size;
    ((uint64_t*)data)[1] = available_size;
    memcpy(&((uint64_t*)data)[2], padding, sizeof(padding));
    ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendStorageSize, &data, len);
    free(data);
    return ret;
}

uint16_t VitaMTP_GetTreatObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len){ // TODO: Figure out data
    // Remember to send ReportResult after calling this
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetTreatObject, data, len);
}

uint16_t VitaMTP_SendCopyConfirmationInfoInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len){ // TODO: Figure out data
    // No need to send ReportResult after calling this
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendCopyConfirmationInfoInit, data, len);
}

uint16_t VitaMTP_SendCopyConfirmationInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len){ // TODO: Figure out data
    // Remember to send ReportResult after calling this
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendCopyConfirmationInfo, &data, len);
}

uint16_t VitaMTP_SendObjectMetadataItems(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t *ofhi){
    uint32_t *p_ofhi;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendObjectMetadataItems, (unsigned char**)&p_ofhi, NULL);
    *ofhi = *p_ofhi;
    return ret;
}

uint16_t VitaMTP_KeepAlive(LIBMTP_mtpdevice_t *device, uint32_t event_id){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_KeepAlive;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

