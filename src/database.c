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

#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitamtp.h>

#include "opencma.h"

struct cma_database *g_database;
int g_ohfi_count;
pthread_mutexattr_t g_database_lock_attr;
pthread_mutex_t g_database_lock;

static inline void initDatabase(struct cma_paths *paths, const char *uuid) {
    pthread_mutex_lock (&g_database_lock);
    g_database->photos.metadata.ohfi = VITA_OHFI_PHOTO;
    g_database->photos.path = strdup (paths->photosPath);
    g_database->videos.metadata.ohfi = VITA_OHFI_VIDEO;
    g_database->videos.path = strdup (paths->videosPath);
    g_database->music.metadata.ohfi = VITA_OHFI_MUSIC;
    g_database->music.path = strdup (paths->musicPath);
    g_database->vitaApps.metadata.ohfi = VITA_OHFI_VITAAPP;
    asprintf(&g_database->vitaApps.path, "%s/%s/%s", paths->appsPath, "APP", uuid);
    g_database->pspApps.metadata.ohfi = VITA_OHFI_PSPAPP;
    asprintf(&g_database->pspApps.path, "%s/%s/%s", paths->appsPath, "PGAME", uuid);
    g_database->pspSaves.metadata.ohfi = VITA_OHFI_PSPSAVE;
    asprintf(&g_database->pspSaves.path, "%s/%s/%s", paths->appsPath, "PSAVEDATA", uuid);
    g_database->psxApps.metadata.ohfi = VITA_OHFI_PSXAPP;
    asprintf(&g_database->psxApps.path, "%s/%s/%s", paths->appsPath, "PSGAME", uuid);
    g_database->psmApps.metadata.ohfi = VITA_OHFI_PSMAPP;
    asprintf(&g_database->psmApps.path, "%s/%s/%s", paths->appsPath, "PSM", uuid);
    g_database->backups.metadata.ohfi = VITA_OHFI_BACKUP;
    asprintf(&g_database->backups.path, "%s/%s/%s", paths->appsPath, "SYSTEM", uuid);
    pthread_mutex_unlock (&g_database_lock);
}

// the database is structured as an array of linked list, each list representing a category (saves, vita games, etc)
// each linked list contains the objects, files, directories, etc
void createDatabase(struct cma_paths *paths, const char *uuid) {
    pthread_mutexattr_init (&g_database_lock_attr);
    pthread_mutexattr_settype (&g_database_lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init (&g_database_lock, &g_database_lock_attr);
    
    pthread_mutex_lock (&g_database_lock);
    g_database = malloc(sizeof(struct cma_database));
    memset(g_database, 0, sizeof(struct cma_database));
    initDatabase(paths, uuid);
    g_ohfi_count = OHFI_OFFSET;
    int i;
    struct cma_object *current;
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        current = &db_objects[i];
        addEntriesForDirectory (current, current->metadata.ohfi);
    }
    pthread_mutex_unlock (&g_database_lock);
}

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
    if (obj->metadata.ohfi >= OHFI_OFFSET) {
        free(obj);
    }
}

void destroyDatabase() {
    if (g_database == NULL) {
        return; // can't destroy what hasn't been created
    }
    pthread_mutex_lock (&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        struct cma_object *next = NULL;
        struct cma_object *current;
        for (current = &db_objects[i]; current != NULL; current = next) {
            next = current->next_object;
            freeCMAObject (current);
        }
    }
    pthread_mutex_unlock (&g_database_lock);
    
    pthread_mutex_destroy (&g_database_lock);
    pthread_mutexattr_destroy (&g_database_lock_attr);
    free (g_database);
    g_database = NULL;
}

inline void lockDatabase () {
    pthread_mutex_lock (&g_database_lock);
}

inline void unlockDatabase () {
    pthread_mutex_unlock (&g_database_lock);
}

void addEntriesForDirectory (struct cma_object *current, int parent_ohfi) {
    pthread_mutex_lock (&g_database_lock);
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
        pthread_mutex_unlock (&g_database_lock);
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
    pthread_mutex_unlock (&g_database_lock);
}

