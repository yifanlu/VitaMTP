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

#define _GNU_SOURCE
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitamtp.h>

#include "opencma.h"

#ifdef _WIN32
extern int asprintf(char **ret, const char *format, ...);
#endif

struct cma_database *g_database;
int g_ohfi_count;
pthread_mutexattr_t g_database_lock_attr;
pthread_mutex_t g_database_lock;

static inline void initDatabase(struct cma_paths *paths, const char *uuid)
{
    pthread_mutex_lock(&g_database_lock);
    g_ohfi_count = OHFI_OFFSET;

    g_database->photos.metadata.ohfi = VITA_OHFI_PHOTO;
    g_database->photos.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->photos.metadata.dataType = Photo;
    g_database->photos.path = strdup(paths->photosPath);
    g_database->photos.num_filters = 2;
    g_database->photos.filters = calloc(2, sizeof(metadata_t));
    createFilter(&g_database->photos, &g_database->photos.filters[0], "Folders",
                 VITA_DIR_TYPE_MASK_PHOTO | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR);
    createFilter(&g_database->photos, &g_database->photos.filters[1], "All",
                 VITA_DIR_TYPE_MASK_PHOTO | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_ALL);
    g_database->videos.metadata.ohfi = VITA_OHFI_VIDEO;
    g_database->videos.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->videos.metadata.dataType = Video;
    g_database->videos.path = strdup(paths->videosPath);
    g_database->videos.num_filters = 2;
    g_database->videos.filters = calloc(2, sizeof(metadata_t));
    createFilter(&g_database->videos, &g_database->videos.filters[0], "Folders",
                 VITA_DIR_TYPE_MASK_VIDEO | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR);
    createFilter(&g_database->videos, &g_database->videos.filters[1], "All",
                 VITA_DIR_TYPE_MASK_VIDEO | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_ALL);
    g_database->music.metadata.ohfi = VITA_OHFI_MUSIC;
    g_database->music.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->music.metadata.dataType = Music;
    g_database->music.path = strdup(paths->musicPath);
    g_database->music.num_filters = 1;
    g_database->music.filters = calloc(1, sizeof(metadata_t));
    //createFilter (&g_database->music, &g_database->music.filters[0], "Folders", VITA_DIR_TYPE_MASK_MUSIC | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_PLAYLISTS); // folders not supported for music
    createFilter(&g_database->music, &g_database->music.filters[0], "All",
                 VITA_DIR_TYPE_MASK_MUSIC | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_SONGS);
    // TODO: Once metadata reading from files is done, add more special folder filters
    g_database->vitaApps.metadata.ohfi = VITA_OHFI_VITAAPP;
    g_database->vitaApps.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->vitaApps.metadata.dataType = App;
    asprintf(&g_database->vitaApps.path, "%s/%s/%s", paths->appsPath, "APP", uuid);
    g_database->pspApps.metadata.ohfi = VITA_OHFI_PSPAPP;
    g_database->pspApps.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->pspApps.metadata.dataType = App;
    asprintf(&g_database->pspApps.path, "%s/%s/%s", paths->appsPath, "PGAME", uuid);
    g_database->pspSaves.metadata.ohfi = VITA_OHFI_PSPSAVE;
    g_database->pspSaves.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->pspSaves.metadata.dataType = SaveData;
    asprintf(&g_database->pspSaves.path, "%s/%s/%s", paths->appsPath, "PSAVEDATA", uuid);
    g_database->psxApps.metadata.ohfi = VITA_OHFI_PSXAPP;
    g_database->psxApps.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->psxApps.metadata.dataType = App;
    asprintf(&g_database->psxApps.path, "%s/%s/%s", paths->appsPath, "PSGAME", uuid);
    g_database->psmApps.metadata.ohfi = VITA_OHFI_PSMAPP;
    g_database->psmApps.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->psmApps.metadata.dataType = App;
    asprintf(&g_database->psmApps.path, "%s/%s/%s", paths->appsPath, "PSM", uuid);
    g_database->backups.metadata.ohfi = VITA_OHFI_BACKUP;
    g_database->backups.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->backups.metadata.dataType = App;
    asprintf(&g_database->backups.path, "%s/%s/%s", paths->appsPath, "SYSTEM", uuid);
#ifndef NO_PACKAGE_INSTALLER
    g_database->packages.metadata.ohfi = VITA_OHFI_PACKAGE;
    g_database->packages.metadata.type = VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_REGULAR;
    g_database->packages.metadata.dataType = Game;
    g_database->packages.path = strdup(paths->packagesPath);
#endif
    pthread_mutex_unlock(&g_database_lock);
}

