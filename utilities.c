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
#include <unistd.h>

#include "opencma.h"

int createNewDirectory (const char *name) {
    return mkdir (name, 0777);
}

int createNewFile (const char *name) {
    int fd = open (name, O_WRONLY | O_CREAT | O_EXCL, 0777);
    if (fd < 0) {
        return fd;
    }
    return close (fd);
}

int readFileToBuffer (const char *name, size_t seek, unsigned char **data, unsigned int *len) {
    FILE *file = fopen (name, "r");
    if (file == NULL) {
        return -1;
    }
    unsigned int buflen = *len;
    unsigned char *buffer;
    if (buflen == 0) {
        if (fseek (file, 0, SEEK_END) < 0) {
            return -1;
        }
        buflen = (unsigned int)ftell (file);
    }
    fseek (file, seek, SEEK_SET);
    buffer = malloc (buflen);
    if (buffer == NULL) {
        fclose (file);
        return -1;
    }
    if (fread (buffer, sizeof (char), buflen, file) < buflen) {
        free (buffer);
        fclose (file);
        return -1;
    }
    fclose (file);
    *data = buffer;
    *len = buflen;
    return 0;
}

int writeFileFromBuffer (const char *name, size_t seek, unsigned char *data, size_t len) {
    FILE *file = fopen (name, "a+");
    if (file == NULL) {
        return -1;
    }
    fseek (file, seek, SEEK_SET);
    if (fwrite (data, sizeof (char), len, file) < len) {
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

char *strreplace (const char *haystack, const char *find, const char *replace) {
    char *newstr;
    off_t off = strstr (haystack, find) - haystack;
    if (off < 0) { // not found
        return strdup (haystack);
    }
    asprintf (&newstr, "%.*s%s%s", (int)off, haystack, replace, haystack + off + strlen (find));
    return newstr;
}
