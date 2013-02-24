//
//  Functions for handling MTP commands
//  VitaMTP
//
//  Created by Yifan Lu
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdlib.h>
#include <string.h>
#include "vitamtp.h"

/**
 * Global debug level
 *  All messages are printed to stderr
 *  DEBUG_LOG   All information printed 
 *  INFO_LOG    Most information printed
 *  WARNING_LOG Only warnings printed (default)
 *  ERROR_LOG   Only errors printed
 */
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

/**
 * Sends a command with a data send phase and a event id
 * 
 * The Vita uses the PC as a slave. It sends an event asking the PC to 
 * send a command, and it complies, sending the command with the event_id 
 * as its first parameter. Most commands only has this paramater, so this 
 * function it called internally to make our lives easier.
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param code the command to send.
 * @param data the array of data to send.
 * @param len the length of "data".
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_GetData()
 */
uint16_t VitaMTP_SendData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char* data, unsigned int len){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = code;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_SENDDATA, len, &data, 0);
    return ret;
}

/**
 * Sends a command with a data get phase and a event id
 * 
 * The Vita uses the PC as a slave. It sends an event asking the PC to 
 * send a command, and it complies, sending the command with the event_id 
 * as its first parameter. Most commands only has this paramater, so this 
 * function it called internally to make our lives easier.
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param code the command to send.
 * @param p_data a pointer to the char array to fill (dynamically allocated).
 * @param p_len a pointer to the length of the filled array.
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_GetData()
 */
uint16_t VitaMTP_GetData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** p_data, unsigned int* p_len){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = code;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, p_data, p_len);
    
    return ret;
}

/**
 * Called during initialization to get Vita information.
 * 
 * @param device a pointer to the device.
 * @param info a pointer to the vita_info struct to fill
 * @return the PTP result code that the Vita returns.
 */
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

/**
 * Sends the number of objects to list.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param num the number of objects to list.
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_SendObjectMetadata()
 */
uint16_t VitaMTP_SendNumOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t num){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendNumOfObject;
    ptp.Nparam = 2;
    ptp.Param1 = event_id;
    ptp.Param2 = num;
    
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

/**
 * Gets the filter for the kinds of object to show.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param info a pointer to the browse_info structure to fill.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_GetBrowseInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, browse_info_t* info){
    unsigned char *data = NULL;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetBrowseInfo, &data, NULL);
    memcpy(info, data, sizeof(browse_info_t));
    free(data);
    return ret;
}

/**
 * Sends a linked list of object metadata for the device to display.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param metas the first metadata in the linked list.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendObjectMetadata(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* metas){
    char *data;
    int len = 0;
    if(metadata_to_xml(metas, &data, &len) < 0)
        return PTP_RC_GeneralError;
    
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectMetadata, (unsigned char*)data, len);
    free(data);
    return ret;
}

/**
 * Sends thumbnail metadata and image data to the device.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param metas the metadata for the thumbnail. Should be only one.
 * @param thumb_data raw image data for the thumbnail.
 * @param thumb_len size of the image data.
 *  Currently, this cannot be larger than sizeof(uint32_t)
 * @return the PTP result code that the Vita returns.
 */
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
    
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendObjectThumb, (unsigned char*)new_data, (unsigned int)new_length); // TODO: Support huge thumbnails
    free(new_data);
    return ret;
}

/**
 * Called after each transaction to indicate that the PC is done
 * sending commands. You should call this after completing the 
 * request of a Vita MTP event, whether that takes one command or 
 * a series of them (some commands require initialization).
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param result a PTP result code to send
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_ReportResult(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint16_t result){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_ReportResult;
    ptp.Nparam = 2;
    ptp.Param1 = event_id;
    ptp.Param2 = result;
    
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

/**
 * Same as VitaMTP_ReportResult(), but also sends another integer
 * as a parameter.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param result a PTP result code to send
 * @param param a parameter to send
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_ReportResult()
 */
