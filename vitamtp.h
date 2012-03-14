//
//  Library for interacting with Vita's MTP connection
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

#ifndef VitaMTP_h
#define VitaMTP_h

#include <libmtp.h>
#include <ptp.h>

/**
 * Contains protocol version, Vita's system version, 
 * and recommended thumbnail sizes.
 * 
 * @see VitaMTP_GetVitaInfo()
 */
struct vita_info {
    char responderVersion[6]; // max: XX.XX\0
    int protocolVersion;
    struct {
        int type;
        int codecType;
        int width;
        int height;
    } photoThumb;
    struct {
        int type;
        int codecType;
        int width;
        int height;
        int duration;
    } videoThumb;
    struct {
        int type;
        int codecType;
        int width;
        int height;
    } musicThumb;
    struct {
        int type;
        int codecType;
        int width;
        int height;
    } gameThumb;
};

/**
 * Contains information about the PC client.
 * Use new_initiator_info() to create and free_initiator_info() 
 * to free.
 * 
 * 
 * @see VitaMTP_SendInitiatorInfo()
 */
struct initiator_info {
    char *platformType;
    char *platformSubtype;
    char *osVersion;
    char *version;
    int protocolVersion;
    char *name;
    int applicationType;
};

/**
 * A linked list of accounts on the Vita.
 * Currently unimplemented by the Vita.
 * 
 * 
 * @see VitaMTP_GetSettingInfo()
 */
struct settings_info {
    struct account { // remember to free each string!
        char *userName;
        char *signInId;
        char *accountId;
        char *countryCode;
        char *langCode;
        char *birthday;
        char *onlineUser;
        char *passwd;
        struct account *next_account;
    } current_account;
};

/* Unknown struct
 *     Received for PSP saves :
 *         0E 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
 *         00 00 00 00 21 00 00 00  00 00 00 00 01 00 00 00
 *     Received for PSP games :
 *     Received for Vita applications :
 */
struct browse_info {
    uint32_t ohfi;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
    uint32_t unk6;
    uint32_t unk7;
};

/**
 * Used by the metadata structure.
 */
enum DataType {
    Folder,
    File,
    SaveData,
    Thumbnail
};

/**
 * A linked list of metadata for objects.
 * The items outside of the union MUST be set for all 
 * data types. After setting dataType, you are required 
 * to fill the data in the union for that type.
 * 
 * The ohfi is a unique id that identifies an object. This 
 * id is used by the Vita to request objects.
 * The title is what is shown on the screen on the Vita.
 * The index is the order that objects are shown on screen.
 * 
 * @see VitaMTP_SendObjectMetadata()
 */
struct metadata {
    int ohfiParent;
    int ohfi;
    char* title;
    int index;
    long dateTimeCreated; // unix timestamp
    uint64_t size;
    enum DataType dataType;
    
    union {
        struct folder {
            int type;
            char* name;
        } folder;
        
        struct file {
            char* name;
            int statusType;
        } file;
        
        struct saveData {
            char* detail;
            char* dirName;
            char* savedataTitle;
            long dateTimeUpdated; // unix timestamp
            int statusType;
        } saveData;
        
        struct thumbnail {
            int codecType;
            int width;
            int height;
            int type;
            int orientationType;
            float aspectRatio;
            int fromType;
        } thumbnail;
    } data;
    
    struct metadata *next_metadata;
};

/**
 * A request from the Vita to obtain metadata 
 * for the file named.
 * 
 * 
 * @see VitaMTP_SendObjectStatus()
 */
struct object_status {
    uint32_t type;
    char *file;
};

/**
 * A request from the Vita for a part of the object 
 * to send over (using the ohfi id).
 * 
 * 
 * @see VitaMTP_SendPartOfObjectInit()
 */
struct send_part_init {
    int ohfi;
    uint64_t offset;
    uint64_t size;
};

/**
 * Information on a HTTP object request.
 * 
 * 
 * @see VitaMTP_SendHttpObjectPropFromURL()
 */
struct http_object_prop {
    uint64_t size;
    uint8_t timestamp_len;
    char* timestamp;
};

/**
 * These make referring to the structs easier.
 */
typedef struct vita_info vita_info_t;
typedef struct initiator_info initiator_info_t;
typedef struct settings_info settings_info_t;
typedef struct browse_info browse_info_t;
typedef struct metadata metadata_t;
typedef struct thumbnail thumbnail_t;
typedef struct object_status object_status_t;
typedef struct send_part_init send_part_init_t;
typedef struct http_object_prop http_object_prop_t;

/**
 * This is the USB information for the Vita.
 */
#define VITA_PID 0x04E4
#define VITA_VID 0x054C

/**
 * Version information on VitaMTP.
 */
