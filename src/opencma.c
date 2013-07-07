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

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "opencma.h"

#define LOCK_SEMAPHORE(s) while (sem_trywait (s) == 0)

#ifdef _WIN32
#include <windows.h>
#define sleep(x) (Sleep((x)*1000))
#endif

extern struct cma_database *g_database;
struct cma_paths g_paths;
char *g_uuid;
#ifndef __APPLE__
static sem_t g_sem_storage;
#endif
static sem_t *g_refresh_database_request;
int g_connected = 0;
unsigned int g_log_level = LINFO;

static const char *g_help_string =
    "usage: opencma [wireless|usb] paths [options]\n"
    "   mode\n"
    "       wireless    Listen for Wifi connection\n"
    "       usb         Listen for USB connection (default)\n"
    "   paths\n"
    "       -p path     Path to photos\n"
    "       -v path     Path to videos\n"
    "       -m path     Path to music\n"
    "       -a path     Path to apps\n"
    "       -k path     Path to packages (default ./packages)\n"
    "   options\n"
    "       -u path     Path to local URL mappings\n"
    "       -l level    logging level, number 1-4.\n"
    "                   1 = error, 2 = info, 3 = verbose, 4 = debug\n"
    "       -h          Show this help text\n"
    "\n"
    "additional information:\n"
    "\n"
    "   All paths must be specified. Please note that having larger\n"
    "   directories means that OpenCMA will run slower and use more memory.\n"
    "   This is because OpenCMA doesn't have an external database and builds\n"
    "   (and keeps) its database in memory. If you try to run OpenCMA with\n"
    "   paths that contains lots of files and directories it may quickly run\n"
    "   out of memory. Also beware that using the same path for multiple data\n"
    "   types (photos and videos, for example) is undefined behavior. It can\n"
    "   result in files not showing up without a manual database refresh.\n"
    "   Modifying the directory as OpenCMA is running may also result in the\n"
    "   same behavior.\n"
    "\n"
    "   URL mappings allow you to redirect Vita's URL download requests to\n"
    "   some file locally. This can be used to, for example, change the file\n"
    "   for firmware upgrading when you choose to update the Vita via USB. The\n"
    "   Vita may request http://example.com/PSP2UPDAT.PUP and if you use the\n"
    "   option '-u /path/to/fw' then OpenCMA will send\n"
    "   /path/to/fw/PSP2UPDAT.PUP to the Vita. You do NOT need to do this for\n"
    "   psp2-updatelist.xml to bypass the update prompt because that file is\n"
    "   built in to OpenCMA for your convenience. If you do wish to  send a\n"
    "   custom psp2-updatelist.xml, you can.\n"
    "\n"
    "   There are four logging levels that you can select with the '-l'\n"
    "   option. '-l 1' is the default and will only show critical error\n"
    "   messages. '-l 2' will allow you to see more of the behind-the-scenes\n"
    "   process such as what file is being sent and so on. '-l 3' will, in\n"
    "   addition, display advanced information like what event the Vita is\n"
    "   sending over, and is for the curious minded. '-l 4' will log\n"
    "   EVERYTHING including the raw USB traffic to and from the device.\n"
    "   PLEASE use this option when you are filing a bug report and attach the\n"
    "   output so the issue can be resolved quickly. Please note that more\n"
    "   logging means OpenCMA will run slower.\n"
    "\n"
    "   Once OpenCMA is running, you can execute various commands like\n"
    "   manually refreshing the database. For more information type in\n"
    "   'help' after the Vita is connected.\n";

static const char *g_update_list =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?><update_data_list><region id=\"au\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"eu\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"jp\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"kr\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"mx\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"ru\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"tw\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"uk\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region><region id=\"us\"><np level0_system_version=\"00.000.000\" level1_system_version=\"00.000.000\" level2_system_version=\"00.000.000\" map=\"00.000.000\" /><version system_version=\"00.000.000\" label=\"0.00\"></version></region></update_data_list>";

static const char *g_commands_help_string =
    "commands:\n"
    "   exit: disconnect and exit\n"
    "   refresh: refresh database\n"
    "   help: show this\n";

static const metadata_t g_thumbmeta = {0, 0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {18, 144, 80, 0, 1, 1.0f, 2}, NULL};

