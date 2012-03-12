//
//  xmlparsing.c
//  VitaMTP
//
//  Created by Yifan Lu on 3/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "vitamtp.h"

int parse_vita_info(vita_info_t *p_vita_info, char *raw_data, int len){
    xmlDocPtr doc;
    xmlNodePtr node;
    if((doc = xmlReadMemory(raw_data, len, "vita_info.xml", NULL, 0)) == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Error parsing XML: %.*s\n", len, raw_data);
        }
        return 1;
    }
    if((node = xmlDocGetRootElement(doc)) == NULL || strcmp((char*)node->name, "VITAInformation") != 0){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot find element in XML: %s\n", "VITAInformation");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    // get info
    xmlChar *responderVersion = xmlGetProp(node, (const xmlChar*)"responderVersion");
    xmlChar *protocolVersion = xmlGetProp(node, (const xmlChar*)"protocolVersion");
    if(responderVersion == NULL || protocolVersion == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot get attributes from XML.\n");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    strcpy(p_vita_info->responderVersion, (char*)responderVersion);
    p_vita_info->protocolVersion = atoi((char*)protocolVersion);
    xmlFree(responderVersion);
    xmlFree(protocolVersion);
    // get thumb info
    if((node = node->children) == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot find children in XML.\n");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    for(; node != NULL; node = node->next){
        if(node->type != XML_ELEMENT_NODE)
            continue;
        char *nodeName = (char*)node->name;
        xmlChar *type = xmlGetProp(node, (const xmlChar*)"type");
        xmlChar *codecType = xmlGetProp(node, (const xmlChar*)"codecType");
        xmlChar *width = xmlGetProp(node, (const xmlChar*)"width");
        xmlChar *height = xmlGetProp(node, (const xmlChar*)"height");
        xmlChar *duration = xmlGetProp(node, (const xmlChar*)"duration");
        if(type == NULL || codecType == NULL || width == NULL || height == NULL){
            if(IS_LOGGING(WARNING_LOG)){
                fprintf(stderr, "Cannot find all attributes for item %s, skipping.\n", nodeName);
            }
            continue;
        }
        if(strcmp(nodeName, "photoThumb") == 0){
            p_vita_info->photoThumb.type = atoi((char*)type);
            p_vita_info->photoThumb.codecType = atoi((char*)codecType);
            p_vita_info->photoThumb.width = atoi((char*)width);
            p_vita_info->photoThumb.height = atoi((char*)height);
        }else if(strcmp(nodeName, "videoThumb") == 0){
            if(duration == NULL){
                if(IS_LOGGING(WARNING_LOG)){
                    fprintf(stderr, "Cannot find all attributes for item %s, skipping.\n", nodeName);
                }
                continue;
            }
            p_vita_info->videoThumb.type = atoi((char*)type);
            p_vita_info->videoThumb.codecType = atoi((char*)codecType);
            p_vita_info->videoThumb.width = atoi((char*)width);
            p_vita_info->videoThumb.height = atoi((char*)height);
            p_vita_info->videoThumb.duration = atoi((char*)duration);
        }else if(strcmp(nodeName, "musicThumb") == 0){
            p_vita_info->musicThumb.type = atoi((char*)type);
            p_vita_info->musicThumb.codecType = atoi((char*)codecType);
            p_vita_info->musicThumb.width = atoi((char*)width);
            p_vita_info->musicThumb.height = atoi((char*)height);
        }else if(strcmp(nodeName, "gameThumb") == 0){
            p_vita_info->gameThumb.type = atoi((char*)type);
            p_vita_info->gameThumb.codecType = atoi((char*)codecType);
            p_vita_info->gameThumb.width = atoi((char*)width);
            p_vita_info->gameThumb.height = atoi((char*)height);
        }
        xmlFree(type);
        xmlFree(codecType);
        xmlFree(width);
        xmlFree(height);
        xmlFree(duration);
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    
    return 0;
}