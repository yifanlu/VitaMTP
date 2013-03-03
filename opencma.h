//
//  Reference implementation of libvitamtp
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

#ifndef VitaMTP_opencma_h
#define VitaMTP_opencma_h

#define OPENCMA_VERSION_STRING "0.2 alpha"
// Our object ids will start at 1000 to prevent conflict with the master ohfi
#define OHFI_OFFSET 1000

struct cma_object {
    metadata_t metadata;
    struct cma_object *next_object; // should be the same as metadata.next_metadata
    char *path; // path of the object
};

struct cma_database {
    struct cma_object photos;
    struct cma_object videos;
    struct cma_object music;
    struct cma_object vitaApps;
    struct cma_object pspApps;
    struct cma_object pspSaves;
    struct cma_object psxApps;
    struct cma_object psmApps;
    struct cma_object backups;
};

typedef void (*vita_event_process_t)(LIBMTP_mtpdevice_t*,LIBMTP_event_t*,int);

/* Process events */
void vitaEventSendNumOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendObjectMetadata (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventCancelTask (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendHttpObjectFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendObjectStatus (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendObjectThumb (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventDeleteObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventGetSettingInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendHttpObjectPropFromURL (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventOperateObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventGetPartOfObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendStorageSize (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventCheckExistance (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventGetTreatObject (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendCopyConfirmationInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendObjectMetadataItems (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventSendNPAccountInfo (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventRequestTerminate (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);
void vitaEventUnimplementated (LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, int eventId);

void *vitaEventListener(LIBMTP_mtpdevice_t *device);

/* Database functions */
void refreshDatabase(void);
void destroyDatabase(void);
void createDatabase(void);
void addEntriesForDirectory (struct cma_object *current, int parent_ohfi);
struct cma_object *addToDatabase (struct cma_object *root, const char *name, size_t size, const enum DataType type);
void removeFromDatabase (int ohfi, struct cma_object *start);
struct cma_object *ohfiToObject(int ohfi);
struct cma_object *titleToObject(char *title, int ohfiRoot);
int filterObjects (int ohfiParent, metadata_t **p_head);

#endif
