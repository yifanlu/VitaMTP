//
//  Reference implementation of libvitamtp
//  This should be a complete, but simple implementation
//  OpenCMA
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vitamtp.h>

#include "opencma.h"

int event_listen;
struct cma_paths *paths;

void *vita_event_listener(LIBMTP_mtpdevice_t *device) {
    LIBMTP_event_t event;
    int event_id;
    while(event_listen) {
        if(LIBMTP_Read_Event(device, &event) < 0) {
            fprintf(stderr, "Error recieving event.\n");
        }
        event_id = event.Param1;
        fprintf(stderr, "Event: 0x%x id %d\n", event.Code, event_id);
        switch(event.Code) {
            case PTP_EC_VITA_RequestSendNumOfObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNumOfObject", event.Code, event_id);
                VitaMTP_SendNumOfObject(device, event_id, 1);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadata:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event.Code, event_id);
                VitaMTP_GetBrowseInfo(device, event_id, NULL);
                VitaMTP_SendObjectMetadata(device, event_id, NULL);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestCancelTask:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendHttpObjectFromURL:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event.Code, event_id);
                VitaMTP_GetUrl(device, event_id, NULL);
                VitaMTP_SendHttpObjectFromURL(device, event_id, NULL, 0);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectStatus:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event.Code, event_id);
                VitaMTP_SendObjectStatus(device, event_id, NULL);
                VitaMTP_SendObjectMetadata(device, event_id, NULL);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectThumb:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectThumb", event.Code, event_id);
                VitaMTP_SendObjectThumb(device, event_id, NULL, NULL, 0);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestDeleteObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestDeleteObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestGetSettingInfo:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetSettingInfo", event.Code, event_id);
                VitaMTP_GetSettingInfo(device, event_id, NULL);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendHttpObjectPropFromURL:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectPropFromURL", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendPartOfObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendPartOfObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestOperateObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestGetPartOfObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendStorageSize:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event.Code, event_id);
                VitaMTP_SendStorageSize(device, event_id, (uint64_t)100*1024*1024*1024, (uint64_t)50*1024*1024*1024); // Send fake 50GB/100GB
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestCheckExistance: // [sic]
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestGetTreatObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendCopyConfirmationInfo:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendCopyConfirmationInfo", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadataItems:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadataItems", event.Code, event_id);
                VitaMTP_SendObjectMetadataItems(device, event_id, NULL);
                VitaMTP_SendObjectMetadata(device, event_id, NULL);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendNPAccountInfo:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNPAccountInfo", event.Code, event_id);
                break;
            case PTP_EC_VITA_Unknown1:
            default:
                fprintf(stderr, "Unknown event not handled, code: 0x%x, id: %d\n", event.Code, event_id);
                break;
        }
    }
    
    return NULL;
}

int main(int argc, char** argv) {
    /* First we will parse the command line arguments */
    
    // Start with some default values
    // We use strdup() in case the main thread exits before listener thread 
    // This never happens in this example, but is here for your reference
    paths = malloc(sizeof(struct cma_paths));
    paths->photoPath = strdup("./photos");
    paths->videoPath = strdup("./videos");
    paths->musicPath = strdup("./music");
    paths->appPath = strdup("./vita");
    
    // Now get the arguments
    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "p:v:m:a:hd")) != -1) {
        switch (c) {
            case 'p': // photo path
                free(paths->photoPath);
                paths->photoPath = strdup(optarg);
                break;
            case 'v': // video path
                free(paths->videoPath);
                paths->videoPath = strdup(optarg);
                break;
            case 'm': // music path
                free(paths->musicPath);
                paths->musicPath = strdup(optarg);
                break;
            case 'a': // app path
                free(paths->appPath);
                paths->appPath = strdup(optarg);
                break;
            case 'd': // start as daemon
                // TODO: What to do if we're a daemon
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr, "OpenCMA Version %s\nlibVitaMTP Version: %d.%d\nVita Protocol Version: %d\n%s\n", 
                        OPENCMA_VERSION_STRING, VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR, VITAMTP_PROTOCOL_VERSION, HELP_STRING);
                break;
        }
    }
    
    /* Now, we can set up the device */
    
    // This lets us have detailed logs including dumps of MTP packets
    LIBMTP_Set_Debug(9);
    
    // This must be called to initialize libmtp 
    LIBMTP_Init();
    
    LIBMTP_mtpdevice_t *device;
    
    // Wait for the device to be plugged in
    do {
        sleep(10);
        // This will do MTP initialization if the device is found
        device = LIBVitaMTP_Get_First_Vita();
    } while (device == NULL);
    
    // Create the event listener thread, technically we do not
    // need a seperate thread to do this since the main thread 
    // is not doing anything else, but to make the example 
    // complete, we will assume the main thread has more 
    // important things.
    pthread_t event_thread;
    event_listen = 1;
    if(pthread_create(&event_thread, NULL, (void*(*)(void*))vita_event_listener, device) != 0) {
        fprintf(stderr, "Cannot create event listener thread.\n");
        return 1;
    }
    
    // Here we will do Vita specific initialization
    vita_info_t vita_info;
    // This will automatically fill pc_info with default information
    const initiator_info_t *pc_info = new_initiator_info();
    
    // First, we get the Vita's info
    VitaMTP_GetVitaInfo(device, &vita_info);
    // Next, we send the client's (this program) info (discard the const here)
    VitaMTP_SendInitiatorInfo(device, (initiator_info_t*)pc_info);
    // Finally, we tell the Vita we are connected
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_Connected);
    // We do not need the client's info anymore, so free it to prevent memory leaks
    // Do not use this function if you manually created the initiator_info.
    // This will only work with ones created from new_initiator_info()
    free_initiator_info(pc_info);
    
    // Keep this thread idle until it is time to shut down the client
    // In a more complex application, this thread is free to do other work.
    while(event_listen) {
        // event_listen should be 0 when the listener thread closes
        sleep(60);
    }
    
    // Tell the Vita it's not him, but it's us and that it's over
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
    
    // Clean up our mess
    LIBMTP_Release_Device(device);
    free(paths->photoPath);
    free(paths->videoPath);
    free(paths->musicPath);
    free(paths->appPath);
    free(paths);
    
    fprintf(stderr, "Exiting...\n");
    
    return 0;
}
