//
//  vitacma.c
//  VitaMTP
//
//  Created by Yifan Lu on 3/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "vitamtp.h"

initiator_info_t *new_initiator_info(){
    initiator_info_t *init_info = malloc(sizeof(initiator_info_t));
    char *version_str;
    asprintf(&version_str, "%d.%d", VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR);
    init_info->platformType = strdup("PC");
    init_info->platformSubtype = strdup("Unknown");
    init_info->osVersion = strdup("0.0");
    init_info->version = version_str;
    init_info->protocolVersion = VITAMTP_PROTOCOL_VERSION;
    init_info->name = strdup("VitaMTP Library");
    init_info->applicationType = 5;
    return init_info;
}

void free_initiator_info(initiator_info_t *init_info){
    free(init_info->platformType);
    free(init_info->platformSubtype);
    free(init_info->osVersion);
    free(init_info->version);
    free(init_info->name);
    free(init_info);
}
