//
//  test.c
//  VitaMTP
//
//  Created by Yifan Lu on 2/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vitamtp.h>

#include "test_data.h"

void *vita_event_listener(LIBMTP_mtpdevice_t *device);

int event_listen;

void *vita_event_listener(LIBMTP_mtpdevice_t *device)
{
    LIBMTP_event_t event;
    int event_id;
    settings_info_t info;
    browse_info_t binfo;
    object_status_t status;
    char *temp;
    unsigned int ofhi;
    while(event_listen)
    {
        if(LIBMTP_Read_Event(device, &event) < 0)
        {
            fprintf(stderr, "Error recieving event.\n");
        }
        event_id = event.Param1;
        fprintf(stderr, "Event: 0x%x id %d\n", event.Code, event_id);
        switch(event.Code)
        {
            case PTP_EC_VITA_RequestGetSettingInfo:
                VitaMTP_GetSettingInfo(device, event_id, &info);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
            case PTP_EC_VITA_RequestSendHttpObjectFromURL:
                VitaMTP_GetUrl(device, event_id, &temp);
                VitaMTP_SendHttpObjectFromURL(device, event_id, data2, 0x3ca);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendStorageSize:
                VitaMTP_SendStorageSize(device, event_id, (uint64_t)100*1024*1024*1024, (uint64_t)50*1024*1024*1024); // Send fake 50GB/100GB
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendNumOfObject:
                VitaMTP_SendNumOfObject(device, event_id, 1);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadata:
                VitaMTP_GetBrowseInfo(device, event_id, &binfo);
                VitaMTP_SendObjectMetadata(device, event_id, get_test_metadata(0));
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectThumb:
                VitaMTP_SendObjectThumb(device, event_id, get_test_metadata(4), thumb, 0x3730);
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectStatus:
                VitaMTP_SendObjectStatus(device, event_id, &status);
                VitaMTP_SendObjectMetadata(device, event_id, get_test_metadata(0));
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
            case PTP_EC_VITA_RequestSendObjectMetadataItems:
                VitaMTP_SendObjectMetadataItems(device, event_id, &ofhi);
                VitaMTP_SendObjectMetadata(device, event_id, get_test_metadata(0));
                VitaMTP_ReportResult(device, event_id, PTP_RC_OK);
                break;
        }
    }
    
    return NULL;
}

int main(int argc, char** argv)
{
    LIBMTP_Set_Debug(9);
    
    LIBMTP_Init();
    
    LIBMTP_mtpdevice_t *device;
    pthread_t event_thread;
    PTPParams *params;
    
    device = LIBVitaMTP_Get_First_Vita();
    if (device == NULL)
    {
        fprintf(stderr, "Unable to open Vita\n");
        return 1;
    }
    
    params = (PTPParams*)device->params;
    
    event_listen = 1;
    if(pthread_create(&event_thread, NULL, (void*(*)(void*))vita_event_listener, device) != 0)
    {
        fprintf(stderr, "Cannot create event listener thread.\n");
        return 1;
    }
    
    vita_info_t vita_info;
    initiator_info_t *pc_info = new_initiator_info();
    VitaMTP_GetVitaInfo(device, &vita_info);
    VitaMTP_SendInitiatorInfo(device, pc_info);
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_Connected);
    free_initiator_info(pc_info);
    
    sleep(120);
    
    event_listen = 0;
    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
    
    LIBMTP_Release_Device(device);
    
    printf("OK.\n");
    
    return 0;
}