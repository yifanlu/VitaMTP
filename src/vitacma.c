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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vitamtp.h"

/**
 * Creates an initiator info structure with default values.
 * You should free with free_initiator_info() to avoid leaks.
 * 
 * @param host_name the name of the host device to display on Vita
 * @return a dynamically allocated structure containing default data.
 * @see VitaMTP_SendInitiatorInfo()
 * @see free_initiator_info()
 */
const initiator_info_t *new_initiator_info(const char *host_name){
    initiator_info_t *init_info = malloc(sizeof(initiator_info_t));
    char *version_str;
    asprintf(&version_str, "%d.%d", VITAMTP_VERSION_MAJOR, VITAMTP_VERSION_MINOR);
    init_info->platformType = strdup("PC");
    init_info->platformSubtype = strdup("Unknown");
    init_info->osVersion = strdup("0.0");
    init_info->version = version_str;
    init_info->protocolVersion = VITAMTP_PROTOCOL_VERSION;
    init_info->name = host_name == NULL ? strdup("VitaMTP Library") : strdup(host_name);
    init_info->applicationType = 5;
    return init_info;
}

/**
 * Frees a initiator_info structure created with new_initiator_info().
 * 
 * @param init_info the structure returned by new_initiator_info().
 * @see VitaMTP_SendInitiatorInfo()
 * @see new_initiator_info()
 */
void free_initiator_info(const initiator_info_t *init_info){
    free(init_info->platformType);
    free(init_info->platformSubtype);
    free(init_info->osVersion);
    free(init_info->version);
    free(init_info->name);
    // DON'T PANIC
    // As Linus said once, we're not modifying a const variable, 
    // but just freeing the memory it takes up.
    free((initiator_info_t*)init_info);
}

/**
 * Creates a RFC 3339 standard timestamp with correct timezone offset.
 * This is the format used by the Vita in object metadata.
 * 
 * @param time a Unix timestamp
 */
char* vita_make_time(time_t time){
    //	YYYY-MM-DDThh:mm:ss+hh:mm
	time_t tlocal = time; // save local time because gmtime modifies it
	struct tm* t1 = gmtime(&time);
	time_t tm1 = mktime(t1); // get GMT in time_t
	int diff = (int)(time - tm1); // make diff
	int h = abs(diff / 3600);
	int m = abs(diff % 60);
	struct tm* tmlocal = localtime(&tlocal); // get local time
	char* str = (char*)malloc(sizeof("0000-00-00T00:00:00+00:00")+1);
	sprintf(str, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d:%02d", tmlocal->tm_year+1900, tmlocal->tm_mon, tmlocal->tm_mday, tmlocal->tm_hour, tmlocal->tm_min, tmlocal->tm_sec, diff<0?"-":"+", h, m);
	return str;
}
