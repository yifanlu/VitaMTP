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

void createDatabase() {
    initDatabase();
    char *path;
    DIR *dirp;
    struct dirent *entry;
    struct stat statbuf;
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;
    int parent_ohfi;
    struct cma_object *current;
    metadata_t *current_meta;
    struct cma_object *last;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        path = db_objects[i].path;
        dirp = opendir(path);
        if(dirp == NULL) {
            continue;
        }
        parent_ohfi = db_objects[i].metadata.ohfi;
        last = &db_objects[i];
        i = 0;
        while ((entry = readdir(dirp)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue; // ignore hidden folders and ., ..
            }
            current = malloc(sizeof(struct cma_object));
            current_meta = &current->metadata;
            char *fullpath;
            asprintf(&fullpath, "%s/%s", path, entry->d_name);
            if(stat(fullpath, &statbuf) != 0) {
                free(fullpath);
                free(current);
                continue;
            }
            
            current->path = fullpath;
            current_meta->ohfiParent = parent_ohfi;
            current_meta->ohfi = ohfi_count;
            current_meta->title = strdup(entry->d_name);
            current_meta->index = i;
            current_meta->dateTimeCreated = (long)statbuf.st_mtime;
            current_meta->size = statbuf.st_blocks * statbuf.st_blksize;
            
            switch(parent_ohfi) {
                case VITA_OHFI_VITAAPP:
                    current_meta->dataType = Folder;
                    current_meta->data.folder.name = strdup(entry->d_name);
                    current_meta->data.folder.type = 1; // TODO: Always 1?
                    break;
                // TODO: other OHFI parsing
            }
            
            // we need to link both linked lists, one for the object and one 
            // for the metadata stored in the object. metadata is used by libVitaMTP
            // essentially, they both point to the same location in memory, but we 
            // want it to look cleaner than constantly casting one to another
            last->next_object = current;
            last->metadata.next_metadata = current_meta;
            last = current;
            
            ohfi_count++;
            i++;
        }
        
        closedir(dirp);
    }
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

int countDatabase(struct cma_object *root) {
    int count = 0;
    struct cma_object *object;
    for(object = root->next_object; object != NULL; object = object->next_object) {
        count++;
    }
    return count;
}
