//
//  Utilities functions
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
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "opencma.h"

#ifdef _WIN32
extern int asprintf(char **ret, const char *format, ...);
#endif

extern struct cma_paths g_paths;

// Windows FS commands
#ifdef _WIN32
int createNewDirectory(const char *path)
{
    char opath[MAX_PATH];
    char *p;
    size_t len;

    len = strlen(path);
    strncpy(opath, path, len);

    if (opath[len - 1] == '\\' || opath[len - 1] == '/')
    {
        opath[len - 1] = '\0';
    }

    for (p = opath; *p; p++)
    {
        if (*p == '\\' || *p == '/')
        {
            *p = '\0';
            
            if (!CreateDirectory(opath, NULL))
            {
                if (GetLastError() == ERROR_ALREADY_EXISTS)
                {
                    // do nothing
                }
                else
                {
                    LOG(LERROR, "Error creating %s, error %d\n", opath, GetLastError());
                    return -1;
                }
            }
            
            *p = '/';
        }
    }

    if (!CreateDirectory(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)          /* if path is not terminated with / */
    {
        return -1;
    }

    return 0;
}

int createNewFile(const char *name)
{
    HANDLE hFile = CreateFile(name, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    return CloseHandle(hFile) ? 0 : -1;
}

int readFileToBuffer(const char *name, size_t seek, unsigned char **p_data, unsigned int *p_len)
{
    LARGE_INTEGER offset = { .QuadPart = seek };
    HANDLE hFile = CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG(LERROR, "Cannot open %s for reading.\n", name);
        return -1;
    }
    
    unsigned int buflen = *p_len;
    unsigned char *buffer;
    
    if (buflen == 0)
    {
        if ((buflen = GetFileSize(hFile, NULL)) == INVALID_FILE_SIZE)
        {
            LOG(LERROR, "Cannot seek to end of file.\n");
            CloseHandle(hFile);
            return -1;
        }
    }
    
    if (SetFilePointer(hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        LOG(LERROR, "Cannot seek to %Iu.\n", seek);
        CloseHandle(hFile);
        return -1;
    }
    
    buffer = malloc(buflen);
    
    if (buffer == NULL)
    {
        LOG(LERROR, "Out of memory!");
        CloseHandle(hFile);
        return -1;
    }
    
    DWORD read;
    if (!ReadFile(hFile, buffer, buflen, &read, NULL) || read < buflen)
    {
        LOG(LERROR, "Read short of %u bytes.\n", buflen);
        free(buffer);
        CloseHandle(hFile);
        return -1;
    }
    
    CloseHandle(hFile);
    *p_data = buffer;
    *p_len = buflen;
    return 0;
}

int writeFileFromBuffer(const char *name, size_t seek, unsigned char *data, size_t len)
{
    LARGE_INTEGER offset = { .QuadPart = seek };
    HANDLE hFile = CreateFile(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG(LERROR, "Cannot open %s for writing.\n", name);
        return -1;
    }
    
    if (SetFilePointer(hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        LOG(LERROR, "Cannot seek to %Iu.\n", seek);
        CloseHandle(hFile);
        return -1;
    }
    
    DWORD written;
    if (!WriteFile(hFile, data, len, &written, NULL) || written < len)
    {
        LOG(LERROR, "Write short of %Iu bytes.\n", len);
        CloseHandle(hFile);
        return -1;
    }
    
    CloseHandle(hFile);
    return 0;
}

void deleteAll(const char *path)
{
    char pathCpy[MAX_PATH];
    size_t len = strlen(path);
    memcpy(pathCpy, path, len);
    memset(pathCpy+len, 0, 2); // double null terminated path
    SHFILEOPSTRUCT file_op = {
        NULL,
        FO_DELETE,
        pathCpy,
        NULL,
        FOF_NOCONFIRMATION |
        FOF_NOERRORUI |
        FOF_SILENT,
        0,
        0,
        "" };
    SHFileOperation(&file_op);
}

int move(const char *src, const char *dest)
{
    return MoveFile(src, dest) > 0 ? 0 : -1;
}

int fileExists(const char *path)
{
    DWORD dwAttrib = GetFileAttributes(path);
    
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

int getDiskSpace(const char *path, uint64_t *free, uint64_t *total)
{
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;
    
    if (!GetDiskFreeSpace(path, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters))
    {
        LOG(LERROR, "Cannot access %s error %d\n", path, GetLastError());
        return -1;
    }
    
    *total = SectorsPerCluster * BytesPerSector * NumberOfFreeClusters;
    *free = SectorsPerCluster * BytesPerSector * TotalNumberOfClusters;
    return 0;
}

void addEntriesForDirectory(struct cma_object *current, int parent_ohfi)
{
    lockDatabase();
    struct cma_object *last = current;
    WIN32_FIND_DATA ffd;
    char fullpath[MAX_PATH];
    size_t fpath_pos;
    HANDLE hFind;
    
    fullpath[0] = '\0';
    sprintf(fullpath, "%s\\", last->path);
    fpath_pos = strlen(fullpath);
    
    fullpath[fpath_pos] = '*';
    fullpath[fpath_pos+1] = '\0';
    if ((hFind = FindFirstFile(fullpath, &ffd)) == INVALID_HANDLE_VALUE)
    {
        LOG(LERROR, "Cannot find %s\n", fullpath);
        unlockDatabase();
        return;
    }
    fullpath[fpath_pos] = '\0';
    
    unsigned long totalSize = 0;
    do
    {
        LARGE_INTEGER filesize;
        if (ffd.cFileName[0] == '.')
        {
            continue; // ignore hidden folders and ., ..
        }
        
        filesize.LowPart = ffd.nFileSizeLow;
        filesize.HighPart = ffd.nFileSizeHigh;
        
        current = addToDatabase(last, ffd.cFileName, filesize.QuadPart, ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? Folder : File);
        
        if (current->metadata.dataType & Folder)
        {
            addEntriesForDirectory(current, current->metadata.ohfi);
        }
        
        totalSize += current->metadata.size;
    }
    while (FindNextFile(hFind, &ffd) != 0);
    
    unlockDatabase();
}
#else // not _WIN32
// from http://nion.modprobe.de/tmp/mkdir.c
// creates all subdirectories
int createNewDirectory(const char *path)
{
    char opath[PATH_MAX];
    char *p;
    size_t len;

    strncpy(opath, path, sizeof(opath));
    len = strlen(opath);

    if (opath[len - 1] == '/')
    {
        opath[len - 1] = '\0';
    }

    for (p = opath; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';

            if (access(opath, F_OK))
            {
                mkdir(opath, S_IRWXU);
            }

            *p = '/';
        }
    }

    if (access(opath, F_OK))          /* if path is not terminated with / */
    {
        return mkdir(opath, S_IRWXU);
    }

    return 0;
}

int createNewFile(const char *name)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0777);

    if (fd < 0)
    {
        LOG(LERROR, "Creation of %s failed!\n", name);
        return fd;
    }

    return close(fd);
}

int readFileToBuffer(const char *name, size_t seek, unsigned char **p_data, unsigned int *p_len)
{
    FILE *file = fopen(name, "r");

    if (file == NULL)
    {
        LOG(LERROR, "Cannot open %s for reading.\n", name);
        return -1;
    }

    unsigned int buflen = *p_len;
    unsigned char *buffer;

    if (buflen == 0)
    {
        if (fseek(file, 0, SEEK_END) < 0)
        {
            LOG(LERROR, "Cannot seek to end of file.\n");
            fclose(file);
            return -1;
        }

        buflen = (unsigned int)ftell(file);
    }

    if (fseek(file, seek, SEEK_SET) < 0)
    {
        LOG(LERROR, "Cannot seek to %zu.\n", seek);
        fclose(file);
        return -1;
    }

    buffer = malloc(buflen);

    if (buffer == NULL)
    {
        LOG(LERROR, "Out of memory!");
        fclose(file);
        return -1;
    }

    if (fread(buffer, sizeof(char), buflen, file) < buflen)
    {
        LOG(LERROR, "Read short of %u bytes.\n", buflen);
        free(buffer);
        fclose(file);
        return -1;
    }

    fclose(file);
    *p_data = buffer;
    *p_len = buflen;
    return 0;
}

int writeFileFromBuffer(const char *name, size_t seek, unsigned char *data, size_t len)
{
    FILE *file = fopen(name, "a+");

    if (file == NULL)
    {
        LOG(LERROR, "Cannot open %s for writing.\n", name);
        return -1;
    }

    if (fseek(file, seek, SEEK_SET) < 0)
    {
        LOG(LERROR, "Cannot seek to %zu.\n", seek);
        fclose(file);
        return -1;
    }

    if (fwrite(data, sizeof(char), len, file) < len)
    {
        LOG(LERROR, "Write short of %zu bytes.\n", len);
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int deleteEntry(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw)
{
    return remove(fpath);
}

void deleteAll(const char *path)
{
    // todo: more portable implementation
    nftw(path, deleteEntry, FD_SETSIZE, FTW_DEPTH | FTW_PHYS);
}

int move(const char *src, const char *dest)
{
    return rename(src, dest);
}

int fileExists(const char *path)
{
    return access(path, F_OK) == 0;
}

int getDiskSpace(const char *path, uint64_t *free, uint64_t *total)
{
    struct statvfs stat;

    if (statvfs(path, &stat) < 0)
    {
        LOG(LERROR, "Stat failed! Cannot access %s\n", path);
        return -1;
    }

    *total = stat.f_frsize * stat.f_blocks;
    *free = stat.f_frsize * stat.f_bfree;
    return 0;
}

void addEntriesForDirectory(struct cma_object *current, int parent_ohfi)
{
    lockDatabase();
    struct cma_object *last = current;
    char fullpath[PATH_MAX];
    DIR *dirp;
    struct dirent *entry;
    struct stat statbuf;
    size_t fpath_pos;
    
    fullpath[0] = '\0';
    sprintf(fullpath, "%s/", last->path);
    fpath_pos = strlen(fullpath);
    
    if ((dirp = opendir(fullpath)) == NULL)
    {
        unlockDatabase();
        return;
    }
    
    unsigned long totalSize = 0;
    
    while ((entry = readdir(dirp)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue; // ignore hidden folders and ., ..
        }
        
        strcat(fullpath, entry->d_name);
        
        if (stat(fullpath, &statbuf) != 0)
        {
            continue;
        }
        
        current = addToDatabase(last, entry->d_name, statbuf.st_size, S_ISDIR(statbuf.st_mode) ? Folder : File);
        
        if (current->metadata.dataType & Folder)
        {
            addEntriesForDirectory(current, current->metadata.ohfi);
        }
        
        totalSize += current->metadata.size;
        
        fullpath[fpath_pos] = '\0';
    }
    
    last->metadata.size += totalSize;
    closedir(dirp);
    unlockDatabase();
}
#endif // not _WIN32

int requestURL(const char *url, unsigned char **p_data, unsigned int *p_len)
{
    char *name;
    size_t len;
    url = strrchr(url, '/');

    if (url == NULL)
    {
        LOG(LERROR, "URL is malformed.\n");
        return -1;
    }

    url++; // get request name
    len = strcspn(url, "?");

    if (asprintf(&name, "%s/%.*s", g_paths.urlPath, (int)len, url) < 0)
    {
        LOG(LERROR, "Out of memory\n");
        return -1;
    }

    int ret = readFileToBuffer(name, 0, p_data, p_len);
    LOG(LDEBUG, "Reading of %s returned %d.\n", name, ret);
    free(name);
    return ret;
}

char *strreplace(const char *haystack, const char *find, const char *replace)
{
    char *newstr;
    char *found = strstr(haystack, find);
    ptrdiff_t off;

    if (found == NULL)   // not found
    {
        return strdup(haystack);
    }

    off = found - haystack;

    if (asprintf(&newstr, "%.*s%s%s", (int)off, haystack, replace, haystack + off + strlen(find)) < 0)
    {
        LOG(LERROR, "Out of memory\n");
        newstr = NULL;
    }

    return newstr;
}

capability_info_t *generate_pc_capability_info(void)
{
    // TODO: Actually generate this based on OpenCMA's capabilities
    capability_info_t *pc_capabilities;
    pc_capabilities = malloc(sizeof(capability_info_t));
    pc_capabilities->version = "1.0";
    struct capability_info_function *functions = calloc(3, sizeof(struct capability_info_function));
    struct capability_info_format *game_formats = calloc(5, sizeof(struct capability_info_format));
    game_formats[0].contentType = "vitaApp";
    game_formats[0].next_item = &game_formats[1];
    game_formats[1].contentType = "PSPGame";
    game_formats[1].next_item = &game_formats[2];
    game_formats[2].contentType = "PSPSaveData";
    game_formats[2].next_item = &game_formats[3];
    game_formats[3].contentType = "PSGame";
    game_formats[3].next_item = &game_formats[4];
    game_formats[4].contentType = "PSMApp";
    functions[0].type = "game";
    functions[0].formats = game_formats[0];
    functions[0].next_item = &functions[1];
    functions[1].type = "backup";
    functions[1].next_item = &functions[2];
    functions[2].type = "systemUpdate";
    pc_capabilities->functions = functions[0];
    return pc_capabilities;
}

void free_pc_capability_info(capability_info_t *info)
{
    free(&info->functions.formats.next_item[-1]);
    free(&info->functions.next_item[-1]);
    free(info);
}
