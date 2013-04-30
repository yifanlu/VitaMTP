//
//  Functions useful for connecting to USB
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmtp.h>
#include <ptp.h>
#include "vitamtp.h"

struct vita_device {
    LIBMTP_mtpdevice_t mtp_device;
};

/**
 * Get the first (as in "first in the list of") connected Vita MTP device.
 * @return a device pointer. NULL if error, no connected device, or no connected Vita
 * @see LIBMTP_Get_Connected_Devices()
 */
vita_device_t *LIBVitaMTP_Get_First_Vita(void){
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
    return (vita_device_t*)first_device; // NULL if no vita
}

/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBVitaMTP_Release_Device(vita_device_t *device){
    LIBMTP_Release_Device (&device->mtp_device);
}

/**
 * To read events sent by the device, repeatedly call this function from a secondary
 * thread until the return value is < 0.
 *
 * @param device a pointer to the MTP device to poll for events.
 * @param event contains a pointer to be filled in with the event retrieved if the call
 * is successful.
 * @return 0 on success, any other value means the polling loop shall be
 * terminated immediately for this session.
 */
int LIBVitaMTP_Read_Event(vita_device_t *device, vita_event_t *event){
    /*
     * FIXME: Potential race-condition here, if client deallocs device
     * while we're *not* waiting for input. As we'll be waiting for
     * input most of the time, it's unlikely but still worth considering
     * for improvement. Also we cannot affect the state of the cache etc
     * unless we know we are the sole user on the device. A spinlock or
     * mutex in the LIBMTP_mtpdevice_t is needed for this to work.
     */
    PTPParams *params = (PTPParams *)device->mtp_device.params;
    PTPContainer ptp_event;
    memset(&ptp_event, 0, sizeof(PTPContainer)); // zero it so params are zeroed too
    uint16_t ret = ptp_usb_event_wait(params, &ptp_event);
    
    memcpy(event, &ptp_event, sizeof(LIBMTP_event_t));
    
    if (ret != PTP_RC_OK) {
        /* Device is closing down or other fatal stuff, exit thread */
        return -1;
    }
    
    return 0;
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
uint16_t VitaMTP_SendData(vita_device_t *device, uint32_t event_id, uint32_t code, unsigned char* data, unsigned int len){
    PTPParams *params = (PTPParams*)device->mtp_device.params;
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
uint16_t VitaMTP_GetData(vita_device_t *device, uint32_t event_id, uint32_t code, unsigned char** p_data, unsigned int* p_len){
    PTPParams *params = (PTPParams*)device->mtp_device.params;
    PTPContainer ptp;
    
    PTP_CNT_INIT(ptp);
    ptp.Code = code;
    ptp.Nparam = 1;
    ptp.Param1 = event_id;
    
    uint16_t ret = ptp_transaction(params, &ptp, PTP_DP_GETDATA, 0, p_data, p_len);
    
    return ret;
}