uint16_t VitaMTP_ReportResultWithParam(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint16_t result, uint32_t param){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_ReportResult;
    ptp.Nparam = 3;
    ptp.Param1 = event_id;
    ptp.Param2 = result;
    ptp.Param3 = param;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

/**
 * Called during initialization to send information abouthe PC.
 * 
 * @param device a pointer to the device.
 * @param info a pointer to the initiator_info structure.
 *  You should get this with new_initiator_info()
 * @return the PTP result code that the Vita returns.
 * @see new_initiator_info()
 */
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

/**
 * Gets a URL request from the Vita.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param url a pointer to the char array to be filled with the URL.
 *  Dynamically allocated and should be freed when done.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_GetUrl(LIBMTP_mtpdevice_t *device, uint32_t event_id, char** url){
    unsigned char *data;
    unsigned int len = 0;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetUrl, &data, &len);
    if(ret != PTP_RC_OK || len == 0){
        return ret;
    }
    int url_len = ((int*)data)[2]; // TODO: Figure out what data[0], data[1] are. They are always zero.
    *url = malloc(url_len);
    memcpy(*url, &((int*)data)[3], url_len);
    return ret;
}

/**
 * Sends the HTTP content from a URL request.
 * This should be called immediately after VitaMTP_GetUrl().
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param data the data from the HTTP response from the URL request.
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_GetUrl()
 */
uint16_t VitaMTP_SendHttpObjectFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, void *data, unsigned int len){
    unsigned char *buffer = malloc (len + sizeof (uint64_t));
    *(uint64_t*)buffer = len;
    memcpy (buffer + sizeof (uint64_t), data, len);
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendHttpObjectFromURL, buffer, len + sizeof (uint64_t));
    free (buffer);
    return ret;
}

/**
 * Unknown function.
 * Only known information is that data is being sent 
 * from the computer.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param data unknown.
 * @param len length of data.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendNPAccountInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len){ // TODO: Figure out data
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendNPAccountInfo, data, len);
}

/**
 * Gets information about the PSN account(s) on the device.
 * This function currently returns only junk data.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param info a pointer to the settings_info structure to fill.
 * @return the PTP result code that the Vita returns.
 */
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

/**
 * Gets the file to send metadata on.
 * VitaMTP_SendObjectMetadata() should be called afterwards.
 * The name is a misnomer, it doesn't send anything, but gets data.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param status a pointer to the object_status structure to fill.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendObjectStatus(LIBMTP_mtpdevice_t *device, uint32_t event_id, object_status_t* status){
    int* data;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendObjectStatus, (unsigned char**)&data, NULL);
    if (ret != PTP_RC_OK) {
        return ret;
    }
    status->ohfiRoot = data[0];
    status->len = data[1];
    status->title = malloc(status->len);
    memcpy(status->title, (char*)&data[2], status->len);
    free(data);
    return ret;
}

/**
 * Sends additional information on an HTTP object.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param prop a pointer to the http_object_prop structure to send.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendHttpObjectPropFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, http_object_prop_t *prop){
    int header_len = sizeof(http_object_prop_t) - sizeof(char*);
    unsigned char *data = malloc(header_len + prop->timestamp_len);
    memcpy(data, prop, header_len);
    memcpy(data + header_len, prop->timestamp, prop->timestamp_len);
    int ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendHttpObjectPropFromURL, data, header_len + prop->timestamp_len);
    free(data);
    return ret;
}

/**
 * Sends the PC client's status.
 * 
 * @param device a pointer to the device.
 * @param status the client's status
 * @return the PTP result code that the Vita returns.
 * @see VITA_HOST_STATUS
 */
uint16_t VitaMTP_SendHostStatus(LIBMTP_mtpdevice_t *device, uint32_t status){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_SendHostStatus;
    ptp.Nparam = 1;
    ptp.Param1 = status;
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

/**
 * Gets information on what part of the object to send.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param init a pointer to the send_part_init structure to fill.
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_SendPartOfObject()
 */
uint16_t VitaMTP_SendPartOfObjectInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init){
    unsigned char *data = NULL;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendPartOfObjectInit, &data, NULL);
    if (ret != PTP_RC_OK) {
        return ret;
    }
    memcpy(init, data, sizeof(send_part_init_t));
    free(data);
    return ret;
}

/**
 * Sends a part of the object. You should first call 
 * VitaMTP_SendPartOfObjectInit() to find out what to send.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param object_data an array containing the data to send.
 * @param object_len the size of that data to send.
 *  Currently, this cannot be larger than sizeof(uint32_t)
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_SendPartOfObjectInit()
 */
uint16_t VitaMTP_SendPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *object_data, uint64_t object_len){
    // TODO: Remove this code. Implementation should be left to developer for more flexability.
    /*
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
     */
    
    unsigned char *data;
    unsigned long len = object_len + sizeof(uint64_t);
    data = malloc(len);
    ((uint64_t*)data)[0] = object_len;
    memcpy(data + sizeof(uint64_t), object_data, object_len);
    
    uint16_t ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendPartOfObject, data, (int)len); // TODO: Support huge part of file
    free(data);
    return ret;
}

