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

#include <vitamtp.h>

// forward reference
struct stat;
struct FTW;

#define OPENCMA_VERSION_STRING "OpenCMA 2.1 Beta"
#define OPENCMA_REQUEST_PORT   9309
#define OPENCMA_CONNECTION_TRIES    10
// Our object ids will start at 1000 to prevent conflict with the master ohfi
#define OHFI_OFFSET 1000

#define LDEBUG       VitaMTP_DEBUG
#define LVERBOSE     VitaMTP_VERBOSE
#define LINFO        VitaMTP_INFO
#define LERROR       VitaMTP_ERROR
#define LNONE        VitaMTP_NONE

#define LOG(mask,format,args...) if (MASK_SET (g_log_level, mask)) fprintf (stderr, "%s: " format, __FUNCTION__, ## args)

extern unsigned int g_log_level;

struct cma_object
{
    metadata_t metadata;
    struct cma_object *next_object;
    char *path; // path of the object
    int num_filters;
    metadata_t *filters;
};

struct cma_database
{
    struct cma_object photos;
    struct cma_object videos;
    struct cma_object music;
#ifndef NO_PACKAGE_INSTALLER
    struct cma_object packages;
#endif
    struct cma_object vitaApps;
    struct cma_object pspApps;
    struct cma_object pspSaves;
    struct cma_object psxApps;
    struct cma_object psmApps;
    struct cma_object backups;
};

struct cma_paths
{
    const char *urlPath;
    const char *photosPath;
    const char *videosPath;
    const char *musicPath;
    const char *appsPath;
#ifndef NO_PACKAGE_INSTALLER
    const char *packagesPath;
#endif
};

typedef void (*vita_event_process_t)(vita_device_t *,vita_event_t *,int);

/* Process events */
void vitaEventSendNumOfObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendObjectMetadata(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventCancelTask(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendHttpObjectFromURL(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendObjectStatus(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendObjectThumb(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventDeleteObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventGetSettingInfo(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendHttpObjectPropFromURL(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendPartOfObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventOperateObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventGetPartOfObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendStorageSize(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventCheckExistance(vita_device_t *device, vita_event_t *event, int eventId);
uint16_t vitaGetAllObjects(vita_device_t *device, int eventId, struct cma_object *parent, uint32_t handle);
void vitaEventGetTreatObject(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendCopyConfirmationInfo(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendObjectMetadataItems(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventSendNPAccountInfo(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventRequestTerminate(vita_device_t *device, vita_event_t *event, int eventId);
void vitaEventUnimplementated(vita_device_t *device, vita_event_t *event, int eventId);

/* Database functions */
void createDatabase(struct cma_paths *paths, const char *uuid);
void destroyDatabase(void);
void lockDatabase(void);
void unlockDatabase(void);
struct cma_object *addToDatabase(struct cma_object *root, const char *name, size_t size, const enum DataType type);
void createFilter(struct cma_object *dirobject, metadata_t *output, const char *name, int type);
void removeFromDatabase(int ohfi, struct cma_object *start);
void renameRootEntry(struct cma_object *object, const char *name, const char *newname);
struct cma_object *ohfiToObject(int ohfi);
struct cma_object *pathToObject(char *path, int ohfiParent);
int filterObjects(int ohfiParent, metadata_t **p_head);

/* Utility functions */
int createNewDirectory(const char *path);
int createNewFile(const char *name);
int readFileToBuffer(const char *name, size_t seek, unsigned char **p_data, unsigned int *p_len);
int writeFileFromBuffer(const char *name, size_t seek, unsigned char *data, size_t len);
int deleteEntry(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw);
void deleteAll(const char *path);
int move(const char *src, const char *dest);
int fileExists(const char *path);
int getDiskSpace(const char *path, uint64_t *free, uint64_t *total);
void addEntriesForDirectory(struct cma_object *current, int parent_ohfi);
int requestURL(const char *url, unsigned char **p_data, unsigned int *p_len);
char *strreplace(const char *haystack, const char *find, const char *replace);
capability_info_t *generate_pc_capability_info(void);
void free_pc_capability_info(capability_info_t *info);

#endif
