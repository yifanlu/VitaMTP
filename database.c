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
    free(meta->name);
    free(meta->path);
    if (meta->dataType & (SaveData | Folder)) {
        free(meta->data.saveData.detail);
        free(meta->data.saveData.dirName);
        free(meta->data.saveData.savedataTitle);
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
    struct cma_object *last = current;
    char fullpath[PATH_MAX];
    DIR *dirp;
    struct dirent *entry;
    struct stat statbuf;
    size_t fpath_pos;
    
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
        
        strcat (fullpath, entry->d_name);
        
        if(stat (fullpath, &statbuf) != 0) {
            continue;
        }
        
        current = addToDatabase (last, entry->d_name, statbuf.st_size, S_ISDIR (statbuf.st_mode) ? Folder : File);
        
        if (current->metadata.dataType & Folder) {
            addEntriesForDirectory (current, current->metadata.ohfi);
        }
        
        totalSize += current->metadata.size;
        
        fullpath[fpath_pos] = '\0';
    }
    
    last->metadata.size += totalSize;
    closedir(dirp);
}

struct cma_object *addToDatabase (struct cma_object *root, const char *name, size_t size, const enum DataType type) {
    struct cma_object *current = malloc(sizeof(struct cma_object));
    memset (current, 0, sizeof (struct cma_object));
    current->metadata.name = strdup (name);
    current->metadata.ohfiParent = root->metadata.ohfi;
    current->metadata.ohfi = ohfi_count++;
    current->metadata.type = 1; // TODO: what is type?
    current->metadata.dateTimeCreated = 0; // TODO: allow for time created
    current->metadata.size = size;
    current->metadata.dataType = type | (root->metadata.dataType & ~Folder); // get parent attributes except Folder
    switch (root->metadata.ohfi) { // add attributes based on absolute root ohfi (if possible)
        case VITA_OHFI_PSPSAVE: // TODO: Parse PSP save data
            current->metadata.dataType |= SaveData;
            current->metadata.data.saveData.title = strdup (name);
            current->metadata.data.saveData.detail = strdup("Under construction.");
            current->metadata.data.saveData.dirName = strdup(name);
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
        case VITA_OHFI_VITAAPP:
        case VITA_OHFI_PSPAPP:
            current->metadata.dataType |= App;
            break;
    }
    asprintf (&current->path, "%s/%s", root->path, name);
    if (root->metadata.path == NULL) {
        current->metadata.path = strdup (name);
    } else {
        asprintf (&current->metadata.path, "%s/%s", root->metadata.path, name);
    }
    while (root->next_object != NULL) {
        root = root->next_object;
    }
    root->next_object = current;
    return current;
}

void removeFromDatabase (int ohfi, struct cma_object *start) {
    struct cma_object **p_object;
    struct cma_object **p_toFree;
    struct cma_object *object;
    for(p_object = &start; *p_object != NULL; p_object = &(*p_object)->next_object) {
        object = *p_object;
        if (object->metadata.ohfiParent == ohfi) {
            // we know that the children must be after the parent in the linked list
            // for this to be a valid database
            removeFromDatabase (object->metadata.ohfi, object);
        }
        if (object->metadata.ohfi == ohfi) {
            p_toFree = p_object;
        }
    }
    // do this at the end so we can still see each node
    object = *p_toFree;
    *p_toFree = object->next_object;
    freeCMAObject (object);
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
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        if (db_objects[i].metadata.ohfi != ohfiRoot) {
            continue;
        }
        // first element in loop is the master object, the ones after are it's children
        for(object = db_objects[i].next_object; object != NULL; object = object->next_object) {
            if(strcmp (object->metadata.path, title) == 0) {
                return object;
            }
        }
    }
    return NULL; // not found
}

int filterObjects (int ohfiParent, metadata_t **p_head) {
    int numObjects = 0;
    metadata_t temp = {0};
    struct cma_object *db_objects = (struct cma_object*)database;
    struct cma_object *object;
    metadata_t *tail = &temp;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;
    for(i = 0; i < count; i++) {
        for(object = &db_objects[i]; object != NULL; object = object->next_object) {
            if (object->metadata.ohfiParent == ohfiParent) {
                tail->next_metadata = &object->metadata;
                tail = tail->next_metadata;
                numObjects++;
            }
        }
    }
    tail->next_metadata = NULL;
    if (p_head != NULL) {
        *p_head = temp.next_metadata;
    }
    return numObjects;
}
