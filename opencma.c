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
struct cma_database *database;
char *uuid;
int ohfi_count;

void *vitaEventListener(LIBMTP_mtpdevice_t *device) {
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
                int ohfi = event.Param2; // what kind of items are we looking for?
                int unk1 = event.Param3; // TODO: what is this? all zeros from tests
                int items = countDatabase(ohfiToObject(ohfi));
                VitaMTP_SendNumOfObject(device, event_id, items);
                fprintf(stderr, "Returned count of %d objects\n", items);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadata:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event.Code, event_id);
                browse_info_t browse;
                struct cma_object *root;
                metadata_t *meta;
                VitaMTP_GetBrowseInfo(device, event_id, &browse);
                root = ohfiToObject(browse.ohfi);
                meta = root->metadata.next_metadata; // next of root is the list
                VitaMTP_SendObjectMetadata(device, event_id, meta);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event.Code, event_id);
                int ofc = 0; // TODO: What is OFC?
                uint32_t propnum;
                uint16_t *props = NULL;
                PTPObjectPropDesc propdesc;
                ptp_mtp_getobjectpropssupported((PTPParams*)device->params, ofc, &propnum, &props);
                for(int i = 0; i < propnum; i++) {
                    if(props[i] == PTP_OPC_ObjectFormat) {
                        ptp_mtp_getobjectpropdesc((PTPParams*)device->params, props[i], ofc, &propdesc);
                    }
                }
                free(props);
                break;
            case PTP_EC_VITA_RequestCancelTask:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event.Code, event_id);
                fprintf(stderr, "Event 0x%x unimplemented!\n", event.Code);
                // TODO: Cancel task, aka, move event processing to another thread so we can listen for this without blocking
                break;
            case PTP_EC_VITA_RequestSendHttpObjectFromURL:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event.Code, event_id);
                char *url = NULL;
                VitaMTP_GetUrl(device, event_id, &url);
                fprintf(stderr, "Request to download %s ignored.\n", url);
                fprintf(stderr, "Event 0x%x unimplemented!\n", event.Code);
                VitaMTP_SendHttpObjectFromURL(device, event_id, NULL, 0);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                free(url);
                break;
            case PTP_EC_VITA_RequestSendObjectStatus:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event.Code, event_id);
                object_status_t objectstatus;
                struct cma_object *object;
                VitaMTP_SendObjectStatus(device, event_id, &objectstatus);
                object = titleToObject(objectstatus.title, objectstatus.ohfiParent);
                free(objectstatus.title);
                if(object == NULL) {
                    VitaMTP_ReportResult(device, event_id, PTP_RC_VITA_ObjectNotFound);
                } else {
                    VitaMTP_SendObjectMetadata(device, event_id, &object->metadata);
                    VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                }
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
                settings_info_t settingsinfo;
                VitaMTP_GetSettingInfo(device, event_id, &settingsinfo);
                fprintf(stderr, "Current account id: %s\n", settingsinfo.current_account.accountId);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendHttpObjectPropFromURL:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectPropFromURL", event.Code, event_id);
                char *propurl = NULL;
                http_object_prop_t httpobjectprop;
                VitaMTP_GetUrl(device, event_id, &propurl);
                memset(&httpobjectprop, 0, sizeof(http_object_prop_t)); // Send null information
                fprintf(stderr, "Request to download %s prop ignored.\n", propurl);
                fprintf(stderr, "Event 0x%x unimplemented!\n", event.Code);
                VitaMTP_SendHttpObjectPropFromURL(device, event_id, &httpobjectprop);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                free(propurl);
                break;
            case PTP_EC_VITA_RequestSendPartOfObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendPartOfObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestOperateObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event.Code, event_id);
                operate_object_t operateobject;
                VitaMTP_OperateObject(device, event_id, &operateobject);
                free(operateobject.title);
                // do command, 1 = create folder for ex
                // refresh database
                break;
            case PTP_EC_VITA_RequestGetPartOfObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestSendStorageSize:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event.Code, event_id);
                int unk2 = event.Param2; // TODO: What is this, is set to 0x0A
                VitaMTP_SendStorageSize(device, event_id, (uint64_t)100*1024*1024*1024, (uint64_t)50*1024*1024*1024); // Send fake 50GB/100GB
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestCheckExistance: // [sic]
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event.Code, event_id);
                break;
            case PTP_EC_VITA_RequestGetTreatObject:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event.Code, event_id);
                treat_object_t treatobject;
                PTPObjectPropDesc opd;
                VitaMTP_GetTreatObject(device, event_id, &treatobject);
                // GetObjectPropDesc for abunch of values
                // GetObjPropList for folder
                // GetObjectHandles
                // GetObjPropList for each file
                // GetObject for each file
                break;
            case PTP_EC_VITA_RequestSendCopyConfirmationInfo:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendCopyConfirmationInfo", event.Code, event_id);
                // AFAIK, Sony hasn't even implemented this in their CMA
                fprintf(stderr, "Event 0x%x unimplemented!\n", event.Code);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadataItems:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadataItems", event.Code, event_id);
                VitaMTP_SendObjectMetadataItems(device, event_id, NULL);
                VitaMTP_SendObjectMetadata(device, event_id, NULL);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendNPAccountInfo:
                fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNPAccountInfo", event.Code, event_id);
                // AFAIK, Sony hasn't even implemented this in their CMA
                fprintf(stderr, "Event 0x%x unimplemented!\n", event.Code);
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
    
    // TODO: Create a uuid for each PSN account, as CMA does
    // The problem is that we need to connect to the Vita first to find the uuid
    // But creating the database after connecting is a waste of time
    uuid = strdup("ffffffffffffffff");
    
    // Start with some default values
    database = malloc(sizeof(struct cma_database));
    memset(database, 0, sizeof(struct cma_database)); // To make pointers null
    char cwd[FILENAME_MAX];
    getcwd(cwd, FILENAME_MAX);
    asprintf(&database->photos.path, "%s/%s", cwd, "photos");
    asprintf(&database->videos.path, "%s/%s", cwd, "videos");
    asprintf(&database->music.path, "%s/%s", cwd, "music");
    asprintf(&database->vitaApps.path, "%s/%s/%s/%s", cwd, "vita", uuid, "APP");
    asprintf(&database->pspApps.path, "%s/%s/%s/%s", cwd, "vita", uuid, "PGAME");
    asprintf(&database->pspSaves.path, "%s/%s/%s/%s", cwd, "vita", uuid, "PSAVEDATA");
    asprintf(&database->backups.path, "%s/%s/%s/%s", cwd, "vita", uuid, "SYSTEM");
    
    // Now get the arguments
    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "p:v:m:a:hd")) != -1) {
        switch (c) {
            case 'p': // photo path
                free(database->photos.path);
                database->photos.path = strdup(optarg);
                break;
            case 'v': // video path
                free(database->videos.path);
                database->videos.path = strdup(optarg);
                break;
            case 'm': // music path
                free(database->music.path);
                database->music.path = strdup(optarg);
                break;
            case 'a': // app path
                free(database->vitaApps.path);
                free(database->pspApps.path);
                free(database->pspSaves.path);
                free(database->backups.path);
                asprintf(&database->vitaApps.path, "%s/%s/%s", optarg, uuid, "APP");
                asprintf(&database->pspApps.path, "%s/%s/%s", optarg, uuid, "PGAME");
                asprintf(&database->pspSaves.path, "%s/%s/%s", optarg, uuid, "PSAVEDATA");
                asprintf(&database->backups.path, "%s/%s/%s", optarg, uuid, "SYSTEM");
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
    
    /* Set up the database */
    ohfi_count = OHFI_OFFSET; // This will be the id for each object. We won't reuse ids
    createDatabase();
    
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
    if(pthread_create(&event_thread, NULL, (void*(*)(void*))vitaEventListener, device) != 0) {
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
    free(database->photos.path);
    free(database->videos.path);
    free(database->music.path);
    free(database->vitaApps.path);
    free(database->pspApps.path);
    free(database->pspSaves.path);
    free(database->backups.path);
    destroyDatabase();
    free(database);
    
    fprintf(stderr, "Exiting...\n");
    
    return 0;
}
