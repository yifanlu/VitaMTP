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

#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "opencma.h"

extern struct cma_paths g_paths;

int createNewDirectory (const char *name) {
    return mkdir (name, 0777);
}

int createNewFile (const char *name) {
    int fd = open (name, O_WRONLY | O_CREAT | O_EXCL, 0777);
    if (fd < 0) {
        LOG (LERROR, "Creation of %s failed!\n", name);
        return fd;
    }
    return close (fd);
}

int readFileToBuffer (const char *name, size_t seek, unsigned char **p_data, unsigned int *p_len) {
    FILE *file = fopen (name, "r");
    if (file == NULL) {
        LOG (LERROR, "Cannot open %s for reading.\n", name);
        return -1;
    }
    unsigned int buflen = *p_len;
    unsigned char *buffer;
    if (buflen == 0) {
        if (fseek (file, 0, SEEK_END) < 0) {
            LOG (LERROR, "Cannot seek to end of file.\n");
            return -1;
        }
        buflen = (unsigned int)ftell (file);
    }
    if (fseek (file, seek, SEEK_SET) < 0) {
        LOG (LERROR, "Cannot seek to %zu.\n", seek);
        fclose (file);
        return -1;
    }
    buffer = malloc (buflen);
    if (buffer == NULL) {
        fclose (file);
        LOG (LERROR, "Out of memory!");
        return -1;
    }
    if (fread (buffer, sizeof (char), buflen, file) < buflen) {
        free (buffer);
        fclose (file);
        LOG (LERROR, "Read short of %u bytes.\n", buflen);
        return -1;
    }
    fclose (file);
    *p_data = buffer;
    *p_len = buflen;
    return 0;
}

int writeFileFromBuffer (const char *name, size_t seek, unsigned char *data, size_t len) {
    FILE *file = fopen (name, "a+");
    if (file == NULL) {
        LOG (LERROR, "Cannot open %s for writing.\n", name);
        return -1;
    }
    if (fseek (file, seek, SEEK_SET) < 0) {
        LOG (LERROR, "Cannot seek to %zu.\n", seek);
        fclose (file);
        return -1;
    }
    if (fwrite (data, sizeof (char), len, file) < len) {
        LOG (LERROR, "Write short of %zu bytes.\n", len);
        fclose (file);
        return -1;
    }
    fclose (file);
    return 0;
}

int deleteEntry (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    return remove (fpath);
}

void deleteAll (const char *path) {
    // todo: more portable implementation
    nftw (path, deleteEntry, FD_SETSIZE, FTW_DEPTH | FTW_PHYS);
}

int getDiskSpace (const char *path, size_t *free, size_t *total) {
    struct statvfs stat;
    if (statvfs (path, &stat) < 0) {
        LOG (LERROR, "Stat failed!\n");
        return -1;
    }
    *total = stat.f_frsize * stat.f_blocks;
    *free = stat.f_frsize * stat.f_bfree;
    return 0;
}

int requestURL (const char *url, unsigned char **p_data, unsigned int *p_len) {
    char *name;
    size_t len;
    url = strrchr (url, '/');
    if (url == NULL) {
        LOG (LERROR, "URL is malformed.\n");
        return -1;
    }
    url++; // get request name
    len = strcspn (url, "?");
    asprintf (&name, "%s/%.*s", g_paths.urlPath, (int)len, url);
    int ret = readFileToBuffer (name, 0, p_data, p_len);
    LOG (LDEBUG, "Reading of %s returned %d.\n", name, ret);
    free (name);
    return ret;
}

char *strreplace (const char *haystack, const char *find, const char *replace) {
    char *newstr;
    off_t off = strstr (haystack, find) - haystack;
    if (off < 0) { // not found
        return strdup (haystack);
    }
    asprintf (&newstr, "%.*s%s%s", (int)off, haystack, replace, haystack + off + strlen (find));
    return newstr;
}