#define VITAMTP_VERSION_MAJOR 1
#define VITAMTP_VERSION_MINOR 0
#define VITAMTP_PROTOCOL_VERSION 1200010

/**
 * PTP event IDs from Sony's Vita extensions to MTP.
 */
#define PTP_EC_VITA_RequestSendNumOfObject 0xC104
#define PTP_EC_VITA_RequestSendObjectMetadata 0xC105
#define PTP_EC_VITA_RequestSendObject 0xC107
#define PTP_EC_VITA_RequestCancelTask 0xC108
#define PTP_EC_VITA_RequestSendHttpObjectFromURL 0xC10B
#define PTP_EC_VITA_Unknown1 0xC10D
#define PTP_EC_VITA_RequestSendObjectStatus 0xC10F
#define PTP_EC_VITA_RequestSendObjectThumb 0xC110
#define PTP_EC_VITA_RequestDeleteObject 0xC111
#define PTP_EC_VITA_RequestGetSettingInfo 0xC112
#define PTP_EC_VITA_RequestSendHttpObjectPropFromURL 0xC113
#define PTP_EC_VITA_RequestSendPartOfObject 0xC115
#define PTP_EC_VITA_RequestOperateObject 0xC117
#define PTP_EC_VITA_RequestGetPartOfObject 0xC118
#define PTP_EC_VITA_RequestSendStorageSize 0xC119
#define PTP_EC_VITA_RequestCheckExistance 0xC120
#define PTP_EC_VITA_RequestGetTreatObject 0xC122
#define PTP_EC_VITA_RequestSendCopyConfirmationInfo 0xC123
#define PTP_EC_VITA_RequestSendObjectMetadataItems 0xC124
#define PTP_EC_VITA_RequestSendNPAccountInfo 0xC125

/**
 * Command IDs from Sony's Vita extensions to MTP.
 */
#define PTP_OC_VITA_Unknown1 0x9510
#define PTP_OC_VITA_GetVitaInfo 0x9511
#define PTP_OC_VITA_SendNumOfObject 0x9513
#define PTP_OC_VITA_GetBrowseInfo 0x9514
#define PTP_OC_VITA_SendObjectMetadata 0x9515
#define PTP_OC_VITA_SendObjectThumb 0x9516
#define PTP_OC_VITA_ReportResult 0x9518
#define PTP_OC_VITA_SendInitiatorInfo 0x951C
#define PTP_OC_VITA_GetUrl 0x951F
#define PTP_OC_VITA_SendHttpObjectFromURL 0x9520
#define PTP_OC_VITA_SendNPAccountInfo 0x9523
#define PTP_OC_VITA_GetSettingInfo 0x9524
#define PTP_OC_VITA_SendObjectStatus 0x9528
#define PTP_OC_VITA_SendHttpObjectPropFromURL 0x9529
#define PTP_OC_VITA_SendHostStatus 0x952A
#define PTP_OC_VITA_SendPartOfObjectInit 0x952B
#define PTP_OC_VITA_SendPartOfObject 0x952C
#define PTP_OC_VITA_OperateObject 0x952E
#define PTP_OC_VITA_GetPartOfObject 0x952F
#define PTP_OC_VITA_SendStorageSize 0x9533
#define PTP_OC_VITA_GetTreatObject 0x9534
#define PTP_OC_VITA_SendCopyConfirmationInfo 0x9535
#define PTP_OC_VITA_SendObjectMetadataItems 0x9536
#define PTP_OC_VITA_SendCopyConfirmationInfoInit 0x9537
#define PTP_OC_VITA_KeepAlive 0x9538

/**
 * Result codes from Sony's Vita extensions to MTP.
 */
#define PTP_RC_Vita_GeneralError 0xA001
#define PTP_RC_VITA_Unknown1 0xA002
#define PTP_RC_VITA_Unknown2 0xA003
#define PTP_RC_VITA_Unknown3 0xA004
#define PTP_RC_VITA_Unknown4 0xA00A
#define PTP_RC_VITA_Unknown5 0xA00D
#define PTP_RC_VITA_Unknown6 0xA008
#define PTP_RC_VITA_Unknown7 0xA010
#define PTP_RC_VITA_Unknown8 0xA012
#define PTP_RC_VITA_Unknown9 0xA017
#define PTP_RC_VITA_Unknown10 0xA018
#define PTP_RC_VITA_Unknown11 0xA01B
#define PTP_RC_VITA_Unknown12 0xA01C
#define PTP_RC_VITA_Unknown13 0xA01F
#define PTP_RC_VITA_Unknown14 0xA020
#define PTP_RC_VITA_Unknown15 0xA027

/**
 * Content types from Sony's Vita extensions to MTP.
 */
