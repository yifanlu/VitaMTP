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

#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vitamtp.h>

#include "opencma.h"

int event_listen;
struct cma_database *database;
char *uuid;
int ohfi_count;

static const char *HELP_STRING =
"usage: opencma [options]\n"
"   options\n"
"       -p path     Path to photos\n"
"       -v path     Path to videos\n"
"       -m path     Path to music\n"
"       -a path     Path to apps\n"
"       -d          Start as a daemon (not implemented)\n"
"       -h          Show this help text\n";

static const metadata_t thumbmeta = {0, 0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {18, 144, 80, 0, 1, 1.0f, 2}, NULL};

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



static inline void incrementSizeMetadata (struct cma_object *object, size_t size) {
    do {
        object->metadata.size += size;
    } while (object->metadata.ohfiParent > 0 && (object = ohfiToObject (object->metadata.ohfiParent)) != NULL);
}

void vitaEventSendNumOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNumOfObject", event->Code, eventId);
    uint32_t ohfi = event->Param2; // what kind of items are we looking for?
    //int unk1 = event->Param3; // TODO: what is this? all zeros from tests
    int items = filterObjects (ohfi, NULL);
    VitaMTP_SendNumOfObject(device, eventId, items);
    fprintf(stderr, "Returned count of %d objects for OHFI parent %d\n", items, ohfi);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendObjectMetadata (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event->Code, eventId);
    browse_info_t browse;
    metadata_t *meta;
    VitaMTP_GetBrowseInfo(device, eventId, &browse);
    filterObjects (browse.ohfiParent, &meta); // if meta is null, will return empty XML
    VitaMTP_SendObjectMetadata(device, eventId, meta); // send all objects with OHFI parent
    fprintf(stderr, "Sent metadata for OHFI parent %d\n", browse.ohfiParent);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event->Code, eventId);
    uint32_t ohfi = event->Param2;
    uint32_t parentHandle = event->Param3;
    uint32_t handle;
    struct cma_object *object = ohfiToObject (ohfi);
    struct cma_object *start = object;
    struct cma_object *temp = NULL;
    if (object == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    unsigned char *data = NULL;
    unsigned int len = 0;
    do {
        len = (unsigned int)object->metadata.size;
        // read the file to send if it's not a directory
        // if it is a directory, data and len are not used by VitaMTP
        if (object->metadata.dataType & File) {
            if (readFileToBuffer (object->path, 0, &data, &len) < 0) {
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
                return;
            }
        }
        
        // get the PTP object ID for the parent to put the object
        // we know the parent has to be before the current node
        // the first time this is called, parentHandle is left untouched
        for (temp = start; temp != NULL && temp != object; temp = temp->next_object) {
            if (temp->metadata.ohfi == object->metadata.ohfiParent) {
                parentHandle = temp->metadata.handle;
                break;
            }
        }
        
        // send the data over
        fprintf (stderr, "Sending OHFI %d to handle 0x%08X\n", ohfi, parentHandle);
        if (VitaMTP_SendObject (device, &parentHandle, &handle, &object->metadata, data) != PTP_RC_OK) {
            VitaMTP_ReportResult (device, eventId, PTP_RC_IncompleteTransfer);
            fprintf (stderr, "Sending of %s failed.\n", object->metadata.name);
            free (data);
            return;
        }
        object->metadata.handle = handle;
        object = object->next_object;
        
        free (data);
    }while (object != NULL && object->metadata.ohfiParent >= OHFI_OFFSET); // get everything under this "folder"
    // TODO: Find out why this always fails at the end "incomplete"
    VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, parentHandle);
}

void vitaEventCancelTask (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event->Code, eventId);
    fprintf(stderr, "Event 0x%x unimplemented!\n", event->Code);
    // TODO: Cancel task, aka, move event processing to another thread so we can listen for this without blocking
}

void vitaEventSendHttpObjectFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event->Code, eventId);
    char *url = NULL;
    if (VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Download);
        return;
    }
    free (url);
    unsigned char *data;
    unsigned int len = 0;
    if (readFileToBuffer ("/Users/yifanlu/Downloads/Downloads/psp2-updatelist.xml", 0, &data, &len) < 0) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Download);
        return;
    }
    VitaMTP_SendHttpObjectFromURL(device, eventId, data, len);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    free (data);
}