// the database is structured as an array of linked list, each list representing a category (saves, vita games, etc)
// each linked list contains the objects, files, directories, etc
void createDatabase(struct cma_paths *paths, const char *uuid)
{
    pthread_mutexattr_init(&g_database_lock_attr);
    pthread_mutexattr_settype(&g_database_lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_database_lock, &g_database_lock_attr);

    pthread_mutex_lock(&g_database_lock);
    g_database = malloc(sizeof(struct cma_database));
    memset(g_database, 0, sizeof(struct cma_database));
    initDatabase(paths, uuid);
    int i;
    struct cma_object *current;
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object *)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);

    // loop through all the master objects
    for (i = 0; i < count; i++)
    {
        current = &db_objects[i];
        addEntriesForDirectory(current, current->metadata.ohfi);
    }

    pthread_mutex_unlock(&g_database_lock);
}

static void freeCMAObject(struct cma_object *obj)
{
    if (obj == NULL)
        return;

    metadata_t *meta = &obj->metadata;
    free(meta->name);
    free(meta->path);

    if (MASK_SET(meta->dataType, SaveData | Folder))
    {
        free(meta->data.saveData.detail);
        free(meta->data.saveData.dirName);
        free(meta->data.saveData.savedataTitle);
    }
    else if (MASK_SET(meta->dataType, Photo | File))
    {
        free(meta->data.photo.tracks);
        free(meta->data.photo.title);
        free(meta->data.photo.fileName);
    }
    else if (MASK_SET(meta->dataType, Music | File))
    {
        free(meta->data.music.tracks);
        free(meta->data.music.title);
        free(meta->data.music.fileName);
        free(meta->data.music.album);
        free(meta->data.music.artist);
    }
    else if (MASK_SET(meta->dataType, Video | File))
    {
        free(meta->data.video.title);
        free(meta->data.video.explanation);
        free(meta->data.video.fileName);
        free(meta->data.video.copyright);
        free(meta->data.video.tracks);
    }

    free(obj->path);
    free(obj->filters);

    if (obj->metadata.ohfi >= OHFI_OFFSET)
    {
        free(obj);
    }
}

void destroyDatabase()
{
    if (g_database == NULL)
    {
        return; // can't destroy what hasn't been created
    }

    pthread_mutex_lock(&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object *)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;

    // loop through all the master objects
    for (i = 0; i < count; i++)
    {
        struct cma_object *next = NULL;
        struct cma_object *current;

        for (current = &db_objects[i]; current != NULL; current = next)
        {
            next = current->next_object;
            freeCMAObject(current);
        }
    }

    pthread_mutex_unlock(&g_database_lock);

    pthread_mutex_destroy(&g_database_lock);
    pthread_mutexattr_destroy(&g_database_lock_attr);
    free(g_database);
    g_database = NULL;
}

inline void lockDatabase()
{
    pthread_mutex_lock(&g_database_lock);
}

inline void unlockDatabase()
{
    pthread_mutex_unlock(&g_database_lock);
}