#define PTP_OFC_Unknown1 0xB005
#define PTP_OFC_Unknown2 0xB006
#define PTP_OFC_PSPGame 0xB007
#define PTP_OFC_PSPSave 0xB00A
#define PTP_OFC_Unknown3 0xB00B
#define PTP_OFC_Unknown4 0xB00F
#define PTP_OFC_Unknown5 0xB010
#define PTP_OFC_VitaGame 0xB014

/**
 * Host statuses.
 * 
 * @see VitaMTP_SendHostStatus()
 */
#define VITA_HOST_STATUS_Connected 0x0
#define VITA_HOST_STATUS_Unknown1 0x1
#define VITA_HOST_STATUS_Deactivate 0x2
#define VITA_HOST_STATUS_EndConnection 0x3
#define VITA_HOST_STATUS_StartConnection 0x4
#define VITA_HOST_STATUS_Unknown2 0x5

/**
 * Filters for showing objects.
 * 
 * @see VitaMTP_GetBrowseInfo()
 */
#define VITA_BROWSE_MUSIC 0x01
#define VITA_BROWSE_PHOTO 0x02
#define VITA_BROWSE_VIDEO 0x03
#define VITA_BROWSE_APPLICATION 0x0A
#define VITA_BROWSE_PSPGAME 0x0D
#define VITA_BROWSE_PSPSAVE 0x0E

#define VITA_BROWSE_SUBNONE 0x00
#define VITA_BROWSE_SUBFILE 0x01

/**
 * VitaMTP logging levels
 */
#define DEBUG_LOG 0x8000
#define INFO_LOG 0x800000
#define WARNING_LOG 0x1000000
#define ERROR_LOG 0x2000000
#define IS_LOGGING(log) ((log_mask & log) == log)

/**
 * Functions to handle MTP commands
 */
LIBMTP_mtpdevice_t *LIBVitaMTP_Get_First_Vita(void);
uint16_t VitaMTP_SendData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int len);
uint16_t VitaMTP_GetData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int* len);
uint16_t VitaMTP_SendInt32(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, uint32_t value);
uint16_t VitaMTP_GetVitaInfo(LIBMTP_mtpdevice_t *device, vita_info_t *info);
uint16_t VitaMTP_SendNumOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t num);
uint16_t VitaMTP_GetBrowseInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, browse_info_t* info);
uint16_t VitaMTP_SendObjectMetadata(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* metas);
uint16_t VitaMTP_SendObjectThumb(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* meta, unsigned char* thumb_data, uint64_t thumb_len);
uint16_t VitaMTP_ReportResult(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint16_t result);
uint16_t VitaMTP_SendInitiatorInfo(LIBMTP_mtpdevice_t *device, initiator_info_t *info);
uint16_t VitaMTP_GetUrl(LIBMTP_mtpdevice_t *device, uint32_t event_id, char **url);
uint16_t VitaMTP_SendHttpObjectFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, void *data, unsigned int len);
uint16_t VitaMTP_SendNPAccountInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len); // unused?
uint16_t VitaMTP_GetSettingInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, settings_info_t *info);
uint16_t VitaMTP_SendObjectStatus(LIBMTP_mtpdevice_t *device, uint32_t event_id, object_status_t* status);
uint16_t VitaMTP_SendHttpObjectPropFromURL(LIBMTP_mtpdevice_t *device, uint32_t event_id, http_object_prop_t *prop);
uint16_t VitaMTP_SendHostStatus(LIBMTP_mtpdevice_t *device, uint32_t status);
uint16_t VitaMTP_SendPartOfObjectInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init);
uint16_t VitaMTP_SendPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *object_data, uint64_t object_len);
uint16_t VitaMTP_OperateObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendStorageSize(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint64_t storage_size, uint64_t available_size);
uint16_t VitaMTP_GetTreatObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendCopyConfirmationInfoInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendCopyConfirmationInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len); // unused?
uint16_t VitaMTP_SendObjectMetadataItems(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t *ofhi);
uint16_t VitaMTP_KeepAlive(LIBMTP_mtpdevice_t *device, uint32_t event_id);

/**
 * Functions to parse XML
 */
char *add_size_header(char *orig, uint32_t len);
int vita_info_from_xml(vita_info_t *p_vita_info, char *raw_data, int len);
int initiator_info_to_xml(initiator_info_t *p_initiator_info, char **data, int *len);
int settings_info_from_xml(settings_info_t *p_settings_info, char *raw_data, int len);
int metadata_to_xml(metadata_t *p_metadata, char **data, int *len);

/**
 * Functions useful for CMAs 
 */
initiator_info_t *new_initiator_info();
void free_initiator_info(initiator_info_t *init_info);

#endif
