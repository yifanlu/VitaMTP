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
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vitamtp.h>

#include "opencma.h"

#define LOCK_SEMAPHORE(s) while (sem_trywait (s) == 0)

extern struct cma_database *g_database;
struct cma_paths g_paths;
char *g_uuid;
static sem_t *g_refresh_database_request;
int g_connected = 0;
int g_log_level = LINFO;

static const char *HELP_STRING =
"usage: opencma [options]\n"
"   options\n"
"       -u path     Path to local URL mappings\n"
"       -p path     Path to photos\n"
"       -v path     Path to videos\n"
"       -m path     Path to music\n"
"       -a path     Path to apps\n"
"       -l level    logging level, number 0-9.\n"
"                   0 = error, 4 = verbose, 6 = debug\n"
"       -h          Show this help text\n";

static const metadata_t g_thumbmeta = {0, 0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {18, 144, 80, 0, 1, 1.0f, 2}, NULL};

static const vita_event_process_t g_event_processes[] = {
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
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNumOfObject", event->Code, eventId);
    uint32_t ohfi = event->Param2; // what kind of items are we looking for?
    //int unk1 = event->Param3; // TODO: what is this? all zeros from tests
    if (g_database == NULL) {
        LOG (LERROR, "Vita tried to reconnect when there is no prevous session. You may have to disconnect the Vita and restart OpenCMA\n");
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Ready);
        return;
    }
    lockDatabase ();
    int items = filterObjects (ohfi, NULL);
    if (VitaMTP_SendNumOfObject(device, eventId, items) != PTP_RC_OK) {
        LOG (LERROR, "Error occured receiving object count for OHFI parent %d\n", ohfi);
    } else {
        LOG (LVERBOSE, "Returned count of %d objects for OHFI parent %d\n", items, ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    unlockDatabase ();
}

void vitaEventSendObjectMetadata (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event->Code, eventId);
    browse_info_t browse;
    metadata_t *meta;
    if (VitaMTP_GetBrowseInfo(device, eventId, &browse) != PTP_RC_OK) {
        LOG (LERROR, "GetBrowseInfo failed.\n");
        return;
    }
    lockDatabase ();
    filterObjects (browse.ohfiParent, &meta); // if meta is null, will return empty XML
    if (VitaMTP_SendObjectMetadata(device, eventId, meta) != PTP_RC_OK) { // send all objects with OHFI parent
        LOG (LERROR, "Sending metadata for OHFI parent %d failed\n", browse.ohfiParent);
    } else {
        LOG (LVERBOSE, "Sent metadata for OHFI parent %d\n", browse.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    unlockDatabase ();
}

void vitaEventSendObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event->Code, eventId);
    uint32_t ohfi = event->Param2;
    uint32_t parentHandle = event->Param3;
    uint32_t handle;
    lockDatabase ();
    struct cma_object *object = ohfiToObject (ohfi);
    struct cma_object *start = object;
    struct cma_object *temp = NULL;
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Failed to find OHFI %d.\n", ohfi);
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
                unlockDatabase ();
                LOG (LERROR, "Failed to read %s.\n", object->path);
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
        // TODO: Use mmap when sending extra large objects like videos
        LOG (LINFO, "Sending %s of %u bytes to device.\n", object->metadata.name, len);
        LOG (LDEBUG, "OHFI %d with handle 0x%08X\n", ohfi, parentHandle);
        if (VitaMTP_SendObject (device, &parentHandle, &handle, &object->metadata, data) != PTP_RC_OK) {
            LOG (LERROR, "Sending of %s failed.\n", object->metadata.name);
            unlockDatabase ();
            free (data);
            return;
        }
        object->metadata.handle = handle;
        object = object->next_object;
        
        free (data);
    }while (object != NULL && object->metadata.ohfiParent >= OHFI_OFFSET); // get everything under this "folder"
    unlockDatabase ();
    VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, handle);
    VitaMTP_ReportResult (device, eventId, PTP_RC_NoThumbnailPresent); // TODO: Send thumbnail
}

void vitaEventCancelTask (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event->Code, eventId);
    int eventIdToCancel = event->Param2;
    VitaMTP_CancelTask (device, eventIdToCancel);
    LOG (LERROR, "Event CancelTask (0x%x) unimplemented!\n", event->Code);
}

void vitaEventSendHttpObjectFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event->Code, eventId);
    char *url = NULL;
    if (VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK) {
        LOG (LERROR, "Failed to recieve URL.\n");
        return;
    }
    unsigned char *data;
    unsigned int len = 0;
    if (requestURL (url, &data, &len) < 0) {
        free (url);
        LOG (LERROR, "Failed to download %s\n", url);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Download);
        return;
    }
    LOG (LINFO, "Sending %ud bytes of data for HTTP request %s\n", len, url);
    if (VitaMTP_SendHttpObjectFromURL(device, eventId, data, len) != PTP_RC_OK) {
        LOG (LERROR, "Failed to send HTTP object.\n");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    free (url);
    free (data);
}

void vitaEventSendObjectStatus (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event->Code, eventId);
    object_status_t objectstatus;
    struct cma_object *object;
    if (VitaMTP_SendObjectStatus(device, eventId, &objectstatus) != PTP_RC_OK) {
        LOG (LERROR, "Failed to get information for object status.\n");
        return;
    }
    lockDatabase ();
    object = pathToObject(objectstatus.title, objectstatus.ohfiRoot);
    if(object == NULL) { // not in database, don't return metadata
        LOG (LVERBOSE, "Object %s not in database. Sending OK response for non-existence.\n", objectstatus.title);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    } else {
        metadata_t *metadata = &object->metadata;
        metadata->next_metadata = NULL;
        LOG (LDEBUG, "Sending metadata for OHFI %d.\n", object->metadata.ohfi);
        if (VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK) {
            LOG (LERROR, "Error sending metadata for %d\n", object->metadata.ohfi);
        } else {
            VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
        }
    }
    unlockDatabase ();
    free(objectstatus.title);
}

void vitaEventSendObjectThumb (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectThumb", event->Code, eventId);
    char thumbpath[PATH_MAX];
    uint32_t ohfi = event->Param2;
    lockDatabase ();
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Cannot find OHFI %d in database.\n", ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    thumbpath[0] = '\0';
    if (MASK_SET (object->metadata.dataType, Photo)) {
        // TODO: Don't send full image (may be too large)
        LOG (LDEBUG, "Sending photo as thumbnail %s", object->metadata.path);
        strcpy (thumbpath, object->path);
    } else if (MASK_SET (object->metadata.dataType, SaveData)) {
        LOG (LDEBUG, "Sending savedata thumbnail %s\n", thumbpath);
        sprintf (thumbpath, "%s/%s", object->path, "ICON0.PNG");
    } else {
        LOG (LERROR, "Thumbnail sending for the file %s is not supported.\n", object->metadata.path);
        unlockDatabase ();
        VitaMTP_ReportResult (device, eventId, PTP_RC_NoThumbnailPresent);
        return;
    }
    unlockDatabase ();
    // TODO: Get thumbnail data correctly
    unsigned char *data;
    unsigned int len = 0;
    if (readFileToBuffer (thumbpath, 0, &data, &len) < 0) {
        LOG (LERROR, "Cannot find thumbnail %s\n", thumbpath);
        VitaMTP_ReportResult (device, eventId, PTP_RC_NoThumbnailPresent);
        return;
    }
    if (VitaMTP_SendObjectThumb (device, eventId, (metadata_t *)&g_thumbmeta, data, len) != PTP_RC_OK) {
        LOG (LERROR, "Error sending thumbnail %s\n", thumbpath);
    } else {
        VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    }
    free (data);
}

void vitaEventDeleteObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestDeleteObject", event->Code, eventId);
    int ohfi = event->Param2;
    lockDatabase ();
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "OHFI %d not found.\n", ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    struct cma_object *parent = ohfiToObject (object->metadata.ohfiParent);
    deleteAll (object->path);
    LOG (LINFO, "Deleted %s\n", object->metadata.path);
    removeFromDatabase (ohfi, parent);
    unlockDatabase ();
    VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
}

void vitaEventGetSettingInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetSettingInfo", event->Code, eventId);
    settings_info_t settingsinfo;
    struct account *account;
    struct account *lastAccount = NULL;
    if (VitaMTP_GetSettingInfo(device, eventId, &settingsinfo) != PTP_RC_OK) {
        LOG (LERROR, "Failed to get setting info from Vita.\n");
        return;
    }
    LOG (LVERBOSE, "Current account id: %s\n", settingsinfo.current_account.accountId);
    free (g_uuid);
    g_uuid = strdup (settingsinfo.current_account.accountId);
    // set the database to be updated ASAP
    sem_post (g_refresh_database_request);
    // free all the information
    for (account = &settingsinfo.current_account; account != NULL; account = account->next_account) {
        free (lastAccount);
        free (account->accountId);
        free (account->birthday);
        free (account->countryCode);
        free (account->langCode);
        free (account->onlineUser);
        free (account->passwd);
        free (account->signInId);
        lastAccount = account;
    }
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendHttpObjectPropFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectPropFromURL", event->Code, eventId);
    char *url = NULL;
    unsigned char *data;
    http_object_prop_t httpobjectprop;
    if (VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK) {
        LOG (LERROR, "Failed to get URL.\n");
        return;
    }
    if (requestURL (url, &data, (unsigned int*)&httpobjectprop.size) < 0) {
        LOG (LERROR, "Failed to read data for %s\n", url);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Download);
        free (url);
        return;
    }
    free (data);
    httpobjectprop.timestamp = NULL; // TODO: Actually get timestamp
    httpobjectprop.timestamp_len = 0;
    if (VitaMTP_SendHttpObjectPropFromURL(device, eventId, &httpobjectprop) != PTP_RC_OK) {
        LOG (LERROR, "Failed to send object properties.\n");
    } else {
        VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    }
    free(url);
}

void vitaEventSendPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendPartOfObject", event->Code, eventId);
    send_part_init_t part_init;
    if (VitaMTP_SendPartOfObjectInit (device, eventId, &part_init) != PTP_RC_OK) {
        LOG (LERROR, "Cannot get information on object to send.\n");
        return;
    }
    lockDatabase ();
    struct cma_object *object = ohfiToObject (part_init.ohfi);
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Cannot find object for OHFI %d\n", part_init.ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }
    unsigned char *data;
    unsigned int len = (unsigned int)part_init.size;
    if (readFileToBuffer (object->path, part_init.offset, &data, &len) < 0) {
        LOG (LERROR, "Cannot read %s.\n", object->path);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        unlockDatabase ();
        return;
    }
    LOG (LINFO, "Sending %s at file offset %llu for %llu bytes\n", object->metadata.path, part_init.offset, part_init.size);
    unlockDatabase ();
    if (VitaMTP_SendPartOfObject (device, eventId, data, len) != PTP_RC_OK) {
        LOG (LERROR, "Failed to send part of object OHFI %d\n", part_init.ohfi);
    } else {
        VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    }
}

void vitaEventOperateObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event->Code, eventId);
    operate_object_t operateobject;
    if (VitaMTP_OperateObject(device, eventId, &operateobject) != PTP_RC_OK) {
        LOG (LERROR, "Cannot get information on object to operate.\n");
        return;
    }
    lockDatabase ();
    struct cma_object *root = ohfiToObject (operateobject.ohfi);
    struct cma_object *newobj;
    // for renaming only
    char *origFullPath;
    char *origName;
    // end for renaming only
    if (root == NULL) {
        unlockDatabase ();
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }
    // do command, 1 = create folder for ex
    switch (operateobject.cmd) {
        case VITA_OPERATE_CREATE_FOLDER:
            LOG (LDEBUG, "Operate command %d: Create folder %s\n", operateobject.cmd, operateobject.title);
            newobj = addToDatabase (root, operateobject.title, 0, Folder);
            if (createNewDirectory (newobj->path) < 0) {
                removeFromDatabase (newobj->metadata.ohfi, root);
                LOG (LERROR, "Unable to create temporary folder: %s\n", operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            LOG (LINFO, "Created folder %s\n", newobj->metadata.path);
            LOG (LVERBOSE, "Folder %s with OHFI %d under parent %s\n", newobj->path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
            break;
        case VITA_OPERATE_CREATE_FILE:
            LOG (LDEBUG, "Operate command %d: Create file %s\n", operateobject.cmd, operateobject.title);
            newobj = addToDatabase (root, operateobject.title, 0, File);
            if (createNewFile (newobj->path) < 0) {
                removeFromDatabase (newobj->metadata.ohfi, root);
                LOG (LERROR, "Unable to create temporary file: %s\n", operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            LOG (LINFO, "Created file %s\n", newobj->metadata.path);
            LOG (LVERBOSE, "File %s with OHFI %d under parent %s\n", newobj->path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
            break;
        case VITA_OPERATE_RENAME:
            LOG (LDEBUG, "Operate command %d: Rename %s to %s\n", operateobject.cmd, root->metadata.name, operateobject.title);
            origName = strdup (root->metadata.name);
            origFullPath = strdup (root->path);
            // rename in database
            renameRootEntry (root, NULL, operateobject.title);
            // rename in filesystem
            if (rename (origFullPath, root->path) < 0) {
                // if failed rename back
                renameRootEntry (root, NULL, origName);
                // report the failure
                LOG (LERROR, "Unable to rename %s to %s\n", origName, operateobject.title);
                VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
                break;
            }
            LOG (LINFO, "Renamed %s to %s\n", origName, root->metadata.path);
            LOG (LVERBOSE, "Renamed OHFI %d from %s to %s\n", root->metadata.ohfi, origName, root->metadata.name);
            // free old data
            free (origFullPath);
            free (origName);
            // send result
            VitaMTP_ReportResultWithParam (device, eventId, PTP_RC_OK, root->metadata.ohfi);
            break;
        default:
            LOG (LERROR, "Operate command %d: Not implemented.\n", operateobject.cmd);
            VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
            
    }
    unlockDatabase ();
    free (operateobject.title);
}

void vitaEventGetPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event->Code, eventId);
    unsigned char *data;
    send_part_init_t part_init;
    if (VitaMTP_GetPartOfObject (device, eventId, &part_init, &data) != PTP_RC_OK) {
        LOG (LERROR, "Cannot get object from device.\n");
        return;
    }
    lockDatabase ();
    struct cma_object *object = ohfiToObject (part_init.ohfi);
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Cannot find OHFI %d.\n", part_init.ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        free (data);
        return;
    }
    LOG (LINFO, "Receiving %s at offset %llu for %llu bytes\n", object->metadata.path, part_init.offset, part_init.size);
    if (writeFileFromBuffer (object->path, part_init.offset, data, part_init.size) < 0) {
        LOG (LERROR, "Cannot write to file %s.\n", object->path);
        VitaMTP_ReportResult (device, eventId, PTP_RC_AccessDenied);
    } else {
        // add size to all parents
        incrementSizeMetadata (object, part_init.size);
        LOG (LDEBUG, "Written %llu bytes to %s at offset %llu.\n", part_init.size, object->path, part_init.offset);
        VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    }
    unlockDatabase ();
    free (data);
}

void vitaEventSendStorageSize (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event->Code, eventId);
    int ohfi = event->Param2;
    lockDatabase ();
    struct cma_object *object = ohfiToObject (ohfi);
    size_t total;
    size_t free;
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Cannot find OHFI %d.\n", ohfi);
        VitaMTP_ReportResult (device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    if (getDiskSpace (object->path, &free, &total) < 0) {
        unlockDatabase ();
        LOG (LERROR, "Cannot get disk space.\n");
        VitaMTP_ReportResult (device, eventId, PTP_RC_AccessDenied);
        return;
    }
    unlockDatabase ();
    LOG (LVERBOSE, "For drive containing OHFI %d, free: %zu, total: %zu\n", ohfi, free, total);
    if (VitaMTP_SendStorageSize(device, eventId, total, free) != PTP_RC_OK) { // Send fake 50GB/100GB
        LOG (LERROR, "Send storage size failed.\n");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void vitaEventCheckExistance (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) { // [sic]
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event->Code, eventId);
    int handle = event->Param2;
    existance_object_t existance;
    struct cma_object *object;
    if (VitaMTP_CheckExistance (device, handle, &existance) != PTP_RC_OK) {
        LOG (LERROR, "Cannot read information on object to be sent.\n");
        return;
    }
    lockDatabase ();
    if ((object = pathToObject (existance.name, 0)) == NULL) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Different_Object);
    } else {
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_VITA_Same_Object, object->metadata.ohfi);
    }
    unlockDatabase ();
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
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
        LOG (LERROR, "Cannot get object for handle %d.\n", handle);
        return PTP_RC_VITA_Invalid_Data;
    }
    lockDatabase ();
    if ((object = addToDatabase (parent, tempMeta.name, 0, tempMeta.dataType)) == NULL) { // size will be added after read
        unlockDatabase ();
        LOG (LERROR, "Cannot add object %s to database.\n", tempMeta.name);
        free (tempMeta.name);
        return PTP_RC_VITA_Invalid_Data;
    }
    object->metadata.handle = tempMeta.handle;
    free (tempMeta.name); // not needed anymore, copy in object
    if ((temp = pathToObject (object->metadata.name, parent->metadata.ohfi)) != object && temp != NULL) { // check if object exists already
        // delete existing file/folder
        LOG (LDEBUG, "Deleting %s\n", temp->path);
        deleteAll (temp->path);
        removeFromDatabase (temp->metadata.ohfi, parent);
    }
    if (object->metadata.dataType & File) {
        LOG (LINFO, "Receiving %s for %lu bytes.\n", object->metadata.path, tempMeta.size);
        if (writeFileFromBuffer (object->path, 0, data.fileData, tempMeta.size) < 0) {
            LOG (LERROR, "Cannot write to %s.\n", object->path);
            removeFromDatabase (object->metadata.ohfi, parent);
            unlockDatabase ();
            return PTP_RC_VITA_Invalid_Permission;
        }
        incrementSizeMetadata (object, tempMeta.size);
    } else if (object->metadata.dataType & Folder) {
        LOG (LINFO, "Receiving directory %s\n", object->metadata.path);
        if (createNewDirectory (object->path) < 0) {
            removeFromDatabase (object->metadata.ohfi, parent);
            LOG (LERROR, "Cannot create directory: %s\n", object->path);
            unlockDatabase ();
            return PTP_RC_VITA_Failed_Operate_Object;
        }
        for (i = 0; i < length; i++) {
            ret = vitaGetAllObjects (device, eventId, object, data.handles[i]);
            if (ret != PTP_RC_OK) {
                removeFromDatabase (object->metadata.ohfi, parent);
                unlockDatabase ();
                return ret;
            }
        }
    } else {
        assert (0); // should not be here
    }
    unlockDatabase ();
    return PTP_RC_OK;
}

void vitaEventGetTreatObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event->Code, eventId);
    treat_object_t treatObject;
    struct cma_object *parent;
    if (VitaMTP_GetTreatObject(device, eventId, &treatObject) != PTP_RC_OK) {
        LOG (LERROR, "Cannot get information on object to get.\n");
        return;
    }
    if ((parent = ohfiToObject (treatObject.ohfiParent)) == NULL) {
        LOG (LERROR, "Cannot find parent OHFI %d.\n", treatObject.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    VitaMTP_ReportResult(device, eventId, vitaGetAllObjects (device, eventId, parent, treatObject.handle));
}

void vitaEventSendCopyConfirmationInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendCopyConfirmationInfo", event->Code, eventId);
    copy_confirmation_info_t *info;
    struct cma_object *object;
    if (VitaMTP_SendCopyConfirmationInfoInit (device, eventId, &info) != PTP_RC_OK) {
        LOG (LERROR, "Error recieving initial information.\n");
        return;
    }
    lockDatabase ();
    uint32_t i;
    uint64_t size = 0;
    for (i = 0; i < info->count; i++) {
        if ((object = ohfiToObject (info->ohfi[i])) == NULL) {
            LOG (LERROR, "Cannot find OHFI %d.\n", info->ohfi[i]);
            free (info);
            unlockDatabase ();
            return;
        }
        size += object->metadata.size;
    }
    unlockDatabase ();
    if (VitaMTP_SendCopyConfirmationInfo (device, eventId, info, size) != PTP_RC_OK) {
        LOG (LERROR, "Error sending copy confirmation.\n");
    } else {
        VitaMTP_ReportResult (device, eventId, PTP_RC_OK);
    }
    free (info);
}