struct cma_object *addToDatabase(struct cma_object *root, const char *name, size_t size, const enum DataType type)
{
    pthread_mutex_lock(&g_database_lock);
    struct cma_object *current = malloc(sizeof(struct cma_object));
    memset(current, 0, sizeof(struct cma_object));
    current->metadata.name = strdup(name);
    current->metadata.ohfiParent = root->metadata.ohfi;
    current->metadata.ohfi = g_ohfi_count++;
    current->metadata.type = VITA_DIR_TYPE_MASK_REGULAR; // ignored for files
    current->metadata.dateTimeCreated = 0; // TODO: allow for time created
    current->metadata.size = size;
    current->metadata.dataType = type | (root->metadata.dataType & ~Folder); // get parent attributes except Folder

    // create additional metadata
    // TODO: Read real metadata for files
    if (MASK_SET(current->metadata.dataType, SaveData | Folder))
    {
        current->metadata.data.saveData.title = strdup(name);
        current->metadata.data.saveData.detail = strdup("");
        current->metadata.data.saveData.dirName = strdup(name);
        current->metadata.data.saveData.savedataTitle = strdup("");
        current->metadata.data.saveData.dateTimeUpdated = 0;
        current->metadata.data.saveData.statusType = 1;
    }
    else if (MASK_SET(current->metadata.dataType, Photo | File))
    {
        current->metadata.data.photo.title = strdup(name);
        current->metadata.data.photo.fileName = strdup(name);
        current->metadata.data.photo.fileFormatType = 28; // working
        current->metadata.data.photo.statusType = 1;
        current->metadata.data.photo.dateTimeOriginal = 0;
        current->metadata.data.photo.numTracks = 1;
        current->metadata.data.photo.tracks = malloc(sizeof(struct media_track));
        memset(current->metadata.data.photo.tracks, 0, sizeof(struct media_track));
        current->metadata.data.photo.tracks->type = VITA_TRACK_TYPE_PHOTO;
        current->metadata.data.photo.tracks->data.track_photo.codecType = 17; // JPEG?
    }
    else if (MASK_SET(current->metadata.dataType, Music | File))
    {
        current->metadata.data.music.title = strdup(name);
        current->metadata.data.music.fileName = strdup(name);;
        current->metadata.data.music.fileFormatType = 20;
        current->metadata.data.music.statusType = 1;
        current->metadata.data.music.album = strdup(root->metadata.name ? root->metadata.name : "");
        current->metadata.data.music.artist = strdup("");
        current->metadata.data.music.numTracks = 1;
        current->metadata.data.music.tracks = malloc(sizeof(struct media_track));
        memset(current->metadata.data.music.tracks, 0, sizeof(struct media_track));
        current->metadata.data.music.tracks->type = VITA_TRACK_TYPE_AUDIO;
        current->metadata.data.music.tracks->data.track_photo.codecType = 12; // MP3?
    }
    else if (MASK_SET(current->metadata.dataType, Video | File))
    {
        current->metadata.data.video.title = strdup(name);
        current->metadata.data.video.explanation = strdup("");
        current->metadata.data.video.fileName = strdup(name);
        current->metadata.data.video.copyright = strdup("");
        current->metadata.data.video.dateTimeUpdated = 0;
        current->metadata.data.video.statusType = 1;
        current->metadata.data.video.fileFormatType = 1;
        current->metadata.data.video.parentalLevel = 0;
        current->metadata.data.video.numTracks = 1;
        current->metadata.data.video.tracks = malloc(sizeof(struct media_track));
        memset(current->metadata.data.video.tracks, 0, sizeof(struct media_track));
        current->metadata.data.video.tracks->type = VITA_TRACK_TYPE_VIDEO;
        current->metadata.data.video.tracks->data.track_video.codecType = 3; // this codec is working
    }

    asprintf(&current->path, "%s/%s", root->path, name);

    if (root->metadata.path == NULL)
    {
        current->metadata.path = strdup(name);
    }
    else
    {
        asprintf(&current->metadata.path, "%s/%s", root->metadata.path, name);
    }

    while (root->next_object != NULL)
    {
        root = root->next_object;
    }

    root->next_object = current;
    pthread_mutex_unlock(&g_database_lock);
    return current;
}

