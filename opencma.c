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

#include <limits.h>
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

static const metadata_t thumbmeta = {0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {18, 144, 80, 0, 1, 1.0f, 2}, NULL};

static const vita_event_process_t event_processes[] = {
    vitaEventSendNumOfObject,
    vitaEventSendObjectMetadata,
    vitaEventUnimplementated, 
    vitaEventSendObject, 
    vitaEventCancelTask,
    vitaEventUnimplementated, 
    vitaEventUnimplementated,
    vitaEventSendHttpObjectFromURL,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventSendObjectStatus, 
    vitaEventSendObjectThumb, 
    vitaEventDeleteObject, 
    vitaEventGetSettingInfo, 
    vitaEventSendHttpObjectPropFromURL,
    vitaEventUnimplementated, 
    vitaEventSendPartOfObject,
    vitaEventUnimplementated, 
    vitaEventOperateObject, 
    vitaEventGetPartOfObject, 
    vitaEventSendStorageSize, 
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventUnimplementated,
    vitaEventCheckExistance, 
    vitaEventUnimplementated, 
    vitaEventGetTreatObject, 
    vitaEventSendCopyConfirmationInfo, 
    vitaEventSendObjectMetadataItems, 
    vitaEventSendNPAccountInfo, 
    vitaEventRequestTerminate, 
    vitaEventUnimplementated
};

void vitaEventSendNumOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNumOfObject", event->Code, eventId);
    uint32_t ohfi = event->Param2; // what kind of items are we looking for?
    int unk1 = event->Param3; // TODO: what is this? all zeros from tests
    int items = filterObjects (ohfi, NULL);
    VitaMTP_SendNumOfObject(device, eventId, items);
    fprintf(stderr, "Returned count of %d objects\n", items);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendObjectMetadata (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event->Code, eventId);
    browse_info_t browse;
    metadata_t *meta;
    VitaMTP_GetBrowseInfo(device, eventId, &browse);
    if (filterObjects (browse.ohfiParent, &meta) == 0) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    VitaMTP_SendObjectMetadata(device, eventId, meta); // send all objects with OHFI parent
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event->Code, eventId);
    uint32_t parentHandle = event->Param3;
}

void vitaEventCancelTask (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event->Code, eventId);
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
    // TODO: Cancel task, aka, move event processing to another thread so we can listen for this without blocking
}

void vitaEventSendHttpObjectFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event->Code, eventId);
    char *url = NULL;
    VitaMTP_GetUrl(device, eventId, &url);
    FILE *file = fopen ("/Users/yifanlu/Downloads/Downloads/psp2-updatelist.xml", "r");
    fseek (file, 0, SEEK_END);
    uint32_t len = (uint32_t)ftell (file);
    fseek (file, 0, SEEK_SET);
    char *data = malloc (len + sizeof(uint64_t));
    fread (data+sizeof(uint64_t), 1, len, file);
    fclose (file);
    *(uint64_t*)data = len;
    VitaMTP_SendHttpObjectFromURL(device, eventId, data, len+sizeof(uint64_t));
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    free(url);
    free (data);
}

void vitaEventSendObjectStatus (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event->Code, eventId);
    object_status_t objectstatus;
    struct cma_object *object;
    VitaMTP_SendObjectStatus(device, eventId, &objectstatus);
    object = titleToObject(objectstatus.title, objectstatus.ohfiRoot);
    free(objectstatus.title);
    if(object == NULL) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }
    metadata_t *metadata = &object->metadata;
    metadata->next_metadata = NULL;
    VitaMTP_SendObjectMetadata(device, eventId, metadata);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendObjectThumb (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectThumb", event->Code, eventId);
    uint32_t ohfi = event->Param2;
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }
    char thumbpath[PATH_MAX];
    thumbpath[0] = '\0';
    sprintf (thumbpath, "%s/%s", object->path, "ICON0.PNG");
    FILE *file = fopen (thumbpath, "r");
    if (file == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    fseek (file, 0, SEEK_END);
    uint32_t length = (uint32_t)ftell (file);
    fseek (file, 0, SEEK_SET);
    unsigned char *data = malloc (length);
    if (fread (data, sizeof (char), length, file) < length) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        free (data);
        fclose (file);
        return;
    }
    // TODO: Get thumbnail data correctly
    VitaMTP_SendObjectThumb (device, eventId, (metadata_t *)&thumbmeta, data, length);
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    free (data);
    fclose (file);
}

void vitaEventDeleteObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestDeleteObject", event->Code, eventId);
}

void vitaEventGetSettingInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetSettingInfo", event->Code, eventId);
    settings_info_t settingsinfo;
    VitaMTP_GetSettingInfo(device, eventId, &settingsinfo);
    fprintf(stderr, "Current account id: %s\n", settingsinfo.current_account.accountId);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendHttpObjectPropFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectPropFromURL", event->Code, eventId);
    char *propurl = NULL;
    http_object_prop_t httpobjectprop;
    VitaMTP_GetUrl(device, eventId, &propurl);
    memset(&httpobjectprop, 0, sizeof(http_object_prop_t)); // Send null information
    fprintf(stderr, "Request to download %s prop ignored.\n", propurl);
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
    VitaMTP_SendHttpObjectPropFromURL(device, eventId, &httpobjectprop);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    free(propurl);
}

void vitaEventSendPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendPartOfObject", event->Code, eventId);
    send_part_init_t part_init;
    if (VitaMTP_SendPartOfObjectInit (device, eventId, &part_init) != PTP_RC_OK) {
        return;
    }
    struct cma_object *object = ohfiToObject (part_init.ohfi);
    if (object == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }
    FILE *file = fopen (object->path, "r");
    if (file == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    if (fseek (file, part_init.offset, SEEK_SET) < 0) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        fclose (file);
        return;
    }
    unsigned char *data = malloc (part_init.size);
    if (fread (data, sizeof (char), part_init.size, file) < part_init.size) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Cannot_Read_Info);
        fclose (file);
        free (data);
        return;
    }
    VitaMTP_SendPartOfObject (device, eventId, data, part_init.size);
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
}

void vitaEventOperateObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event->Code, eventId);
    operate_object_t operateobject;
    VitaMTP_OperateObject(device, eventId, &operateobject);
    free(operateobject.title);
    // do command, 1 = create folder for ex
    // refresh database
}

void vitaEventGetPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event->Code, eventId);
}

void vitaEventSendStorageSize (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event->Code, eventId);
    int unk2 = event->Param2; // TODO: What is this, is set to 0x0A
    VitaMTP_SendStorageSize(device, eventId, (uint64_t)100*1024*1024*1024, (uint64_t)50*1024*1024*1024); // Send fake 50GB/100GB
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventCheckExistance (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) { // [sic]
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event->Code, eventId);
}

void vitaEventGetTreatObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event->Code, eventId);
    treat_object_t treatobject;
    PTPObjectPropDesc opd;
    VitaMTP_GetTreatObject(device, eventId, &treatobject);
    // GetObjectPropDesc for abunch of values
    // GetObjPropList for folder
    // GetObjectHandles
    // GetObjPropList for each file
    // GetObject for each file
}

void vitaEventSendCopyConfirmationInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendCopyConfirmationInfo", event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventSendObjectMetadataItems (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadataItems", event->Code, eventId);
    uint32_t ohfi;
    VitaMTP_SendObjectMetadataItems(device, eventId, &ohfi);
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Context);
    }
    metadata_t *metadata = &object->metadata;
    metadata->next_metadata = NULL;
    VitaMTP_SendObjectMetadata(device, eventId, metadata);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendNPAccountInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNPAccountInfo", event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventRequestTerminate (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestTerminate", event->Code, eventId);
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventUnimplementated (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Unknown event not handled, code: 0x%x, id: %d\n", event->Code, eventId);
}

void *vitaEventListener(LIBMTP_mtpdevice_t *device) {
    LIBMTP_event_t event;
    int slot;
    while(event_listen) {
        if(LIBMTP_Read_Event(device, &event) < 0) {
            fprintf(stderr, "Error recieving event.\n");
            break;
        }
        slot = event.Code - PTP_EC_VITA_RequestSendNumOfObject;
        if (slot < 0 || slot > sizeof (event_processes)/sizeof (void*)) {
            slot = sizeof (event_processes)/sizeof (void*); // last item is pointer to "unimplemented
        }
        event_processes[slot] (device, &event, event.Param1);
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
    asprintf(&database->vitaApps.path, "%s/%s/%s/%s", cwd, "vita", "APP", uuid);
    asprintf(&database->pspApps.path, "%s/%s/%s/%s", cwd, "vita", "PGAME", uuid);
    asprintf(&database->pspSaves.path, "%s/%s/%s/%s", cwd, "vita", "PSAVEDATA", uuid);
    asprintf(&database->backups.path, "%s/%s/%s/%s", cwd, "vita", "SYSTEM", uuid);
    
    // Show help string
    fprintf(stderr, "OpenCMA Version %s\nlibVitaMTP Version: %d.%d\nVita Protocol Version: %08d\n",
            OPENCMA_VERSION_STRING, VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR, VITAMTP_PROTOCOL_VERSION);
    
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
                asprintf(&database->vitaApps.path, "%s/%s/%s", optarg, "APP", uuid);
                asprintf(&database->pspApps.path, "%s/%s/%s", optarg, "PGAME", uuid);
                asprintf(&database->pspSaves.path, "%s/%s/%s", optarg, "PSAVEDATA", uuid);
                asprintf(&database->psxApps.path, "%s/%s/%s", optarg, "PSGAME", uuid);
                asprintf(&database->psmApps.path, "%s/%s/%s", optarg, "PSM", uuid);
                asprintf(&database->backups.path, "%s/%s/%s", optarg, "SYSTEM", uuid);
                break;
            case 'd': // start as daemon
                // TODO: What to do if we're a daemon
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr, "%s\n", HELP_STRING);
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
    fprintf (stderr, "Waiting for Vita to connect...\n");
    do {
        sleep(10);
        // This will do MTP initialization if the device is found
        device = LIBVitaMTP_Get_First_Vita();
    } while (device == NULL);
    fprintf (stderr, "Vita connected: serial %s\n", LIBMTP_Get_Serialnumber (device));
    
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
    
    // End this connection with the Vita
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