static const vita_event_process_t g_event_processes[] =
{
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

#ifdef _WIN32
// from http://stackoverflow.com/a/4899487
#include <stdarg.h>
int asprintf(char **ret, const char *format, ...)
{
    va_list ap;
    
    *ret = NULL;  /* Ensure value can be passed to free() */
    
    va_start(ap, format);
    int count = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    
    if (count >= 0)
    {
        char* buffer = malloc(count + 1);
        if (buffer == NULL)
            return -1;
        
        va_start(ap, format);
        count = vsnprintf(buffer, count + 1, format, ap);
        va_end(ap);
        
        if (count < 0)
        {
            free(buffer);
            return count;
        }
        *ret = buffer;
    }
    
    return count;
}
#endif

static inline void incrementSizeMetadata(struct cma_object *object, size_t size)
{
    do
    {
        object->metadata.size += size;
    }
    while (object->metadata.ohfiParent > 0 && (object = ohfiToObject(object->metadata.ohfiParent)) != NULL);
}

void vitaEventSendNumOfObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNumOfObject", event->Code, eventId);
    uint32_t ohfi = event->Param2; // what kind of items are we looking for?
    
    // special instructions for Package Installer
    if (ohfi == VITA_OHFI_PACKAGE && g_database == NULL)
    {
        // the database will be refreshed with the correct
        // account id whenever another CMA app is opened
        g_uuid = strdup("0000000000000000");
        sem_post(g_refresh_database_request);
        sleep(1); // TODO: better way to wait for refresh
    }

    //int unk1 = event->Param3; // TODO: what is this? all zeros from tests
    if (g_database == NULL)
    {
        LOG(LERROR,
            "Vita tried to reconnect when there is no prevous session. You may have to disconnect the Vita and restart OpenCMA\n");
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Ready);
        return;
    }

    lockDatabase();
    int items = filterObjects(ohfi, NULL);

    if (VitaMTP_SendNumOfObject(device, eventId, items) != PTP_RC_OK)
    {
        LOG(LERROR, "Error occured receiving object count for OHFI parent %d\n", ohfi);
    }
    else
    {
        LOG(LVERBOSE, "Returned count of %d objects for OHFI parent %d\n", items, ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    unlockDatabase();
}

void vitaEventSendObjectMetadata(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadata", event->Code, eventId);
    browse_info_t browse;
    metadata_t *meta;

    if (VitaMTP_GetBrowseInfo(device, eventId, &browse) != PTP_RC_OK)
    {
        LOG(LERROR, "GetBrowseInfo failed.\n");
        return;
    }

    lockDatabase();
    filterObjects(browse.ohfiParent, &meta);  // if meta is null, will return empty XML

    if (VitaMTP_SendObjectMetadata(device, eventId, meta) != PTP_RC_OK)   // send all objects with OHFI parent
    {
        LOG(LERROR, "Sending metadata for OHFI parent %d failed\n", browse.ohfiParent);
    }
    else
    {
        LOG(LVERBOSE, "Sent metadata for OHFI parent %d\n", browse.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    unlockDatabase();
}

void vitaEventSendObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObject", event->Code, eventId);
    uint32_t ohfi = event->Param2;
    uint32_t parentHandle = event->Param3;
    uint32_t handle;
    lockDatabase();
    struct cma_object *object = ohfiToObject(ohfi);
    struct cma_object *start = object;
    struct cma_object *temp = NULL;

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Failed to find OHFI %d.\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    unsigned char *data = NULL;
    unsigned int len = 0;

    do
    {
        data = NULL;
        len = (unsigned int)object->metadata.size;

        // read the file to send if it's not a directory
        // if it is a directory, data and len are not used by VitaMTP
        if (object->metadata.dataType & File)
        {
            if (readFileToBuffer(object->path, 0, &data, &len) < 0)
            {
                unlockDatabase();
                LOG(LERROR, "Failed to read %s.\n", object->path);
                VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
                return;
            }
        }

        // get the PTP object ID for the parent to put the object
        // we know the parent has to be before the current node
        // the first time this is called, parentHandle is left untouched
        for (temp = start; temp != NULL && temp != object; temp = temp->next_object)
        {
            if (temp->metadata.ohfi == object->metadata.ohfiParent)
            {
                parentHandle = temp->metadata.handle;
                break;
            }
        }

        // send the data over
        // TODO: Use mmap when sending extra large objects like videos
        LOG(LINFO, "Sending %s of %u bytes to device.\n", object->metadata.name, len);
        LOG(LDEBUG, "OHFI %d with handle 0x%08X\n", ohfi, parentHandle);

        if (VitaMTP_SendObject(device, &parentHandle, &handle, &object->metadata, data) != PTP_RC_OK)
        {
            LOG(LERROR, "Sending of %s failed.\n", object->metadata.name);
            unlockDatabase();
            free(data);
            return;
        }

        object->metadata.handle = handle;
        object = object->next_object;

        free(data);
    }
    while (object != NULL && object->metadata.ohfiParent >= OHFI_OFFSET);  // get everything under this "folder"

    unlockDatabase();
    VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, handle);
    VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);  // TODO: Send thumbnail
}

void vitaEventCancelTask(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCancelTask", event->Code, eventId);
    int eventIdToCancel = event->Param2;
    VitaMTP_CancelTask(device, eventIdToCancel);
    LOG(LERROR, "Event CancelTask (0x%x) unimplemented!\n", event->Code);
}

void vitaEventSendHttpObjectFromURL(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectFromURL", event->Code, eventId);
    char *url = NULL;

    if (VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to recieve URL.\n");
        return;
    }

    unsigned char *data;
    unsigned int len = 0;
    int ret = requestURL(url, &data, &len);

    if (ret < 0 && strstr(url, "/psp2-updatelist.xml"))
    {
        LOG(LINFO, "Found request for update request. Sending cached data.\n");
        data = (unsigned char *)strdup(g_update_list);
        // weirdly there must NOT be a null terminator
        len = (unsigned int)strlen(g_update_list);
    }
    else if (ret < 0)
    {
        free(url);
        LOG(LERROR, "Failed to download %s\n", url);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Download);
        return;
    }

    LOG(LINFO, "Sending %ud bytes of data for HTTP request %s\n", len, url);

    if (VitaMTP_SendHttpObjectFromURL(device, eventId, data, len) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to send HTTP object.\n");
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(url);
    free(data);
}

