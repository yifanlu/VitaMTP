//
//  Simple database for reference implementation
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

#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitamtp.h>

#include "opencma.h"

extern struct cma_database *database;
extern int ohfi_count;

static void freeCMAObject(struct cma_object *obj) {
    if(obj == NULL)
        return;
    metadata_t *meta = &obj->metadata;
    free(meta->title);
    switch(meta->dataType){
        case Game:
        case Folder:
            free(meta->data.folder.name);
            break;
        case File:
            free(meta->data.file.name);
            break;
        case SaveData:
            free(meta->data.saveData.detail);
            free(meta->data.saveData.dirName);
            free(meta->data.saveData.savedataTitle);
            break;
        default:
            break;
    }
    free(obj->path);
    free(obj);
}

static inline void destroyDatabaseList(struct cma_object *current) {
    if(current == NULL)
        return;
    destroyDatabaseList(current->next_object);
    freeCMAObject(current);
}

static inline void initDatabase() {
    database->photos.metadata.ohfi = VITA_OHFI_PHOTO;
    database->videos.metadata.ohfi = VITA_OHFI_VIDEO;
    database->music.metadata.ohfi = VITA_OHFI_MUSIC;
    database->vitaApps.metadata.ohfi = VITA_OHFI_VITAAPP;
    database->pspApps.metadata.ohfi = VITA_OHFI_PSPAPP;
    database->pspSaves.metadata.ohfi = VITA_OHFI_PSPSAVE;
    database->psxApps.metadata.ohfi = VITA_OHFI_PSXAPP;
    database->psmApps.metadata.ohfi = VITA_OHFI_PSMAPP;
    database->backups.metadata.ohfi = VITA_OHFI_BACKUP;
}

void refreshDatabase() {
    destroyDatabase();
    createDatabase();
}

void destroyDatabase() {
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        destroyDatabaseList(db_objects[i].next_object);
        db_objects[i].metadata.next_metadata = NULL;
    }
}

// the database is structured as an array of linked list, each list representing a category (saves, vita games, etc)
// each linked list contains the objects, files, directories, etc
void createDatabase() {
    initDatabase();
    int i;
    struct cma_object *current;
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        current = &db_objects[i];
        addEntriesForDirectory (current, current->metadata.ohfi);
    }
}

void addEntriesForDirectory (struct cma_object *current, int parent_ohfi) {
    struct cma_object *caller = current;
    struct cma_object *last = current;
    char path[PATH_MAX];
    char fullpath[PATH_MAX];
    DIR *dirp;
    struct dirent *entry;
    struct stat statbuf;
    size_t path_pos;
    size_t fpath_pos;
    
    path[0] = '\0';
    if (last->metadata.title != NULL) {
        sprintf (path, "%s/", last->metadata.title);
    }
    path_pos = strlen (path);
    fullpath[0] = '\0';
    sprintf (fullpath, "%s/", last->path);
    fpath_pos = strlen (fullpath);
    
    if ((dirp = opendir (fullpath)) == NULL) {
        return;
    }
    
    unsigned long totalSize = 0;
    while ((entry = readdir (dirp)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; // ignore hidden folders and ., ..
        }
        
        strcat (path, entry->d_name);
        strcat (fullpath, entry->d_name);
        
        if(stat (fullpath, &statbuf) != 0) {
            continue;
        }
        
        current = malloc(sizeof(struct cma_object));
        memset (current, 0, sizeof (struct cma_object));
        
        current->path = strdup (fullpath);
        current->metadata.ohfiParent = parent_ohfi;
        current->metadata.ohfi = ohfi_count++;
        /*
        int pos;
        if ((pos = strchr (entry->d_name, '/')) > -1) {
            current->metadata.title[pos] = '\0';
        }
         */
        current->metadata.dateTimeCreated = (long)statbuf.st_mtime;
        current->metadata.size = statbuf.st_size;
        
        switch (parent_ohfi) {
            case VITA_OHFI_PSPSAVE: // TODO: Parse PSP save data
                current->metadata.title = strdup (entry->d_name);
                current->metadata.dataType = SaveData;
                current->metadata.data.saveData.detail = strdup("Under construction.");
                current->metadata.data.saveData.dirName = strdup(entry->d_name);
                current->metadata.data.saveData.savedataTitle = strdup("Under construction.");
                current->metadata.data.saveData.dateTimeUpdated = 0;
                current->metadata.data.saveData.statusType = 1;
                break;
                // TODO: other OHFI parsing
            case VITA_OHFI_MUSIC:
            case VITA_OHFI_PHOTO:
            case VITA_OHFI_VIDEO:
            case VITA_OHFI_BACKUP:
                break;
            default: // any other id is just a file/folder
            case VITA_OHFI_VITAAPP: // apps are non-hidden folders
            case VITA_OHFI_PSPAPP:
                // folder and file share same union structure
                current->metadata.title = strdup (entry->d_name);
                current->metadata.dataType = S_ISDIR (statbuf.st_mode) ? parent_ohfi < OHFI_OFFSET ? Game : Folder : File;
                current->metadata.data.folder.name = strdup (entry->d_name);
                current->metadata.data.folder.type = 1; // TODO: always 1?
                if (current->metadata.dataType != File) {
                    addEntriesForDirectory (current, current->metadata.ohfi);
                }
                break;
        }
        
        totalSize += current->metadata.size;
        while (last->next_object != NULL) {
            last = last->next_object; // go to end of list
        }
        last->next_object = current;
        last->metadata.next_metadata = &current->metadata;
        last = current;
        path[path_pos] = '\0';
        fullpath[fpath_pos] = '\0';
    }
    
    caller->metadata.size += totalSize;
    closedir(dirp);
}

struct cma_object *ohfiToObject(int ohfi) {
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    metadata_t *meta;
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        // first element in loop is the master object, the ones after are it's children
        for(object = &db_objects[i]; object != NULL; object = object->next_object) {
            meta = &object->metadata;
            if(meta->ohfi == ohfi){
                return object;
            }
        }
    }
    return NULL; // not found
}

struct cma_object *titleToObject(char *title, int ohfiRoot) {
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    size_t src_len;
    size_t dest_len = strlen (title);
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        if (db_objects[i].metadata.ohfi != ohfiRoot) {
            continue;
        }
        // first element in loop is the master object, the ones after are it's children
        for(object = db_objects[i].next_object; object != NULL; object = object->next_object) {
            src_len = strlen (object->path);
            if(strncmp (object->path + src_len - dest_len, title, dest_len) == 0) {
                return object;
            }
        }
    }
    return NULL; // not found
}

int countDatabase(struct cma_object *root) {
    int count = 0;
    struct cma_object *object;
    for(object = root->next_object; object != NULL; object = object->next_object) {
        count++;
    }
    return count;
}
