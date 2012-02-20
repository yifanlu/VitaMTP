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
    unsigned char *raw_xml;
    char *responderVersion;
    int protocolVersion;
    struct photoThumb {
        int type;
        int codecType;
        int width;
        int height;
    };
    struct videoThumb {
        int type;
        int codecType;
        int width;
        int height;
        int duration;
    };
    struct musicThumb {
        int type;
        int codecType;
        int width;
        int height;
    };
    struct gameThumb {
        int type;
        int codecType;
        int width;
        int height;
    };
};

struct initiator_info {
    int raw_len;
    unsigned char *raw_xml;
    char *platformType;
    char *platformSubtype;
    char *osVersion;
    char *version;
    int protocolVersion;
    char *name;
    int applicationType;
};

struct settings_info {
    unsigned char *raw_xml;
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

typedef struct vita_info vita_info_t;
typedef struct initiator_info initiator_info_t;
typedef struct settings_info settings_info_t;

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

LIBMTP_mtpdevice_t *LIBVitaMTP_Get_First_Vita(void);
uint16_t VitaMTP_GetVitaInfo(LIBMTP_mtpdevice_t *device, vita_info_t *info);
uint16_t VitaMTP_SendInitiatorInfo(LIBMTP_mtpdevice_t *device, initiator_info_t *info);
uint16_t VitaMTP_SendHostStatus(LIBMTP_mtpdevice_t *device, int status);
uint16_t VitaMTP_GetSettingInfo(LIBMTP_mtpdevice_t *device, int event_id, settings_info_t *info);

#endif