void vitaEventSendObjectStatus(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectStatus", event->Code, eventId);
    object_status_t objectstatus;
    struct cma_object *object;

    if (VitaMTP_SendObjectStatus(device, eventId, &objectstatus) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to get information for object status.\n");
        return;
    }

    lockDatabase();
    object = pathToObject(objectstatus.title, objectstatus.ohfiRoot);

    if (object == NULL)  // not in database, don't return metadata
    {
        LOG(LVERBOSE, "Object %s not in database. Sending OK response for non-existence.\n", objectstatus.title);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
    else
    {
        metadata_t *metadata = &object->metadata;
        metadata->next_metadata = NULL;
        LOG(LDEBUG, "Sending metadata for OHFI %d.\n", object->metadata.ohfi);

        if (VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK)
        {
            LOG(LERROR, "Error sending metadata for %d\n", object->metadata.ohfi);
        }
        else
        {
            VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
        }
    }

    unlockDatabase();
    free(objectstatus.title);
}

void vitaEventSendObjectThumb(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectThumb", event->Code, eventId);
    char thumbpath[PATH_MAX];
    uint32_t ohfi = event->Param2;
    lockDatabase();
    struct cma_object *object = ohfiToObject(ohfi);

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot find OHFI %d in database.\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    thumbpath[0] = '\0';

    if (MASK_SET(object->metadata.dataType, Photo))
    {
        // TODO: Don't send full image (may be too large)
        LOG(LDEBUG, "Sending photo as thumbnail %s", object->metadata.path);
        strcpy(thumbpath, object->path);
    }
    else if (MASK_SET(object->metadata.dataType, SaveData))
    {
        LOG(LDEBUG, "Sending savedata thumbnail %s\n", thumbpath);
        sprintf(thumbpath, "%s/%s", object->path, "ICON0.PNG");
    }
    else
    {
        LOG(LERROR, "Thumbnail sending for the file %s is not supported.\n", object->metadata.path);
        unlockDatabase();
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);
        return;
    }

    unlockDatabase();
    // TODO: Get thumbnail data correctly
    unsigned char *data;
    unsigned int len = 0;

    if (readFileToBuffer(thumbpath, 0, &data, &len) < 0)
    {
        LOG(LERROR, "Cannot find thumbnail %s\n", thumbpath);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);
        return;
    }

    if (VitaMTP_SendObjectThumb(device, eventId, (metadata_t *)&g_thumbmeta, data, len) != PTP_RC_OK)
    {
        LOG(LERROR, "Error sending thumbnail %s\n", thumbpath);
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(data);
}

void vitaEventDeleteObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestDeleteObject", event->Code, eventId);
    int ohfi = event->Param2;
    lockDatabase();
    struct cma_object *object = ohfiToObject(ohfi);

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "OHFI %d not found.\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    struct cma_object *parent = ohfiToObject(object->metadata.ohfiParent);

    deleteAll(object->path);

    LOG(LINFO, "Deleted %s\n", object->metadata.path);

    removeFromDatabase(ohfi, parent);

    unlockDatabase();

    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventGetSettingInfo(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetSettingInfo", event->Code, eventId);
    settings_info_t *settingsinfo;

    if (VitaMTP_GetSettingInfo(device, eventId, &settingsinfo) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to get setting info from Vita.\n");
        return;
    }

    LOG(LVERBOSE, "Current account id: %s\n", settingsinfo->current_account.accountId);
    free(g_uuid);
    g_uuid = strdup(settingsinfo->current_account.accountId);
    // set the database to be updated ASAP
    sem_post(g_refresh_database_request);
    // free all the information
    VitaMTP_Data_Free_Settings(settingsinfo);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void vitaEventSendHttpObjectPropFromURL(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendHttpObjectPropFromURL", event->Code, eventId);
    char *url = NULL;
    unsigned char *data;
    http_object_prop_t httpobjectprop;

    if (VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to get URL.\n");
        return;
    }

    if (requestURL(url, &data, (unsigned int *)&httpobjectprop.size) < 0)
    {
        LOG(LERROR, "Failed to read data for %s\n", url);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Download);
        free(url);
        return;
    }

    free(data);
    httpobjectprop.timestamp = NULL; // TODO: Actually get timestamp
    httpobjectprop.timestamp_len = 0;

    if (VitaMTP_SendHttpObjectPropFromURL(device, eventId, &httpobjectprop) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to send object properties.\n");
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(url);
}

void vitaEventSendPartOfObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendPartOfObject", event->Code, eventId);
    send_part_init_t part_init;

    if (VitaMTP_SendPartOfObjectInit(device, eventId, &part_init) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get information on object to send.\n");
        return;
    }

    lockDatabase();
    struct cma_object *object = ohfiToObject(part_init.ohfi);

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot find object for OHFI %d\n", part_init.ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }

    unsigned char *data;
    unsigned int len = (unsigned int)part_init.size;

    if (readFileToBuffer(object->path, part_init.offset, &data, &len) < 0)
    {
        LOG(LERROR, "Cannot read %s.\n", object->path);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
        unlockDatabase();
        return;
    }

    LOG(LINFO, "Sending %s at file offset %llu for %llu bytes\n", object->metadata.path, part_init.offset, part_init.size);
    unlockDatabase();

    if (VitaMTP_SendPartOfObject(device, eventId, data, len) != PTP_RC_OK)
    {
        LOG(LERROR, "Failed to send part of object OHFI %d\n", part_init.ohfi);
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(data);
}

void vitaEventOperateObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestOperateObject", event->Code, eventId);
    operate_object_t operateobject;

    if (VitaMTP_OperateObject(device, eventId, &operateobject) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get information on object to operate.\n");
        return;
    }

    lockDatabase();
    struct cma_object *root = ohfiToObject(operateobject.ohfi);
    struct cma_object *newobj;
    // for renaming only
    char *origFullPath;
    char *origName;

    // end for renaming only
    if (root == NULL)
    {
        unlockDatabase();
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }

    // do command, 1 = create folder for ex
    switch (operateobject.cmd)
    {
    case VITA_OPERATE_CREATE_FOLDER:
        LOG(LDEBUG, "Operate command %d: Create folder %s\n", operateobject.cmd, operateobject.title);
        newobj = addToDatabase(root, operateobject.title, 0, Folder);

        if (createNewDirectory(newobj->path) < 0)
        {
            removeFromDatabase(newobj->metadata.ohfi, root);
            LOG(LERROR, "Unable to create temporary folder: %s\n", operateobject.title);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
        }

        LOG(LINFO, "Created folder %s\n", newobj->metadata.path);
        LOG(LVERBOSE, "Folder %s with OHFI %d under parent %s\n", newobj->path, newobj->metadata.ohfi, root->metadata.path);
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
        break;

    case VITA_OPERATE_CREATE_FILE:
        LOG(LDEBUG, "Operate command %d: Create file %s\n", operateobject.cmd, operateobject.title);
        newobj = addToDatabase(root, operateobject.title, 0, File);

        if (createNewFile(newobj->path) < 0)
        {
            removeFromDatabase(newobj->metadata.ohfi, root);
            LOG(LERROR, "Unable to create temporary file: %s\n", operateobject.title);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
        }

        LOG(LINFO, "Created file %s\n", newobj->metadata.path);
        LOG(LVERBOSE, "File %s with OHFI %d under parent %s\n", newobj->path, newobj->metadata.ohfi, root->metadata.path);
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
        break;

    case VITA_OPERATE_RENAME:
        LOG(LDEBUG, "Operate command %d: Rename %s to %s\n", operateobject.cmd, root->metadata.name, operateobject.title);
        origName = strdup(root->metadata.name);
        origFullPath = strdup(root->path);
        // rename in database
        renameRootEntry(root, NULL, operateobject.title);

        // rename in filesystem
        if (move(origFullPath, root->path) < 0)
        {
            // report the failure
            LOG(LERROR, "Unable to rename %s to %s\n", origName, operateobject.title);
            // rename back
            renameRootEntry(root, NULL, origName);
            // free old data
            free(origFullPath);
            free(origName);
            // send result
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
        }

        LOG(LINFO, "Renamed %s to %s\n", origName, root->metadata.path);
        LOG(LVERBOSE, "Renamed OHFI %d from %s to %s\n", root->metadata.ohfi, origName, root->metadata.name);
        // free old data
        free(origFullPath);
        free(origName);
        // send result
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, root->metadata.ohfi);
        break;

    default:
        LOG(LERROR, "Operate command %d: Not implemented.\n", operateobject.cmd);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        break;

    }

    unlockDatabase();
    free(operateobject.title);
}

void vitaEventGetPartOfObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetPartOfObject", event->Code, eventId);
    unsigned char *data;
    send_part_init_t part_init;

    if (VitaMTP_GetPartOfObject(device, eventId, &part_init, &data) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get object from device.\n");
        return;
    }

    lockDatabase();
    struct cma_object *object = ohfiToObject(part_init.ohfi);

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot find OHFI %d.\n", part_init.ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        free(data);
        return;
    }

    LOG(LINFO, "Receiving %s at offset %llu for %llu bytes\n", object->metadata.path, part_init.offset, part_init.size);

    if (writeFileFromBuffer(object->path, part_init.offset, data, part_init.size) < 0)
    {
        LOG(LERROR, "Cannot write to file %s.\n", object->path);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
    }
    else
    {
        // add size to all parents
        incrementSizeMetadata(object, part_init.size);
        LOG(LDEBUG, "Written %llu bytes to %s at offset %llu.\n", part_init.size, object->path, part_init.offset);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    unlockDatabase();
    free(data);
}