void vitaEventSendObjectMetadataItems (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadataItems", event->Code, eventId);
    uint32_t ohfi;
    if (VitaMTP_SendObjectMetadataItems(device, eventId, &ohfi) != PTP_RC_OK) {
        LOG (LERROR, "Cannot get OHFI for retreving metadata.\n");
        return;
    }
    lockDatabase ();
    struct cma_object *object = ohfiToObject (ohfi);
    if (object == NULL) {
        unlockDatabase ();
        LOG (LERROR, "Cannot find OHFI %d in database\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }
    metadata_t *metadata = &object->metadata;
    metadata->next_metadata = NULL;
    LOG (LVERBOSE, "Sending metadata for OHFI %d (%s)\n", ohfi, metadata->path);
    if (VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK) {
        LOG (LERROR, "Error sending metadata.\n");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    unlockDatabase ();
}

void vitaEventSendNPAccountInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNPAccountInfo", event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    LOG (LERROR, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventRequestTerminate (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestTerminate", event->Code, eventId);
    LOG (LERROR, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventUnimplementated (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId) {
    LOG (LERROR, "Unknown event not handled, code: 0x%x, id: %d\n", event->Code, eventId);
    LOG (LDEBUG, "Param1: 0x%08X, Param2: 0x%08X, Param3: 0x%08X\n", event->Param1, event->Param2, event->Param3);
}

void *vitaEventListener(LIBMTP_mtpdevice_t *device) {
    LIBMTP_event_t event;
    int slot;
    while(g_connected) {
        if(LIBMTP_Read_Event(device, &event) < 0) {
            LOG (LERROR, "Error reading event from USB interrupt.\n");
            g_connected = 0;
            continue;
        }
        slot = event.Code - PTP_EC_VITA_RequestSendNumOfObject;
        if (slot < 0 || slot > sizeof (g_event_processes)/sizeof (void*)) {
            slot = sizeof (g_event_processes)/sizeof (void*) - 1; // last item is pointer to "unimplemented
        }
        LOG (LDEBUG, "Event 0x%04X recieved, slot %d with function address %p\n", event.Code, slot, g_event_processes[slot]);
        g_event_processes[slot] (device, &event, event.Param1);
    }
    
    return NULL;
}

static void interrupt_handler (int signum) {
    if (!g_connected) {
        LOG (LINFO, "No active connection, killing process.\n");
        exit (0);
    } else {
        LOG (LINFO, "Stopping event listener.\n");
        g_connected = 0;
        sem_post (g_refresh_database_request); // so we stop waiting
    }
}

static void sigtstp_handler (int signum) {
    if (!g_connected) {
        LOG (LINFO, "No active connection, ignoring request to refresh database.\n");
        return;
    }
    LOG (LINFO, "Refreshing the database.\n");
    // SIGTSTP will be used for a user request to refresh the database
    sem_post (g_refresh_database_request);
    // TODO: For some reason SIGTSTP automatically unlocks the semp.
}

int main(int argc, char** argv) {
    /* First we will parse the command line arguments */
    
    // Start with some default values
    char cwd[FILENAME_MAX];
    getcwd(cwd, FILENAME_MAX);
    g_uuid = strdup("ffffffffffffffff");
    g_paths.urlPath = cwd;
    g_paths.photosPath = cwd;
    g_paths.videosPath = cwd;
    g_paths.musicPath = cwd;
    g_paths.appsPath = cwd;
    
    // Show help string
    fprintf (stderr, "%s\nlibVitaMTP Version: %d.%d\nVita Protocol Version: %08d\n",
            OPENCMA_VERSION_STRING, VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR, VITAMTP_PROTOCOL_VERSION);
    fprintf (stderr, "Once connected, send SIGTSTP (usually Ctrl+Z) to refresh the database.\n");
    
    // Now get the arguments
    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "u:p:v:m:a:l:hd")) != -1) {
        switch (c) {
            case 'u': // local url mapping path
                g_paths.urlPath = optarg;
                break;
            case 'p': // photo path
                g_paths.photosPath = optarg;
                break;
            case 'v': // video path
                g_paths.videosPath = optarg;
                break;
            case 'm': // music path
                g_paths.musicPath = optarg;
                break;
            case 'a': // app path
                g_paths.appsPath = optarg;
                break;
            case 'l': // logging
                g_log_level = atoi (optarg);
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr, "%s\n", HELP_STRING);
                exit (1);
                break;
        }
    }
    
    /* Check if folders exist */
    if (!fileExists (g_paths.urlPath)) {
        LOG (LERROR, "Cannot find path: %s\n", g_paths.urlPath);
        return 1;
    }
    if (!fileExists (g_paths.photosPath)) {
        LOG (LERROR, "Cannot find path: %s\n", g_paths.photosPath);
        return 1;
    }
    if (!fileExists (g_paths.videosPath)) {
        LOG (LERROR, "Cannot find path: %s\n", g_paths.videosPath);
        return 1;
    }
    if (!fileExists (g_paths.musicPath)) {
        LOG (LERROR, "Cannot find path: %s\n", g_paths.musicPath);
        return 1;
    }
    if (!fileExists (g_paths.appsPath)) {
        LOG (LERROR, "Cannot find path: %s\n", g_paths.appsPath);
        return 1;
    }
    
    /* Set up the database */
    struct sigaction action;
    action.sa_handler = sigtstp_handler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGTSTP, &action, NULL) < 0) {   // SIGTERM will be used for cleanup
        LOG (LERROR, "Cannot install SIGTSTP handler.\n");
        return 1;
    }
    action.sa_handler = interrupt_handler;
    if (sigaction (SIGINT, &action, NULL) < 0) {    // SIGINT will be used to refresh database
        LOG (LERROR, "Cannot install SIGINT handler.\n");
        return 1;
    }
    if ((g_refresh_database_request = sem_open ("/opencma_refresh_db", O_CREAT, 0777, 0)) == SEM_FAILED) {
        LOG (LERROR, "Cannot create semaphore for event flag.\n");
        return 1;
    }
    LOCK_SEMAPHORE (g_refresh_database_request); // make sure it's locked
    
    /* Now, we can set up the device */
    
    // This lets us have detailed logs including dumps of MTP packets
    LIBMTP_Set_Debug(g_log_level <= LVERBOSE ? 0 : g_log_level <= LDEBUG ? 0x8 : 0xF);
    
    // This must be called to initialize libmtp 
    LIBMTP_Init();
    
    LIBMTP_mtpdevice_t *device;
    
    // Wait for the device to be plugged in
    LOG (LINFO, "Waiting for Vita to connect...\n");
    do {
        sleep(10);
        // This will do MTP initialization if the device is found
        device = LIBVitaMTP_Get_First_Vita();
    } while (device == NULL);
    LOG (LINFO, "Vita connected: serial %s\n", LIBMTP_Get_Serialnumber (device));
    
    // Create the event listener thread, technically we do not
    // need a seperate thread to do this since the main thread 
    // is not doing anything else, but to make the example 
    // complete, we will assume the main thread has more 
    // important things.
    pthread_t event_thread;
    g_connected = 1;
    if(pthread_create(&event_thread, NULL, (void*(*)(void*))vitaEventListener, device) != 0) {
        LOG (LERROR, "Cannot create event listener thread.\n");
        return 1;
    }
    
    // Here we will do Vita specific initialization
    vita_info_t vita_info;
    // This will automatically fill pc_info with default information
    const initiator_info_t *pc_info = new_initiator_info(OPENCMA_VERSION_STRING);
    
    // First, we get the Vita's info
    if (VitaMTP_GetVitaInfo(device, &vita_info) != PTP_RC_OK) {
        LOG (LERROR, "Cannot retreve device information.\n");
        return 1;
    }
    // Next, we send the client's (this program) info (discard the const here)
    if (VitaMTP_SendInitiatorInfo(device, (initiator_info_t*)pc_info) != PTP_RC_OK) {
        LOG (LERROR, "Cannot send host information.\n");
        return 1;
    }
    // Finally, we tell the Vita we are connected
    if (VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_Connected) != PTP_RC_OK) {
        LOG (LERROR, "Cannot send host status.\n");
        return 1;
    }
    // We do not need the client's info anymore, so free it to prevent memory leaks
    // Do not use this function if you manually created the initiator_info.
    // This will only work with ones created from new_initiator_info()
    free_initiator_info(pc_info);
    
    // this thread will update the database when needed
    while (g_connected) {
        sem_wait (g_refresh_database_request);
        if (!g_connected) { // TODO: more clean way of doing this
            break;
        }
        LOG (LINFO, "Refreshing database for user %s (this may take some time)...\n", g_uuid);
        LOG (LDEBUG, "URL Mapping Path: %s\nPhotos Path: %s\nVideos Path: %s\nMusic Path: %s\nApps Path: %s\n",
             g_paths.urlPath, g_paths.photosPath, g_paths.videosPath, g_paths.musicPath, g_paths.appsPath);
        destroyDatabase ();
        createDatabase (&g_paths, g_uuid);
        LOG (LINFO, "Database refreshed.\n");
        LOCK_SEMAPHORE (g_refresh_database_request); // in case multiple requests were made
    }
    
    LOG (LINFO, "Shutting down...\n");
    
    // End this connection with the Vita
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
    
    // Clean up our mess
    LIBMTP_Release_Device(device);
    destroyDatabase ();
    sem_close (g_refresh_database_request);
    sem_unlink ("/opencma_refresh_db");
    
    LOG (LINFO, "Exiting.\n");
    
    return 0;
}