void vitaEventSendObjectStatus (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event->Code, eventId);
    object_status_t objectstatus;
    struct cma_object *object;
    if (VitaMTP_SendObjectStatus(device, eventId, &objectstatus) != PTP_RC_OK) {
        return;
    }
    object = titleToObject(objectstatus.title, objectstatus.ohfiRoot);
    if(object == NULL) { // not in database, don't return metadata
        fprintf (stderr, "Object %s not in database.\n", objectstatus.title);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    } else {
        metadata_t *metadata = &object->metadata;
        metadata->next_metadata = NULL;
        VitaMTP_SendObjectMetadata(device, eventId, metadata);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    free(objectstatus.title);
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
    unsigned char *data;
    unsigned int len = 0;
    if (readFileToBuffer (thumbpath, 0, &data, &len) < 0) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    // TODO: Get thumbnail data correctly
    VitaMTP_SendObjectThumb (device, eventId, (metadata_t *)&thumbmeta, data, len);
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    free (data);
}

void vitaEventDeleteObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestDeleteObject", event->Code, eventId);
    int ohfi = event->Param2;
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        fprintf (stderr, "OHFI %d not found.\n", ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    struct cma_object *parent = ohfiToObject (object->metadata.ohfiParent);
    deleteAll (object->path);
    fprintf (stderr, "Deleted %s from filesystem.\n", object->metadata.path);
    removeFromDatabase (ohfi, parent);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
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
    unsigned char *data;
    unsigned int len = (unsigned int)part_init.size;
    if (readFileToBuffer (object->path, part_init.offset, &data, &len) < 0) {
        fprintf (stderr, "Cannot read %s.\n", object->path);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    VitaMTP_SendPartOfObject (device, eventId, data, len);
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
}

void vitaEventOperateObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event->Code, eventId);
    operate_object_t operateobject;
    VitaMTP_OperateObject(device, eventId, &operateobject);
    struct cma_object *root = ohfiToObject (operateobject.ohfi);
    struct cma_object *newobj;
    // for renaming only
    char *origFullPath;
    char *origName;
    // end for renaming only
    if (root == NULL) {
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    // do command, 1 = create folder for ex
    switch (operateobject.cmd) {
        case VITA_OPERATE_CREATE_FOLDER:
            newobj = addToDatabase (root, operateobject.title, 0, Folder);
            if (createNewDirectory (newobj->path) < 0) {
                removeFromDatabase (newobj->metadata.ohfi, root);
                fprintf (stderr, "Unable to create temporary folder: %s\n", operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            fprintf (stderr, "Created new folder %s with OHFI %d under parent %s\n", newobj->metadata.path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
            break;
        case VITA_OPERATE_CREATE_FILE:
            newobj = addToDatabase (root, operateobject.title, 0, File);
            if (createNewFile (newobj->path) < 0) {
                removeFromDatabase (newobj->metadata.ohfi, root);
                fprintf (stderr, "Unable to create temporary file: %s\n", operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            fprintf (stderr, "Created new file %s with OHFI %d under parent %s\n", newobj->metadata.path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
            break;
        case VITA_OPERATE_RENAME:
            origName = strdup (root->metadata.name);
            origFullPath = strdup (root->path);
            // rename in database
            renameRootEntry (root, NULL, operateobject.title);
            // rename in filesystem
            if (rename (origFullPath, root->path) < 0) {
                // if failed rename back
                renameRootEntry (root, NULL, origName);
                // report the failure
                fprintf (stderr, "Unable to rename %s to %s\n", origName, operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            fprintf (stderr, "Renamed OHFI %d from %s to %s\n", root->metadata.ohfi, origName, root->metadata.name);
            // free old data
            free (origFullPath);
            free (origName);
            // send result
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, root->metadata.ohfi);
            break;
        default:
            VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
            
    }
    free (operateobject.title);
}

void vitaEventGetPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event->Code, eventId);
    unsigned char *data;
    send_part_init_t part_init;
    if (VitaMTP_GetPartOfObject (device, eventId, &part_init, &data) != PTP_RC_OK) {
        fprintf (stderr, "Cannot get object from device.\n");
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_Data);
        return;
    }
    struct cma_object *object = ohfiToObject (part_init.ohfi);
    if (object == NULL) {
        fprintf (stderr, "Cannot find OHFI %d.\n", part_init.ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        free (data);
        return;
    }
    if (writeFileFromBuffer (object->path, part_init.offset, data, part_init.size) < 0) {
        fprintf (stderr, "Cannot write to file %s.\n", object->path);
        VitaMTP_ReportResult (device, eventId, PTP_RC_AccessDenied);
        free (data);
        return;
    }
    free (data);
    // add size to all parents
    incrementSizeMetadata (object, part_init.size);
    fprintf (stderr, "Written %llu bytes to %s at offset %llu.\n", part_init.size, object->path, part_init.offset);
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
}

void vitaEventSendStorageSize (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event->Code, eventId);
    //int unk2 = event->Param2; // TODO: What is this, is set to 0x0A
    VitaMTP_SendStorageSize(device, eventId, (uint64_t)100*1024*1024*1024, (uint64_t)50*1024*1024*1024); // Send fake 50GB/100GB
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventCheckExistance (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) { // [sic]
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event->Code, eventId);
}

uint16_t vitaGetAllObjects (LIBMTP_mtpdevice_t *device, int eventId, struct cma_object *parent, uint32_t handle) {
    union {
        unsigned char *fileData;
        uint32_t *handles;
    } data;
    unsigned int length;
    metadata_t tempMeta;
    struct cma_object *object;
    struct cma_object *temp;
    unsigned int i;
    uint16_t ret;
    
    if (VitaMTP_GetObject (device, handle, &tempMeta, (void**)&data, &length) != PTP_RC_OK) {
        fprintf (stderr, "Cannot get object for handle %d.\n", handle);
        return PTP_RC_VITA_Invalid_Data;
    }
    if ((object = addToDatabase (parent, tempMeta.name, 0, tempMeta.dataType)) == NULL) { // size will be added after read
        fprintf (stderr, "Cannot add object %s to database.\n", tempMeta.name);
        free (tempMeta.name);
        return PTP_RC_VITA_Invalid_Data;
    }
    object->metadata.handle = tempMeta.handle;
    free (tempMeta.name); // not needed anymore, copy in object
    if ((temp = titleToObject (object->metadata.path, parent->metadata.ohfi)) != object && temp != NULL) { // check if object exists already
        // delete existing file/folder
        deleteAll (temp->path);
        removeFromDatabase (temp->metadata.ohfi, parent);
    }
    if (object->metadata.dataType & File) {
        if (writeFileFromBuffer (object->path, 0, data.fileData, tempMeta.size) < 0) {
            fprintf (stderr, "Cannot write to %s.\n", object->path);
            removeFromDatabase (object->metadata.ohfi, parent);
            return PTP_RC_VITA_Invalid_Permission;
        }
        incrementSizeMetadata (object, tempMeta.size);
    } else if (object->metadata.dataType & Folder) {
        if (createNewDirectory (object->path) < 0) {
            removeFromDatabase (object->metadata.ohfi, parent);
            fprintf (stderr, "Cannot create directory: %s\n", object->path);
            return PTP_RC_VITA_Failed_Operate_Object;
        }
        for (i = 0; i < length; i++) {
            ret = vitaGetAllObjects (device, eventId, object, data.handles[i]);
            if (ret != PTP_RC_OK) {
                removeFromDatabase (object->metadata.ohfi, parent);
                return ret;
            }
        }
    } else {
        assert (0); // should not be here
    }
    return PTP_RC_OK;
}

void vitaEventGetTreatObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    fprintf(stderr, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event->Code, eventId);
    treat_object_t treatObject;
    struct cma_object *parent;
    if (VitaMTP_GetTreatObject(device, eventId, &treatObject) != PTP_RC_OK) {
        fprintf (stderr, "Cannot get information on object to get.\n");
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    if ((parent = ohfiToObject (treatObject.ohfiParent)) == NULL) {
        fprintf (stderr, "Cannot find parent OHFI %d.\n", treatObject.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);
        return;
    }
    VitaMTP_ReportResult(device, eventId, vitaGetAllObjects (device, eventId, parent, treatObject.handle));
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
            slot = sizeof (event_processes)/sizeof (void*) - 1; // last item is pointer to "unimplemented
        }
        fprintf (stderr, "Event 0x%04X recieved, slot %d with function address %p\n", event.Code, slot, event_processes[slot]);
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