void vitaEventSendStorageSize(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendStorageSize", event->Code, eventId);
    int ohfi = event->Param2;
    lockDatabase();
    struct cma_object *object = ohfiToObject(ohfi);
    uint64_t total;
    uint64_t free;

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot find OHFI %d.\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    if (!fileExists(object->path))
    {
        LOG(LINFO, "Creating %s\n", object->path);

        if (createNewDirectory(object->path) < 0)
        {
            unlockDatabase();
            LOG(LERROR, "Create directory failed.\n");
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
            return;
        }
    }

    if (getDiskSpace(object->path, &free, &total) < 0)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot get disk space.\n");
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
        return;
    }

    unlockDatabase();
    LOG(LVERBOSE, "For drive containing OHFI %d, free: %llu, total: %llu\n", ohfi, free, total);

    if (VitaMTP_SendStorageSize(device, eventId, total, free) != PTP_RC_OK)   // Send fake 50GB/100GB
    {
        LOG(LERROR, "Send storage size failed.\n");
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void vitaEventCheckExistance(vita_device_t *device, vita_event_t *event, int eventId)    // [sic]
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestCheckExistance [sic]", event->Code, eventId);
    int handle = event->Param2;
    existance_object_t existance;
    struct cma_object *object;

    if (VitaMTP_CheckExistance(device, handle, &existance) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot read information on object to be sent.\n");
        return;
    }

    lockDatabase();

    if ((object = pathToObject(existance.name, 0)) == NULL)
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Different_Object);
    }
    else
    {
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_VITA_Same_Object, object->metadata.ohfi);
    }

    unlockDatabase();
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

uint16_t vitaGetAllObjects(vita_device_t *device, int eventId, struct cma_object *parent, uint32_t handle)
{
    union
    {
        unsigned char *fileData;
        uint32_t *handles;
    } data;
    unsigned int length;
    metadata_t tempMeta;
    struct cma_object *object;
    struct cma_object *temp;
    unsigned int i;
    uint16_t ret;

    if (VitaMTP_GetObject(device, handle, &tempMeta, (void **)&data, &length) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get object for handle %d.\n", handle);
        return PTP_RC_VITA_Invalid_Data;
    }

    lockDatabase();

    if ((object = addToDatabase(parent, tempMeta.name, 0, tempMeta.dataType)) == NULL)    // size will be added after read
    {
        unlockDatabase();
        LOG(LERROR, "Cannot add object %s to database.\n", tempMeta.name);
        free(tempMeta.name);
        free(data.fileData);
        return PTP_RC_VITA_Invalid_Data;
    }

    object->metadata.handle = tempMeta.handle;
    free(tempMeta.name);  // not needed anymore, copy in object

    if ((temp = pathToObject(object->metadata.name, parent->metadata.ohfi)) != object
            && temp != NULL)    // check if object exists already
    {
        // delete existing file/folder
        LOG(LDEBUG, "Deleting %s\n", temp->path);
        deleteAll(temp->path);
        removeFromDatabase(temp->metadata.ohfi, parent);
    }

    if (object->metadata.dataType & File)
    {
        LOG(LINFO, "Receiving %s for %lu bytes.\n", object->metadata.path, tempMeta.size);
        
        if (createNewFile(object->path) < 0 || writeFileFromBuffer(object->path, 0, data.fileData, tempMeta.size) < 0)
        {
            LOG(LERROR, "Cannot write to %s.\n", object->path);
            removeFromDatabase(object->metadata.ohfi, parent);
            unlockDatabase();
            free(data.fileData);
            return PTP_RC_VITA_Invalid_Permission;
        }

        incrementSizeMetadata(object, tempMeta.size);
    }
    else if (object->metadata.dataType & Folder)
    {
        LOG(LINFO, "Receiving directory %s\n", object->metadata.path);

        if (createNewDirectory(object->path) < 0)
        {
            removeFromDatabase(object->metadata.ohfi, parent);
            LOG(LERROR, "Cannot create directory: %s\n", object->path);
            unlockDatabase();
            free(data.fileData);
            return PTP_RC_VITA_Failed_Operate_Object;
        }

        for (i = 0; i < length; i++)
        {
            ret = vitaGetAllObjects(device, eventId, object, data.handles[i]);

            if (ret != PTP_RC_OK)
            {
                removeFromDatabase(object->metadata.ohfi, parent);
                unlockDatabase();
                free(data.fileData);
                return ret;
            }
        }
    }
    else
    {
        LOG(LERROR, "Invalid object.\n"); // should not be here
    }

    unlockDatabase();
    free(data.fileData);
    return PTP_RC_OK;
}