struct cma_object *addToDatabase (struct cma_object *root, const char *name, size_t size, const enum DataType type) {
    pthread_mutex_lock (&g_database_lock);
    struct cma_object *current = malloc(sizeof(struct cma_object));
    memset (current, 0, sizeof (struct cma_object));
    current->metadata.name = strdup (name);
    current->metadata.ohfiParent = root->metadata.ohfi;
    current->metadata.ohfi = g_ohfi_count++;
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
    pthread_mutex_unlock (&g_database_lock);
    return current;
}

void removeFromDatabase (int ohfi, struct cma_object *start) {
    pthread_mutex_lock (&g_database_lock);
    struct cma_object **p_object;
    struct cma_object **p_toFree;
    struct cma_object *prev = NULL;
    for(p_object = &start; *p_object != NULL; p_object = &(*p_object)->next_object) {
        if ((*p_object)->metadata.ohfi == ohfi) {
            p_toFree = p_object;
        }
        if ((*p_object)->metadata.ohfiParent == ohfi) {
            // we know that the children must be after the parent in the linked list
            // for this to be a valid database
            assert (prev != NULL);
            removeFromDatabase ((*p_object)->metadata.ohfi, prev);
            p_object = &prev; // back up one since *p_object is freed
        }
        prev = *p_object; // store "prevous" object
    }
    // do this at the end so we can still see each node
    prev = *p_toFree;
    *p_toFree = prev->next_object;
    pthread_mutex_unlock (&g_database_lock);
    freeCMAObject (prev);
}

void renameRootEntry (struct cma_object *object, const char *name, const char *newname) {
    pthread_mutex_lock (&g_database_lock);
    struct cma_object *temp;
    char *origPath = object->path;
    char *origRelPath = object->metadata.path;
    char *origName = object->metadata.name;
    if (name == NULL) {
        name = origName;
    }
    object->metadata.name = strreplace (origName, name, newname);
    object->metadata.path = strreplace (origRelPath, name, newname);
    object->path = strreplace (origPath, origRelPath, object->metadata.path);
    for (temp = object; temp != NULL; temp = temp->next_object) {
        // rename all child objects
        if (temp->metadata.ohfiParent == object->metadata.ohfi) {
            char *nname;
            char *nnewname;
            asprintf (&nname, "%s/%s", origRelPath, temp->metadata.name);
            asprintf (&nnewname, "%s/%s", object->metadata.path, temp->metadata.name);
            renameRootEntry (temp, nname, nnewname);
            free (nname);
            free (nnewname);
        }
    }
    free (origPath);
    free (origRelPath);
    free (origName);
    pthread_mutex_unlock (&g_database_lock);
}

struct cma_object *ohfiToObject(int ohfi) {
    pthread_mutex_lock (&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    struct cma_object *found = NULL;
    metadata_t *meta;
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        // first element in loop is the master object, the ones after are it's children
        for(object = &db_objects[i]; object != NULL; object = object->next_object) {
            meta = &object->metadata;
            if(meta->ohfi == ohfi){
                found = object;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    pthread_mutex_unlock (&g_database_lock);
    return found;
}

struct cma_object *titleToObject(char *title, int ohfiRoot) {
    pthread_mutex_lock (&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object*)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    struct cma_object *found = NULL;
    int i;
    // loop through all the master objects
    for(i = 0; i < count; i++) {
        if (db_objects[i].metadata.ohfi != ohfiRoot) {
            continue;
        }
        // first element in loop is the master object, the ones after are it's children
        for(object = db_objects[i].next_object; object != NULL; object = object->next_object) {
            if(strcmp (object->metadata.path, title) == 0) {
                found = object;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    pthread_mutex_unlock (&g_database_lock);
    return found;
}

int filterObjects (int ohfiParent, metadata_t **p_head) {
    pthread_mutex_lock (&g_database_lock);
    int numObjects = 0;
    metadata_t temp = {0};
    struct cma_object *db_objects = (struct cma_object*)g_database;
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
    pthread_mutex_unlock (&g_database_lock);
    return numObjects;
}