void createFilter(struct cma_object *dirobject, metadata_t *output, const char *name, int type)
{
    pthread_mutex_lock(&g_database_lock);
    output->ohfiParent = dirobject->metadata.ohfi;
    output->ohfi = g_ohfi_count++;
    output->name = strdup(name);
    output->path = strdup(dirobject->metadata.path ? dirobject->metadata.path : "");
    output->type = type;
    output->dateTimeCreated = 0;
    output->size = 0;
    output->dataType = Folder | Special;
    output->next_metadata = NULL;
    pthread_mutex_unlock(&g_database_lock);
}

void removeFromDatabase(int ohfi, struct cma_object *start)
{
    pthread_mutex_lock(&g_database_lock);
    struct cma_object **p_object;
    struct cma_object **p_toFree;
    struct cma_object *prev = NULL;

    for (p_object = &start; *p_object != NULL; p_object = &(*p_object)->next_object)
    {
        if ((*p_object)->metadata.ohfi == ohfi)
        {
            p_toFree = p_object;
        }

        if ((*p_object)->metadata.ohfiParent == ohfi)
        {
            // we know that the children must be after the parent in the linked list
            // for this to be a valid database
            if (prev == NULL)
            {
                LOG(LERROR, "Invalid database entry %d\n", ohfi);
                return;
            }
            removeFromDatabase((*p_object)->metadata.ohfi, prev);
            p_object = &prev; // back up one since *p_object is freed
        }

        prev = *p_object; // store "prevous" object
    }

    // do this at the end so we can still see each node
    prev = *p_toFree;
    *p_toFree = prev->next_object;
    pthread_mutex_unlock(&g_database_lock);
    freeCMAObject(prev);
}

void renameRootEntry(struct cma_object *object, const char *name, const char *newname)
{
    pthread_mutex_lock(&g_database_lock);
    struct cma_object *temp;
    char *origPath = object->path;
    char *origRelPath = object->metadata.path;
    char *origName = object->metadata.name;

    if (name == NULL)
    {
        name = origName;
    }

    object->metadata.name = strreplace(origName, name, newname);
    object->metadata.path = strreplace(origRelPath, name, newname);
    object->path = strreplace(origPath, origRelPath, object->metadata.path);

    for (temp = object; temp != NULL; temp = temp->next_object)
    {
        // rename all child objects
        if (temp->metadata.ohfiParent == object->metadata.ohfi)
        {
            char *nname;
            char *nnewname;
            asprintf(&nname, "%s/%s", origRelPath, temp->metadata.name);
            asprintf(&nnewname, "%s/%s", object->metadata.path, temp->metadata.name);
            renameRootEntry(temp, nname, nnewname);
            free(nname);
            free(nnewname);
        }
    }

    free(origPath);
    free(origRelPath);
    free(origName);
    pthread_mutex_unlock(&g_database_lock);
}

struct cma_object *ohfiToObject(int ohfi)
{
    pthread_mutex_lock(&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object *)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    struct cma_object *found = NULL;
    int i;
    int j;

    // loop through all the master objects
    for (i = 0; i < count; i++)
    {
        // first element in loop is the master object, the ones after are it's children
        for (object = &db_objects[i]; object != NULL; object = object->next_object)
        {
            if (object->metadata.ohfi == ohfi)
            {
                found = object;
                break;
            }

            if (object->num_filters > 0)
            {
                for (j = 0; j < object->num_filters; j++)
                {
                    if (object->filters[j].ohfi == ohfi)
                    {
                        found = object;
                        break;
                    }
                }
            }
        }

        if (found)
        {
            break;
        }
    }

    pthread_mutex_unlock(&g_database_lock);
    return found;
}

// ohfiRoot == 0 means look in all lists
struct cma_object *pathToObject(char *path, int ohfiRoot)
{
    pthread_mutex_lock(&g_database_lock);
    // the database is basically an array of cma_objects, so we'll cast it so
    struct cma_object *db_objects = (struct cma_object *)g_database;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    struct cma_object *object;
    struct cma_object *found = NULL;
    int i;

