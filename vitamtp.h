//
//  vitamtp.h
//  VitaMTP
//
//  Created by Yifan Lu on 2/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef VitaMTP_h
#define VitaMTP_h

#include <libmtp.h>
#include <ptp.h>

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

struct initiator_info {
    // TEMP
    int raw_len;
    unsigned char *raw_xml;
    // END TEMP
    char *platformType;
    char *platformSubtype;
    char *osVersion;
    char *version;
    int protocolVersion;
    char *name;
    uint32_t applicationType;
};

struct settings_info {
    // TEMP
    unsigned char *raw_xml;
    // END TEMP
    struct account {
        char *userName;
        char *signInId;
        char *accountId;
        char *countryCode;
        char *langCode;
        char *birthday;
        char *onlineUser;
        char *password;
        struct account *next_account;
    } first_account;
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

enum DataType {
    Folder,
    File,
    SaveData,
    Thumbnail
};

struct metadata {
    // TEMP
    int raw_len;
    unsigned char *raw_xml;
    // END TEMP
    
    char *path; // must free before freeing this struct
    
    int ohfiParent;
    int ohfi;
    char* title;
    int index;
    char* dateTimeCreated;
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
            char* dateTimeUpdated;
            int statusType;
        } saveData;
        
        struct thumbnail {
            // TEMP
            int raw_len;
            unsigned char *raw_xml;
            // END TEMP
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

struct object_status {
    uint32_t type;
    uint32_t size;
    char *file;
    void *p_data; // for freeing
};

struct send_part_init {
    int ohfi;
    uint64_t offset;
    uint64_t size;
};

struct http_object_prop {
    uint64_t size;
    uint8_t timestamp_len;
    char* timestamp;
};

typedef struct vita_info vita_info_t;
typedef struct initiator_info initiator_info_t;
typedef struct settings_info settings_info_t;
typedef struct browse_info browse_info_t;
typedef struct metadata metadata_t;
typedef struct thumbnail thumbnail_t;
typedef struct object_status object_status_t;
typedef struct send_part_init send_part_init_t;
typedef struct http_object_prop http_object_prop_t;

#define VITA_PID 0x04E4
#define VITA_VID 0x054C

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

#define PTP_OFC_Unknown1 0xB005
#define PTP_OFC_Unknown2 0xB006
#define PTP_OFC_PSPGame 0xB007
#define PTP_OFC_PSPSave 0xB00A
#define PTP_OFC_Unknown3 0xB00B
#define PTP_OFC_Unknown4 0xB00F
#define PTP_OFC_Unknown5 0xB010
#define PTP_OFC_VitaGame 0xB014

#define VITA_HOST_STATUS_Connected 0x0
#define VITA_HOST_STATUS_Unknown1 0x1
#define VITA_HOST_STATUS_Deactivate 0x2
#define VITA_HOST_STATUS_EndConnection 0x3
#define VITA_HOST_STATUS_StartConnection 0x4
#define VITA_HOST_STATUS_Unknown2 0x5

#define VITA_BROWSE_MUSIC 0x01
#define VITA_BROWSE_PHOTO 0x02
#define VITA_BROWSE_VIDEO 0x03
#define VITA_BROWSE_APPLICATION 0x0A
#define VITA_BROWSE_PSPGAME 0x0D
#define VITA_BROWSE_PSPSAVE 0x0E

#define VITA_BROWSE_SUBNONE 0x00
#define VITA_BROWSE_SUBFILE 0x01

#define DEBUG_LOG 0x8000
#define INFO_LOG 0x800000
#define WARNING_LOG 0x1000000
#define ERROR_LOG 0x2000000
#define IS_LOGGING(log) ((log_mask & log) == log)

extern int log_mask;

LIBMTP_mtpdevice_t *LIBVitaMTP_Get_First_Vita(void);
uint16_t VitaMTP_SendData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int len);
uint16_t VitaMTP_GetData(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, unsigned char** data, unsigned int* len);
uint16_t VitaMTP_SendInt32(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t code, uint32_t value);
uint16_t VitaMTP_GetVitaInfo(LIBMTP_mtpdevice_t *device, vita_info_t *info);
uint16_t VitaMTP_SendNumOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t num);
uint16_t VitaMTP_GetBrowseInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, browse_info_t* info);
uint16_t VitaMTP_SendObjectMetadata(LIBMTP_mtpdevice_t *device, uint32_t event_id, metadata_t* metas);
uint16_t VitaMTP_SendObjectThumb(LIBMTP_mtpdevice_t *device, uint32_t event_id, thumbnail_t* thumb);
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
uint16_t VitaMTP_SendPartOfObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, send_part_init_t* init, metadata_t* meta);
uint16_t VitaMTP_OperateObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendStorageSize(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint64_t storage_size, uint64_t available_size);
uint16_t VitaMTP_GetTreatObject(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendCopyConfirmationInfoInit(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char **data, unsigned int *len); // unused?
uint16_t VitaMTP_SendCopyConfirmationInfo(LIBMTP_mtpdevice_t *device, uint32_t event_id, unsigned char *data, unsigned int len); // unused?
uint16_t VitaMTP_SendObjectMetadataItems(LIBMTP_mtpdevice_t *device, uint32_t event_id, uint32_t *ofhi);
uint16_t VitaMTP_KeepAlive(LIBMTP_mtpdevice_t *device, uint32_t event_id);

int parse_vita_info(vita_info_t *p_vita_info, char *raw_data, int len);

#endif
