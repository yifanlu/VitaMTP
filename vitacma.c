//
//  Functions useful for CMAs
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

#include <stdlib.h>
#include <string.h>
#include "vitamtp.h"

/**
 * Creates an initiator info structure with default values.
 * You should free with free_initiator_info() to avoid leaks.
 * 
 * @return a dynamically allocated structure containing default data.
 * @see VitaMTP_SendInitiatorInfo()
 * @see free_initiator_info()
 */
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

/**
 * Frees a initiator_info structure created with new_initiator_info().
 * 
 * @see VitaMTP_SendInitiatorInfo()
 * @see new_initiator_info()
 */
void free_initiator_info(initiator_info_t *init_info){
    free(init_info->platformType);
    free(init_info->platformSubtype);
    free(init_info->osVersion);
    free(init_info->version);
    free(init_info->name);
    free(init_info);
}