/**
 * Gets a command to perform an operation on an object.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param op_object an operate_object structure to fill with the infomation.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_OperateObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, operate_object_t* op_object){
    uint32_t len = 0;
    uint32_t* data = NULL;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_OperateObject, (unsigned char**)&data, &len);
    op_object->cmd = data[0];
    op_object->ohfiParent = data[1];
    op_object->unk1 = data[2];
    op_object->len = data[3];
    op_object->title = (char*)malloc(op_object->len+1);
    memcpy(op_object->title, (char*)&data[4], op_object->len+1);
    free(data);
    return ret;
}

/**
 * Gets a part of the object from the device.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param init a send_part_init_t struct to fill with object's info.
 * @param data a pointer to an array to fill with data (dynamically allocated)
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_GetPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init, unsigned char** data){
    unsigned char* _data = NULL;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetPartOfObject, (unsigned char**)&_data, NULL);
    memcpy(init, _data, sizeof(send_part_init_t));
    *data = malloc(init->size);
    memcpy(*data, _data + sizeof(send_part_init_t), init->size);
    free(_data);
    return ret;
}

/**
 * Sends the size of the current storage device on the PC.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param storage_size storage size in bytes.
 * @param available_size free space in bytes.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendStorageSize(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint64_t storage_size, uint64_t available_size){
    static const unsigned char padding[] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static const int len = 25;
    uint16_t ret;
    unsigned char* data = malloc(len);
    ((uint64_t*)data)[0] = storage_size;
    ((uint64_t*)data)[1] = available_size;
    memcpy(&((uint64_t*)data)[2], padding, sizeof(padding));
    ret = VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendStorageSize, data, len);
    free(data);
    return ret;
}

/**
 * Unknown function.
 * Only known information is that data is being obtained 
 * from the computer.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param treat unknown.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_GetTreatObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, treat_object_t* treat){
    unsigned char *data = NULL;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_GetTreatObject, &data, NULL);
    memcpy(treat, data, sizeof(treat_object_t));
    free(data);
    return ret;
}

/**
 * Unknown function.
 * Only known information is that data is being obtained 
 * from the computer.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param data unknown. Dynamically allocated.
 * @param len length of data.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendCopyConfirmationInfoInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char** data, unsigned int *len){ // TODO: Figure out data
    // No need to send ReportResult after calling this
    return VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendCopyConfirmationInfoInit, data, len);
}

/**
 * Unknown function.
 * Only known information is that data is being sent 
 * from the computer.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param data unknown.
 * @param len length of data.
 * @return the PTP result code that the Vita returns.
 */
uint16_t VitaMTP_SendCopyConfirmationInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len){ // TODO: Figure out data
    // Remember to send ReportResult after calling this
    return VitaMTP_SendData(device, event_id, PTP_OC_VITA_SendCopyConfirmationInfo, data, len);
}

/**
 * Gets the ohfi id of the object requested by the Vita
 * to send metadata on. VitaMTP_SendObjectMetadata() should be 
 * called afterwards.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 * @param ohfi a pointer to an integer that will be filled with the id.
 * @return the PTP result code that the Vita returns.
 * @see VitaMTP_SendObjectMetadata()
 */
uint16_t VitaMTP_SendObjectMetadataItems(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t *ohfi){
    uint32_t *p_ohfi;
    uint16_t ret = VitaMTP_GetData(device, event_id, PTP_OC_VITA_SendObjectMetadataItems, (unsigned char**)&p_ohfi, NULL);
    if (ret != PTP_RC_OK) {
        return ret;
    }
    *ohfi = *p_ohfi;
    return ret;
}

/**
 * Tell the Vita that the current event is being processed and not 
 * to time out on us.
 * 
 * @param device a pointer to the device.
 * @param event_id the unique ID sent by the Vita with the event.
 */
uint16_t VitaMTP_KeepAlive(LIBMTP_mtpdevice_t *device, uint32_t event_id){
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_VITA_KeepAlive;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    return ptp_transaction(params, &ptp, PTP_DP_NODATA, 0, NULL, 0);
}

/**
 * Gets a MTP properties list for an object
 * 
 * @param device a pointer to the device.
 * @param handle a object handle.
 * @param props a pointer to a MTPProperties array to be created.
 *  Dynamically allocated.
 * @param nrofprops number of elements in the props array.
 */