void vitaEventGetTreatObject(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestGetTreatObject", event->Code, eventId);
    treat_object_t treatObject;
    struct cma_object *parent;

    if (VitaMTP_GetTreatObject(device, eventId, &treatObject) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get information on object to get.\n");
        return;
    }

    if ((parent = ohfiToObject(treatObject.ohfiParent)) == NULL)
    {
        LOG(LERROR, "Cannot find parent OHFI %d.\n", treatObject.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    VitaMTP_ReportResult(device, eventId, vitaGetAllObjects(device, eventId, parent, treatObject.handle));
}

void vitaEventSendCopyConfirmationInfo(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendCopyConfirmationInfo", event->Code, eventId);
    copy_confirmation_info_t *info;
    struct cma_object *object;

    if (VitaMTP_SendCopyConfirmationInfoInit(device, eventId, &info) != PTP_RC_OK)
    {
        LOG(LERROR, "Error recieving initial information.\n");
        return;
    }

    lockDatabase();
    uint32_t i;
    uint64_t size = 0;

    for (i = 0; i < info->count; i++)
    {
        if ((object = ohfiToObject(info->ohfi[i])) == NULL)
        {
            LOG(LERROR, "Cannot find OHFI %d.\n", info->ohfi[i]);
            free(info);
            unlockDatabase();
            return;
        }

        size += object->metadata.size;
    }

    unlockDatabase();

    if (VitaMTP_SendCopyConfirmationInfo(device, eventId, info, size) != PTP_RC_OK)
    {
        LOG(LERROR, "Error sending copy confirmation.\n");
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(info);
}

void vitaEventSendObjectMetadataItems(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendObjectMetadataItems", event->Code, eventId);
    uint32_t ohfi;

    if (VitaMTP_SendObjectMetadataItems(device, eventId, &ohfi) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot get OHFI for retreving metadata.\n");
        return;
    }

    lockDatabase();
    struct cma_object *object = ohfiToObject(ohfi);

    if (object == NULL)
    {
        unlockDatabase();
        LOG(LERROR, "Cannot find OHFI %d in database\n", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    metadata_t *metadata = &object->metadata;
    metadata->next_metadata = NULL;
    LOG(LVERBOSE, "Sending metadata for OHFI %d (%s)\n", ohfi, metadata->path);

    if (VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK)
    {
        LOG(LERROR, "Error sending metadata.\n");
    }
    else
    {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    unlockDatabase();
}

void vitaEventSendNPAccountInfo(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestSendNPAccountInfo", event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    LOG(LERROR, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventRequestTerminate(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LVERBOSE, "Event recieved: %s, code: 0x%x, id: %d\n", "RequestTerminate", event->Code, eventId);
    LOG(LERROR, "Event 0x%x unimplemented!\n", event->Code);
}

void vitaEventUnimplementated(vita_device_t *device, vita_event_t *event, int eventId)
{
    LOG(LERROR, "Unknown event not handled, code: 0x%x, id: %d\n", event->Code, eventId);
    LOG(LDEBUG, "Param1: 0x%08X, Param2: 0x%08X, Param3: 0x%08X\n", event->Param1, event->Param2, event->Param3);
}

static void *vitaEventListener(void* args)
{
    vita_device_t *device = (vita_device_t *)args;
    vita_event_t event;
    int slot;

    while (g_connected)
    {
        if (VitaMTP_Read_Event(device, &event) < 0)
        {
            LOG(LERROR, "Error reading event from Vita.\n");
            g_connected = 0;
            continue;
        }

        slot = event.Code - PTP_EC_VITA_RequestSendNumOfObject;

        if (slot < 0 || slot > sizeof(g_event_processes)/sizeof(void *))
        {
            slot = sizeof(g_event_processes)/sizeof(void *) - 1;  // last item is pointer to "unimplemented
        }

        LOG(LDEBUG, "Event 0x%04X recieved, slot %d with function address %p\n", event.Code, slot, g_event_processes[slot]);
        g_event_processes[slot](device, &event, event.Param1);
    }

    return NULL;
}

static vita_device_t *connect_usb()
{
    vita_device_t *device;
    LOG(LDEBUG, "Looking for USB device...\n");

    for (int i = 1; i <= OPENCMA_CONNECTION_TRIES; i++)
    {
        // This will do MTP initialization if the device is found
        if ((device = VitaMTP_Get_First_USB_Vita()) != NULL)
        {
            break;
        }
        LOG(LINFO, "No Vita found. Attempt %d of %d.\n", i, OPENCMA_CONNECTION_TRIES);
        sleep(10);
    }

    return device;
}

static void *broadcast_server(void *args)
{
    LOG(LDEBUG, "Starting CMA wireless broadcast...\n");
    wireless_host_info_t *info = (wireless_host_info_t *)args;

    if (VitaMTP_Broadcast_Host(info, 0) < 0)
    {
        LOG(LERROR, "An error occured during broadcast. Exiting.\n");
        exit(1);
    }

    LOG(LDEBUG, "Broadcast ended.\n");
    return NULL;
}

static int device_registered(const char *deviceid)
{
    LOG(LDEBUG, "Got connection request from %s\n", deviceid);
    return 1;
}

static int generate_pin(wireless_vita_info_t *info, int *p_err)
{
    LOG(LDEBUG, "Registration request from %s (MAC: %s)\n", info->name, info->mac_addr);
    int pin = rand() % 100000000;
    fprintf(stderr, "Your registration PIN for %s is: %08d\n", info->name, pin);
    return pin;
}

static vita_device_t *connect_wireless()
{
    vita_device_t *device;
    wireless_host_info_t info = {"00000000-0000-0000-0000-000000000000", "win", OPENCMA_VERSION_STRING, OPENCMA_REQUEST_PORT};
    pthread_t broadcast_thread;

    if (pthread_create(&broadcast_thread, NULL, broadcast_server, &info) < 0)
    {
        LOG(LERROR, "Cannot create broadcast thread\n");
        return NULL;
    }

    for (int i = 1; i <= OPENCMA_CONNECTION_TRIES; i++)
    {
        if ((device = VitaMTP_Get_First_Wireless_Vita(&info, 0, 0, device_registered, generate_pin)) != NULL)
        {
            break;
        }
        LOG(LINFO, "Error connecting to device. Attempt %d of %d.\n", i, OPENCMA_CONNECTION_TRIES);
    }

    VitaMTP_Stop_Broadcast();
    LOG(LDEBUG, "Waiting for broadcast thread to complete and exit.\n");
    if (pthread_join(broadcast_thread, NULL) < 0)
    {
        LOG(LERROR, "Error joining broadcast thread.\n");
        return NULL;
    }
    return device;
}

static void *handle_commands(void *args)
{
    char cmd[10];
    do
    {
        fscanf(stdin, "%10s", cmd);
        if (strcmp("help", cmd) == 0) {
            fprintf(stderr, "%s\n", g_commands_help_string);
        }
        else if (strcmp("exit", cmd) == 0)
        {
            if (!g_connected)
            {
                LOG(LINFO, "No active connection.\n");
            }
            else
            {
                LOG(LINFO, "Stopping event listener.\n");
                g_connected = 0;
                sem_post(g_refresh_database_request);  // so we stop waiting
            }
        }
        else if (strcmp("refresh", cmd) == 0)
        {
            if (!g_connected)
            {
                LOG(LINFO, "No active connection, ignoring request to refresh database.\n");
                return NULL;
            }
            
            LOG(LINFO, "Refreshing the database.\n");
            // SIGTSTP will be used for a user request to refresh the database
            sem_post(g_refresh_database_request);
            // TODO: For some reason SIGTSTP automatically unlocks the semp.
        }
        else
        {
            LOG(LERROR, "Unknown command: %s\n", cmd);
        }
    }
    while (1);
    return NULL;
}

int main(int argc, char **argv)
{
    /* Seed the random number generator */
    srand((unsigned int)time(NULL));
    /* Parse the command line arguments */
    int wireless = 0;

    // Start with some default values
    g_uuid = strdup("ffffffffffffffff");
    g_paths.urlPath = ".";
    g_paths.photosPath = NULL;
    g_paths.videosPath = NULL;
    g_paths.musicPath = NULL;
    g_paths.appsPath = NULL;
    g_paths.packagesPath = "package";

    if (argc > 2 && argv[1][0] != '-')
    {
        if (strcmp("wireless", argv[1]) == 0)
        {
            wireless = 1;
        }
        else if (strcmp("usb", argv[1]) == 0)
        {
            wireless = 0;
        }
        else
        {
            LOG(LERROR, "Invalid mode '%s'\n", argv[1]);
            fprintf(stderr, "%s\n", g_help_string);
            return 1;
        }

        argc--;
        argv++;
    }

    // Get the arguments
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "u:p:v:m:a:k:l:hd")) != -1)
    {
        switch (c)
        {
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
                
        case 'k': // packages path
            g_paths.packagesPath = optarg;
            break;

        case 'l': // logging
            g_log_level = atoi(optarg);

            if (g_log_level > 4) g_log_level = 4;

            g_log_level = (1 << g_log_level) - 1; // convert to mask
            break;

        case 'h':
        case '?':
        default:
            fprintf(stderr, "%s\n", g_help_string);
            return 1;
            break;
        }
    }

    /* Make sure paths are specified */
    if (!(g_paths.photosPath && g_paths.videosPath && g_paths.musicPath && g_paths.appsPath))
    {
        LOG(LERROR, "Not enough arguments. Please specify all paths\n");
        fprintf(stderr, "%s\n", g_help_string);
        return 1;
    }

    /* Check if folders exist */
    if (!fileExists(g_paths.urlPath))
    {
        LOG(LERROR, "Cannot find path: %s\n", g_paths.urlPath);
        return 1;
    }

    if (!fileExists(g_paths.photosPath))
    {
        LOG(LERROR, "Cannot find path: %s\n", g_paths.photosPath);
        return 1;
    }

    if (!fileExists(g_paths.videosPath))
    {
        LOG(LERROR, "Cannot find path: %s\n", g_paths.videosPath);
        return 1;
    }

    if (!fileExists(g_paths.musicPath))
    {
        LOG(LERROR, "Cannot find path: %s\n", g_paths.musicPath);
        return 1;
    }

    if (!fileExists(g_paths.appsPath))
    {
        LOG(LERROR, "Cannot find path: %s\n", g_paths.appsPath);
        return 1;
    }

    // Show information string
    fprintf(stderr, "%s\nlibVitaMTP Version: %d.%d\nProtocol Max Version: %08d\n",
            OPENCMA_VERSION_STRING, VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR, VITAMTP_PROTOCOL_MAX_VERSION);

    /* Set up the database */
#ifdef __APPLE__
    if ((g_refresh_database_request = sem_open("/opencma_refresh_db", O_CREAT, 0777, 0)) == SEM_FAILED)
#else
    g_refresh_database_request = &g_sem_storage;
    if (sem_init(g_refresh_database_request, 0, 0) < 0)
#endif
    {
        LOG(LERROR, "Cannot create semaphore for event flag.\n");
        return 1;
    }

    LOCK_SEMAPHORE(g_refresh_database_request);  // make sure it's locked

    /* Now, we can set up the device */

    // This lets us have detailed logs including dumps of MTP packets
    VitaMTP_Set_Logging(g_log_level);

    vita_device_t *device;

    // Wait for the device to be plugged in
    LOG(LINFO, "Waiting for Vita to connect...\n");

    if (wireless)
    {
        device = connect_wireless();
    }
    else
    {
        device = connect_usb();
    }

    if (device == NULL)
    {
        LOG(LERROR, "Error connecting.\n");
        return 1;
    }

    LOG(LINFO, "Vita connected: id %s\nType in 'help' for list of commands.\n", VitaMTP_Get_Identification(device));

    // Here we will do Vita specific initialization
    vita_info_t vita_info;
    // This will automatically fill pc_info with default information
    const initiator_info_t *pc_info;
    // Capability information is both sent from the Vita and the PC
    capability_info_t *vita_capabilities;
    capability_info_t *pc_capabilities = generate_pc_capability_info();

    // First, we get the Vita's info
    if (VitaMTP_GetVitaInfo(device, &vita_info) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot retreve device information.\n");
        return 1;
    }

    if (vita_info.protocolVersion > VITAMTP_PROTOCOL_MAX_VERSION)
    {
        LOG(LERROR, "Vita wants protocol version %08d while we only support %08d. Attempting to continue.\n",
            vita_info.protocolVersion, VITAMTP_PROTOCOL_MAX_VERSION);
    }

    pc_info = VitaMTP_Data_Initiator_New(OPENCMA_VERSION_STRING, vita_info.protocolVersion);

    // Next, we send the client's (this program) info (discard the const here)
    if (VitaMTP_SendInitiatorInfo(device, (initiator_info_t *)pc_info) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot send host information.\n");
        return 1;
    }

    if (vita_info.protocolVersion >= VITAMTP_PROTOCOL_FW_2_10)
    {
        // Get the device's capabilities
        if (VitaMTP_GetVitaCapabilityInfo(device, &vita_capabilities) != PTP_RC_OK)
        {
            LOG(LERROR, "Failed to get capability information from Vita.\n");
            return 1;
        }

        VitaMTP_Data_Free_Capability(vita_capabilities); // TODO: Use this data

        // Send the host's capabilities
        if (VitaMTP_SendPCCapabilityInfo(device, pc_capabilities) != PTP_RC_OK)
        {
            LOG(LERROR, "Failed to send capability information to Vita.\n");
            return 1;
        }
    }

    // Finally, we tell the Vita we are connected
    if (VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_Connected) != PTP_RC_OK)
    {
        LOG(LERROR, "Cannot send host status.\n");
        return 1;
    }

    // Create the event listener thread, technically we do not
    // need a seperate thread to do this since the main thread
    // is not doing anything else, but to make the example
    // complete, we will assume the main thread has more
    // important things.
    pthread_t event_thread;
    // The command handler thread allows the user to modify the
    // behavior while OpenCMA is running.
    pthread_t command_thread;
    g_connected = 1;
    
    if (pthread_create(&event_thread, NULL, vitaEventListener, device) != 0)
    {
        LOG(LERROR, "Cannot create event listener thread.\n");
        return 1;
    }
    
    if (pthread_create(&command_thread, NULL, handle_commands, NULL) != 0)
    {
        LOG(LERROR, "Cannot create command handler thread.\n");
        return 1;
    }
    
    if (pthread_detach(event_thread) < 0)
    {
        LOG(LERROR, "Cannot detatch event thread.\n");
        return 1;
    }
    
    if (pthread_detach(command_thread) < 0)
    {
        LOG(LERROR, "Cannot detatch command handler thread.\n");
        return 1;
    }

    // Free dynamically obtained data
    VitaMTP_Data_Free_Initiator(pc_info);
    free_pc_capability_info(pc_capabilities);

    // this thread will update the database when needed
    while (g_connected)
    {
        sem_wait(g_refresh_database_request);

        if (!g_connected)   // TODO: more clean way of doing this
        {
            break;
        }

        LOG(LINFO, "Refreshing database for user %s (this may take some time)...\n", g_uuid);
        LOG(LDEBUG, "URL Mapping Path: %s\nPhotos Path: %s\nVideos Path: %s\nMusic Path: %s\nApps Path: %s\nPackages Path: %s\n",
            g_paths.urlPath, g_paths.photosPath, g_paths.videosPath, g_paths.musicPath, g_paths.appsPath, g_paths.packagesPath);
        destroyDatabase();
        createDatabase(&g_paths, g_uuid);
        LOG(LINFO, "Database refreshed.\n");
        LOCK_SEMAPHORE(g_refresh_database_request);  // in case multiple requests were made
    }

    LOG(LINFO, "Shutting down...\n");

    // End this connection with the Vita
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);

    // Clean up our mess
    VitaMTP_Release_Device(device);
    destroyDatabase();
#ifdef __APPLE__
    sem_close(g_refresh_database_request);
    sem_unlink("/opencma_refresh_db");
#else
    sem_destroy(g_refresh_database_request);
#endif

    LOG(LINFO, "Exiting.\n");

    return 0;
}