    // loop through all the master objects
    for (i = 0; i < count; i++)
    {
        if (ohfiRoot && db_objects[i].metadata.ohfi != ohfiRoot)
        {
            continue;
        }

        // first element in loop is the master object, the ones after are it's children
        for (object = db_objects[i].next_object; object != NULL; object = object->next_object)
        {
            if (strcmp(object->metadata.path, path) == 0)
            {
                found = object;
                break;
            }
        }

        if (found)
        {
            break;
        }
    }

    pthread_mutex_unlock(&g_database_lock);
    return found;
}

static int acceptFilteredObject(const struct cma_object *parent, const struct cma_object *current, int type)
{
    int result = 0;
    pthread_mutex_lock(&g_database_lock);

    if (MASK_SET(type, VITA_DIR_TYPE_MASK_PHOTO))
    {
        result = (current->metadata.dataType & Photo);
    }
    else if (MASK_SET(type, VITA_DIR_TYPE_MASK_VIDEO))
    {
        result = (current->metadata.dataType & Video);
    }
    else if (MASK_SET(type, VITA_DIR_TYPE_MASK_MUSIC))
    {
        result = (current->metadata.dataType & Music);
    }

    if (type & (VITA_DIR_TYPE_MASK_ALL | VITA_DIR_TYPE_MASK_SONGS))
    {
        result = result && (current->metadata.dataType & File);
    }
    else if (type & (VITA_DIR_TYPE_MASK_REGULAR))
    {
        result = (parent->metadata.ohfi == current->metadata.ohfiParent);
    }

    // TODO: Support other filter types
    pthread_mutex_unlock(&g_database_lock);
    return result;
}

static int getFilters(const struct cma_object *parent, metadata_t **p_head)
{
    int numObjects = 0;
    pthread_mutex_lock(&g_database_lock);
    numObjects = parent->num_filters;

    for (int i = 0; i < numObjects; i++)
    {
        parent->filters[i].next_metadata = &parent->filters[i+1];
    }

    parent->filters[numObjects-1].next_metadata = NULL;

    if (p_head != NULL)
    {
        *p_head = parent->filters;
    }

    pthread_mutex_unlock(&g_database_lock);
    return numObjects;
}

int filterObjects(int ohfiParent, metadata_t **p_head)
{
    pthread_mutex_lock(&g_database_lock);
    int numObjects = 0;
    metadata_t temp = {0};
    struct cma_object *db_objects = (struct cma_object *)g_database;
    struct cma_object *object;
    struct cma_object *parent = ohfiToObject(ohfiParent);

    if (parent == NULL)
    {
        pthread_mutex_unlock(&g_database_lock);
        return 0;
    }

    int type = parent->metadata.type;
    int j;

    if (parent->filters > 0)   // if we have filters
    {
        if (ohfiParent == parent->metadata.ohfi)   // if we are looking at root
        {
            pthread_mutex_unlock(&g_database_lock);
            // return the filter list
            return getFilters(parent, p_head);
        }
        else     // we are looking at a filter
        {
            // get the filter type
            for (j = 0; j < parent->num_filters; j++)
            {
                if (parent->filters[j].ohfi == ohfiParent)
                {
                    type = parent->filters[j].type;
                    break;
                }
            }
        }
    }

    metadata_t *tail = &temp;
    int count = sizeof(struct cma_database) / sizeof(struct cma_object);
    int i;

    for (i = 0; i < count; i++)
    {
        for (object = &db_objects[i]; object != NULL; object = object->next_object)
        {
            if (acceptFilteredObject(parent, object, type))
            {
                tail->next_metadata = &object->metadata;
                tail = tail->next_metadata;
                numObjects++;
            }
        }

        if (numObjects > 0)
        {
            break; // quick speedup to prevent looking at all lists
        }
    }

    tail->next_metadata = NULL;

    if (p_head != NULL)
    {
        *p_head = temp.next_metadata;
    }

    pthread_mutex_unlock(&g_database_lock);
    return numObjects;
}