uint16_t VitaMTP_GetObjectPropList(LIBMTP_mtpdevice_t *device, uint32_t handle, MTPProperties** props, int* nrofprops){
    uint16_t ret;
    PTPParams *params = (PTPParams*)device->params;
    PTPContainer ptp;
    unsigned char* opldata = NULL;
    unsigned int oplsize;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = PTP_OC_MTP_GetObjPropList;
    ptp.Param1 = handle;
    ptp.Param2 = 0x00000000;
    ptp.Param3 = 0x00000000;
    ptp.Param4 = 0x00000001;
    ptp.Param5 = 0x00000000;
    ptp.Nparam = 5;
    ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, &opldata, &oplsize);  
    if(ret == PTP_RC_OK){
        *nrofprops = ptp_unpack_MTPProperties(params, opldata, props, oplsize);
    }
    if(opldata){
        free(opldata);
    }
    return ret;
}

/**
 * Sends a MTP object to the device. Size of the object and other 
 * information is found in the metadata.
 * 
 * @param device a pointer to the device.
 * @param p_parenthandle a pointer to the parent handle.
 * @param p_handle a pointer to the handle.
 * @param meta the metadata to describe the object.
 * @param data the object data to send.
 */
uint16_t VitaMTP_SendObject(LIBMTP_mtpdevice_t *device, uint32_t* p_parenthandle, uint32_t* p_handle, metadata_t* meta, unsigned char* data){
    uint32_t store = VITA_STORAGE_ID;
    uint16_t ret;
    PTPObjectInfo objectinfo;
    memset(&objectinfo, 0x0, sizeof(PTPObjectInfo));
    
    if(meta->dataType & Folder){
        objectinfo.ObjectFormat = PTP_OFC_Association; // 0x3001
        objectinfo.AssociationType = PTP_AT_GenericFolder;
        objectinfo.Filename = meta->path;
        ret = ptp_sendobjectinfo((PTPParams*)device->params, &store, p_parenthandle, p_handle, &objectinfo);
    }else if(meta->dataType & SaveData){
        objectinfo.ObjectFormat = PTP_OFC_PSPSave; // 0xB00A
        objectinfo.ObjectCompressedSize = (uint32_t)meta->size;
        objectinfo.CaptureDate = meta->dateTimeCreated;
        objectinfo.ModificationDate = meta->dateTimeCreated;
        objectinfo.Filename = meta->path;
        ret = ptp_sendobjectinfo((PTPParams*)device->params, &store, p_parenthandle, p_handle, &objectinfo);
        if (ret != PTP_RC_OK) {
            return ret;
        }
        ret = ptp_sendobject((PTPParams*)device->params, data, (uint32_t)meta->size);
    }
    return ret;
}

#if 0
/**
 * Gets a MTP object from the device.
 * 
 * @param device a pointer to the device.
 * @param handle the handle of the object.
 * @param p_meta a metadata structure to create for the object.
 *  Dynamically allocated.
 * @param p_data a pointer to the data to recieve.
 *  Dynamically allocated.
 */
void VitaMTP_GetObject(LIBMTP_mtpdevice_t *device, uint32_t handle, metadata_t** p_meta, unsigned char** p_data){
    MTPProperties* props = NULL;
    int nProps = 0;
    int i = 0;
    unsigned char* data = NULL;
    metadata_t* meta = NULL;
    
    VitaMTP_GetObjectPropList(device, handle, &props, &nProps);
    ptp_getobject((PTPParams*)device->params, handle, &data);
    
    meta = (metadata_t*)malloc(sizeof(metadata_t));
    memset(meta, 0x0, sizeof(metadata_t));
    for(i=0; i<nProps; i++){
        // [u16]property [u16]datatype [u32]handle data
        if(props[i].property == PTP_OPC_ObjectFormat){
            if(props[i].propval.u16 == PTP_OFC_Association){
                meta->dataType = Folder;
            }else{
                meta->dataType = File;
            }
        }
        if(meta->dataType == File){
            if(props[i].property == PTP_OPC_ObjectSize){
                meta->size = props[i].propval.u64;
            }
        }
        if(props[i].property == PTP_OPC_ObjectFileName){
            meta->title = malloc(strlen(props[i].propval.str)+1);
            meta->data.file.name = malloc(strlen(props[i].propval.str)+1);
            strcpy(meta->title, props[i].propval.str);
            strcpy(meta->data.file.name, props[i].propval.str);
            if(strchr(meta->title, '.')){
                strchr(meta->title, '.')[0] = 0x0;
            }
        }
        if(props[i].property == PTP_OPC_DateModified){
            meta->dateTimeCreated = ptp_unpack_MTPTime(props[i].propval.str);
        }
    }
    
    free(props);
    *p_meta = meta;
    *p_data = data;
}
#endif
