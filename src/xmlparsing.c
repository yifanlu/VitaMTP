//
//  Converts XML data to structs and vice versa
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
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <time.h>
#include "vitamtp.h"

/**
 * Takes raw data and inserts the size of it as the first 4 bytes.
 * This format is used many times by the Vita's MTP commands.
 * 
 * @param orig an array containing the data.
 * @param len the length of the data.
 * @return a new array containing the data plus the header.
 *  The new array is dynamically allocated and must be freed when done.
 */
char *add_size_header(char *orig, uint32_t len){
    char *new_data;
    int tot_len = len + sizeof(uint32_t); // room for header
    new_data = malloc(tot_len);
    memcpy(new_data, &len, sizeof(uint32_t)); // copy header
    memcpy(new_data + sizeof(uint32_t), orig, len);
    return new_data;
}

/**
 * Takes XML data from GetVitaInfo and turns it into a structure.
 * This should be called automatically.
 * 
 * @param p_vita_info a pointer to the structure to fill.
 * @param raw_data the XML data.
 * @param len the length of the XML data.
 * @return zero on success
 * @see VitaMTP_GetVitaInfo()
 */
int vita_info_from_xml(vita_info_t *p_vita_info, const char *raw_data, const int len){
    xmlDocPtr doc;
    xmlNodePtr node;
    if((doc = xmlReadMemory(raw_data, len, "vita_info.xml", NULL, 0)) == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Error parsing XML: %.*s\n", len, raw_data);
        }
        return 1;
    }
    if((node = xmlDocGetRootElement(doc)) == NULL || xmlStrcmp(node->name, BAD_CAST "VITAInformation") != 0){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot find element in XML: %s\n", "VITAInformation");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    // get info
    xmlChar *responderVersion = xmlGetProp(node, BAD_CAST "responderVersion");
    xmlChar *protocolVersion = xmlGetProp(node, BAD_CAST "protocolVersion");
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
        xmlChar *type = xmlGetProp(node, BAD_CAST "type");
        xmlChar *codecType = xmlGetProp(node, BAD_CAST "codecType");
        xmlChar *width = xmlGetProp(node, BAD_CAST "width");
        xmlChar *height = xmlGetProp(node, BAD_CAST "height");
        xmlChar *duration = xmlGetProp(node, BAD_CAST "duration");
        if(type == NULL || codecType == NULL || width == NULL || height == NULL){
            if(IS_LOGGING(WARNING_LOG)){
                fprintf(stderr, "Cannot find all attributes for item %s, skipping.\n", node->name);
            }
            continue;
        }
        if(xmlStrcmp(node->name, BAD_CAST "photoThumb") == 0){
            p_vita_info->photoThumb.type = atoi((char*)type);
            p_vita_info->photoThumb.codecType = atoi((char*)codecType);
            p_vita_info->photoThumb.width = atoi((char*)width);
            p_vita_info->photoThumb.height = atoi((char*)height);
        }else if(xmlStrcmp(node->name, BAD_CAST "videoThumb") == 0){
            if(duration == NULL){
                if(IS_LOGGING(WARNING_LOG)){
                    fprintf(stderr, "Cannot find all attributes for item %s, skipping.\n", node->name);
                }
                continue;
            }
            p_vita_info->videoThumb.type = atoi((char*)type);
            p_vita_info->videoThumb.codecType = atoi((char*)codecType);
            p_vita_info->videoThumb.width = atoi((char*)width);
            p_vita_info->videoThumb.height = atoi((char*)height);
            p_vita_info->videoThumb.duration = atoi((char*)duration);
        }else if(xmlStrcmp(node->name, BAD_CAST "musicThumb") == 0){
            p_vita_info->musicThumb.type = atoi((char*)type);
            p_vita_info->musicThumb.codecType = atoi((char*)codecType);
            p_vita_info->musicThumb.width = atoi((char*)width);
            p_vita_info->musicThumb.height = atoi((char*)height);
        }else if(xmlStrcmp(node->name, BAD_CAST "gameThumb") == 0){
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

/**
 * Takes initiator information and generate XML from it.
 * This should be called automatically.
 * 
 * @param p_vita_info a pointer to the structure as input.
 * @param data a pointer to the array to fill.
 * @param len a pointer to the length of the array.
 * @return zero on success.
 * @see VitaMTP_SendInitiatorInfo()
 */
int initiator_info_to_xml(const initiator_info_t *p_initiator_info, char** data, int *len){
    static const char *format = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<initiatorInfo platformType=\"%s\" platformSubtype=\"%s\" osVersion=\"%s\" version=\"%s\" protocolVersion=\"%08d\" name=\"%s\" applicationType=\"%d\" />\n";

    int ret = asprintf(data, format, p_initiator_info->platformType, p_initiator_info->platformSubtype, p_initiator_info->osVersion, p_initiator_info->version, p_initiator_info->protocolVersion, p_initiator_info->name, p_initiator_info->applicationType);
    if(ret > 0){
        // create the length header
        char *new_data = add_size_header(*data, (int)strlen(*data) + 1);
        *len = (int)strlen(*data) + 1 + sizeof(uint32_t);
        free(*data); // free old string
        *data = new_data;
    }
    return ret;
}

/**
 * Takes settings information from XML and creates a structure.
 * This should be called automatically.
 * 
 * @param p_settings_info a pointer to the structure to fill.
 * @param raw_data the XML input.
 * @param len the size of the XML input.
 * @return zero on success.
 * @see VitaMTP_GetSettingInfo()
 */
int settings_info_from_xml(settings_info_t *p_settings_info, const char *raw_data, const int len){
    xmlDocPtr doc;
    xmlNodePtr node;
    xmlNodePtr innerNode;
    if((doc = xmlReadMemory(raw_data, len, "setting_info.xml", NULL, 0)) == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Error parsing XML: %.*s\n", len, raw_data);
        }
        return 1;
    }
    if((node = xmlDocGetRootElement(doc)) == NULL || xmlStrcmp(node->name, BAD_CAST "settingInfo") != 0){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot find element in XML: %s\n", "settingInfo");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    if((node = node->children) == NULL){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "Cannot find children in XML.\n");
        }
        xmlFreeDoc(doc);
        return 1;
    }
    for(; node != NULL; node = node->next){
        if(xmlStrcmp(node->name, BAD_CAST "accounts") == 0){
            struct account *current_account = &p_settings_info->current_account;
            for(innerNode = node->children; innerNode != NULL; innerNode = innerNode->next){
                if(xmlStrcmp(innerNode->name, BAD_CAST "npAccount") != 0)
                    continue;
                current_account->userName = (char*)xmlGetProp(innerNode, BAD_CAST "userName");
                current_account->signInId = (char*)xmlGetProp(innerNode, BAD_CAST "signInId");
                current_account->accountId = (char*)xmlGetProp(innerNode, BAD_CAST "accountId");
                current_account->countryCode = (char*)xmlGetProp(innerNode, BAD_CAST "countryCode");
                current_account->langCode = (char*)xmlGetProp(innerNode, BAD_CAST "langCode");
                current_account->birthday = (char*)xmlGetProp(innerNode, BAD_CAST "birthday");
                current_account->onlineUser = (char*)xmlGetProp(innerNode, BAD_CAST "onlineUser");
                current_account->passwd = (char*)xmlGetProp(innerNode, BAD_CAST "passwd");
                if(innerNode->next != NULL){
                    struct account *next_account = malloc(sizeof(struct account));
                    current_account->next_account = next_account;
                    current_account = next_account;
                } else {
                    current_account->next_account = NULL;
                }
            }
        }
        // here is room for future additions
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    
    return 0;
}

/**
 * Takes a metadata linked list and generates XML data.
 * This should be called automatically.
 * 
 * @param p_metadata a pointer to the structure as input.
 * @param data a pointer to the array to output.
 * @param len a pointer to the length of the output.
 * @return zero on success.
 * @see VitaMTP_SendObjectMetadata()
 */
int metadata_to_xml(const metadata_t *p_metadata, char** data, int *len){
    xmlTextWriterPtr writer;
    xmlBufferPtr buf;
    
    buf = xmlBufferCreate();
    if (buf == NULL) {
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "metadata_to_xml: Error creating the xml buffer\n");
            return 1;
        }
    }
    writer = xmlNewTextWriterMemory(buf, 0);
    if (writer == NULL) {
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "metadata_to_xml: Error creating the xml writer\n");
            return 1;
        }
    }
    if(xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL) < 0){
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "metadata_to_xml: Error at xmlTextWriterStartDocument\n");
            return 1;
        }
    }
    xmlTextWriterStartElement(writer, BAD_CAST "objectMetadata");
    
    int i = 0;
    int j = 0;
    for(const metadata_t *current = p_metadata; current != NULL; current = current->next_metadata){
        char *timestamp;
        if (MASK_SET (current->dataType, SaveData | Folder)) {
            xmlTextWriterStartElement(writer, BAD_CAST "saveData");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "detail", "%s", current->data.saveData.detail);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dirName", "%s", current->data.saveData.dirName);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "savedataTitle", "%s", current->data.saveData.savedataTitle);
            timestamp = vita_make_time(current->data.saveData.dateTimeUpdated);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dateTimeUpdated", "%s", timestamp);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "title", "%s", current->data.saveData.title);
            free(timestamp);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "statusType", "%d", current->data.saveData.statusType);
        } else if (MASK_SET (current->dataType, Photo | File)) {
            xmlTextWriterStartElement(writer, BAD_CAST "photo");
            timestamp = vita_make_time(current->data.photo.dateTimeOriginal);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "title", "%s", current->data.photo.title);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dateTimeOriginal", "%s", timestamp);
            free(timestamp);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileFormatType", "%d", current->data.photo.fileFormatType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileName", "%s", current->data.photo.fileName);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "statusType", "%d", current->data.photo.statusType);
        } else if (MASK_SET (current->dataType, Music | File)) {
            xmlTextWriterStartElement(writer, BAD_CAST "music");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "title", "%s", current->data.music.title);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "album", "%s", current->data.music.album);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "artist", "%s", current->data.music.artist);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "statusType", "%d", current->data.music.statusType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileFormatType", "%d", current->data.music.fileFormatType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileName", "%s", current->data.music.fileName);
        } else if (MASK_SET (current->dataType, Video | File)) {
            xmlTextWriterStartElement(writer, BAD_CAST "video");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileFormatType", "%d", current->data.video.fileFormatType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fileName", "%s", current->data.video.fileName);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "parentalLevel", "%d", current->data.video.parentalLevel);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "statusType", "%d", current->data.video.statusType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "explanation", "%s", current->data.video.explanation);
            timestamp = vita_make_time(current->data.saveData.dateTimeUpdated);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dateTimeUpdated", "%s", timestamp);
            free(timestamp);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "copyright", "%s", current->data.video.copyright);
        } else if (MASK_SET (current->dataType, Thumbnail)) {
            xmlTextWriterStartElement(writer, BAD_CAST "thumbnail");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "codecType", "%d", current->data.thumbnail.codecType);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "width", "%d", current->data.thumbnail.width);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "height", "%d", current->data.thumbnail.height);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "%d", current->data.thumbnail.type);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "orientationType", "%d", current->data.thumbnail.orientationType);
            char *aspectRatio;
            asprintf(&aspectRatio, "%.6f", current->data.thumbnail.aspectRatio);
            char *period = strchr(aspectRatio, '.');
            *period = ','; // All this to make period a comma, maybe there is an easier way?
            xmlTextWriterWriteAttribute(writer, BAD_CAST "aspectRatio", BAD_CAST aspectRatio);
            free(aspectRatio);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "fromType", "%d", current->data.thumbnail.fromType);
        } else if (current->dataType & Folder) {
            xmlTextWriterStartElement(writer, BAD_CAST "folder");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "%d", current->type);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "name", "%s", current->name);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "title", "%s", current->name ? current->name : "");
        } else if (current->dataType & File) {
            xmlTextWriterStartElement(writer, BAD_CAST "file");
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "name", "%s", current->name);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "statusType", "%d", current->type);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "title", "%s", current->name ? current->name : "");
        } else {
            continue; // not supported
        }
        
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "index", "%d", i++);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ohfiParent", "%d", current->ohfiParent);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ohfi", "%d", current->ohfi);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "size", "%lu", current->size);
        timestamp = vita_make_time(current->dateTimeCreated);
        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "dateTimeCreated", "%s", timestamp);
        free(timestamp);
        
        if (current->dataType & (Photo | Music | Video) && MASK_SET (current->dataType, File)) {
            for (j = 0; j < current->data.photo.numTracks; j++) { // union layed out so any one of the three can be used
                xmlTextWriterStartElement(writer, BAD_CAST "track");
                xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "index", "%d", j+1);
                xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "%d", current->data.video.tracks[j].type);
                switch (current->data.photo.tracks[j].type) {
                    case VITA_TRACK_TYPE_AUDIO:
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "bitrate", "%d", current->data.video.tracks[j].data.track_audio.bitrate);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "codecType", "%d", current->data.video.tracks[j].data.track_audio.codecType);
                        break;
                    case VITA_TRACK_TYPE_VIDEO:
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "width", "%d", current->data.video.tracks[j].data.track_video.width);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "height", "%d", current->data.video.tracks[j].data.track_video.height);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "bitrate", "%d", current->data.video.tracks[j].data.track_video.bitrate);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "codecType", "%d", current->data.video.tracks[j].data.track_video.codecType);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "duration", "%ld", current->data.video.tracks[j].data.track_video.duration);
                        break;
                    case VITA_TRACK_TYPE_PHOTO:
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "width", "%d", current->data.video.tracks[j].data.track_photo.width);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "height", "%d", current->data.video.tracks[j].data.track_photo.height);
                        xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "codecType", "%d", current->data.video.tracks[j].data.track_photo.codecType);
                        break;
                }
                xmlTextWriterEndElement(writer);
            }
        }
        xmlTextWriterEndElement(writer);
    }
    
    xmlTextWriterEndElement(writer);
    if (xmlTextWriterEndDocument(writer) < 0) {
        if(IS_LOGGING(ERROR_LOG)){
            fprintf(stderr, "metadata_to_xml: Error at xmlTextWriterEndDocument\n");
            return 1;
        }
    }
    xmlFreeTextWriter(writer);
    *data = add_size_header((char*)buf->content, (uint32_t)buf->use + 1);
    *len = buf->use + sizeof(uint32_t) + 1;
    xmlBufferFree(buf);
    return 0;
}
