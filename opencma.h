//
//  Reference implementation of libvitamtp
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

#ifndef VitaMTP_opencma_h
#define VitaMTP_opencma_h

#define OPENCMA_VERSION_STRING "0.1 alpha"

static const char *HELP_STRING = 
"usage: opencma [options]\n"
"   options\n"
"       -p path     Path to photos\n"
"       -v path     Path to videos\n"
"       -m path     Path to music\n"
"       -a path     Path to apps\n"
"       -d          Start as a daemon (not implemented)\n"
"       -h          Show this help text\n";

struct cma_paths {
    char *photoPath;
    char *videoPath;
    char *musicPath;
    char *appPath;
};

void *vita_event_listener(LIBMTP_mtpdevice_t *device);

#endif
