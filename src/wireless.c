//
//  Connecting to a Vita PTP/IP device
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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ptp.h"
#include "vitamtp.h"

#define REQUEST_BUFFER_SIZE 100
#define RESPONSE_MAX_SIZE 100

struct vita_device
{
    PTPParams *params;
    char guid[33];
    struct vita_network {
        int init_sockfd;
        struct sockaddr_in addr;
        int data_port;
    } network_device;
};

extern int g_VitaMTP_logmask;
static int g_stopbroadcast;

void VitaMTP_hex_dump(const unsigned char *data, unsigned int size, unsigned int num);

static int VitaMTP_Connect_PTPIP_Device(vita_device_t *device) {
    return -1;
}

static int VitaMTP_Sock_Read_All(int sockfd, unsigned char **p_data, size_t *p_len, struct sockaddr *src_addr, socklen_t *addrlen) {
    unsigned char buffer[REQUEST_BUFFER_SIZE];
    unsigned char *data = NULL;
    ssize_t len = 0;
    while (1) {
        ssize_t clen;
        if ((clen = recvfrom(sockfd, buffer, REQUEST_BUFFER_SIZE, MSG_DONTWAIT, src_addr, addrlen)) < 0) {
            if (errno == EWOULDBLOCK) {
                break;
            }
            VitaMTP_Log(VitaMTP_ERROR, "error recieving data\n");
            free(data);
            return -1;
        }
        VitaMTP_Log(VitaMTP_DEBUG, "Recieved %d bytes from socket %d\n", (unsigned int)clen, sockfd);
        if (MASK_SET(g_VitaMTP_logmask, VitaMTP_DEBUG)) {
            VitaMTP_hex_dump(buffer, (unsigned int)clen, 16);
        }
        data = realloc(data, len+clen);
        memcpy(data+len, buffer, clen);
        len += clen;
    }
    *p_data = data;
    *p_len = len;
    return 0;
}

static int VitaMTP_Sock_Write_All(int sockfd, const unsigned char *data, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen) {
    while (1) {
        ssize_t clen;
        if ((clen = sendto(sockfd, data, len, 0, dest_addr, addrlen)) == len) {
            break;
        }
        if (clen < 0) {
            return -1;
        }
        VitaMTP_Log(VitaMTP_DEBUG, "Sent %d bytes to socket %d\n", (unsigned int)clen, sockfd);
        if (MASK_SET(g_VitaMTP_logmask, VitaMTP_DEBUG)) {
            VitaMTP_hex_dump(data, (unsigned int)clen, 16);
        }
        data += clen;
        len -= clen;
    }
    return 0;
}

int VitaMTP_Broadcast_Host(wireless_host_info_t *info, unsigned int host_addr) {
    char *host_response;
    if (asprintf(&host_response, "HTTP/1.1 200 OK\nhost-id:%s\nhost-type:%s\nhost-name:%s\nhost-mtp-protocol-version:%08d\nhost-request-port:%d\nhost-wireless-protocol-version:%08d\n", info->guid, info->type, info->name, VITAMTP_PROTOCOL_MAX_VERSION, info->port, VITAMTP_WIRELESS_MAX_VERSION) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "out of memory\n");
        free(host_response);
        return -1;
    }
    
    int sock;
    struct sockaddr_in si_host;
    struct sockaddr_in si_client;
    unsigned int slen = sizeof(si_client);
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot create broadcast socket\n");
        free(host_response);
        return -1;
    }
    memset(&si_host, 0, sizeof(si_host));
    si_host.sin_family = AF_INET;
    si_host.sin_port = htons(info->port);
    si_host.sin_addr.s_addr = host_addr ? htonl(host_addr) : htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&si_host, sizeof(si_host)) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot bind listening socket\n");
        free(host_response);
        return -1;
    }
    
    char *data;
    size_t len;
    g_stopbroadcast = 0;
    while (!g_stopbroadcast) {
        if (VitaMTP_Sock_Read_All(sock, (unsigned char **)&data, &len, (struct sockaddr *)&si_client, &slen) < 0) {
            VitaMTP_Log(VitaMTP_ERROR, "error recieving data\n");
            free(host_response);
            close(sock);
            return -1;
        }
        if (strcmp(data, "SRCH * HTTP/1.1\n")) {
            VitaMTP_Log(VitaMTP_DEBUG, "Unknown request: %.*s\n", (int)len, data);
            free(data);
            continue;
        }
        if (sendto(sock, host_response, strlen(host_response)+1, 0, (struct sockaddr *)&si_client, slen) < 0) {
            VitaMTP_Log(VitaMTP_ERROR, "error sending response\n");
            free(host_response);
            free(data);
            close(sock);
            return -1;
        }
    }
    
    free(host_response);
    free(data);
    close(sock);
    return 0;
}

void VitaMTP_Stop_Broadcast() {
    g_stopbroadcast = 1;
}

static inline void VitaMTP_Parse_Device_Headers(char *data, wireless_vita_info_t *info, char **p_host, char **p_pin) {
    char *info_str = strtok(data, "\n");
    while (info_str != NULL) {
        if (strncmp(info_str, "host-id:", strlen("host-id:")) == 0) {
            if (p_host) *p_host = info_str + strlen("host-id:");
        } else if (strncmp(info_str, "device-id:", strlen("device-id:")) == 0) {
            info->deviceid = info_str + strlen("device-id:");
        } else if (strncmp(info_str, "device-type:", strlen("device-type:")) == 0) {
            info->type = info_str + strlen("device-type:");
        } else if (strncmp(info_str, "device-mac-address:", strlen("device-mac-address:")) == 0) {
            info->mac_addr = info_str + strlen("device-mac-address:");
        } else if (strncmp(info_str, "device-name:", strlen("device-name:")) == 0) {
            info->name = info_str + strlen("device-name:");
        } else if (strncmp(info_str, "pin-code:", strlen("pin-code:")) == 0) {
            if (p_pin) *p_pin = info_str + strlen("pin-code:");
        } else {
            VitaMTP_Log(VitaMTP_INFO, "Unknown field in Vita registration request: %s\n", info_str);
        }
        info_str = strtok(NULL, "\n");
    }
}

static int VitaMTP_Get_Wireless_Device(wireless_host_info_t *info, vita_device_t *device, unsigned int host_addr, int timeout, device_registered_callback_t is_registered, register_device_callback_t create_register_pin) {
    int s_sock;
    unsigned int slen;
    struct sockaddr_in si_host;
    struct sockaddr_in si_client;
    int flags;
    if ((s_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot create server socket\n");
        return -1;
    }
    if ((flags = fcntl(s_sock, F_GETFL, 0)) < 0) {
        flags = 0;
    }
    if (fcntl(s_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot mark server socket as non-blocking\n");
        close(s_sock);
        return -1;
    }
    memset(&si_host, 0, sizeof(si_host));
    si_host.sin_family = AF_INET;
    si_host.sin_port = htons(info->port);
    si_host.sin_addr.s_addr = host_addr ? htonl(host_addr) : htonl(INADDR_ANY);
    if (bind(s_sock, (struct sockaddr *)&s_sock, sizeof(s_sock)) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot bind server socket\n");
        close(s_sock);
        return -1;
    }
    if (listen(s_sock, SOMAXCONN) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "cannot listen on server socket\n");
        close(s_sock);
        return -1;
    }
    
    fd_set fd;
    struct timeval time = {0};
    int ret;
    int c_sock = -1;
    char *data = NULL;
    size_t len;
    char method[20];
    int read;
    int pin = -1;
    int listen = 1;
    memset(device, 0, sizeof(vita_device_t));
    while (listen) {
        FD_ZERO(&fd);
        FD_SET(s_sock, &fd);
        if (timeout) time.tv_sec = timeout;
        // use select for the timeout feature, ignore fd
        if ((ret = select(s_sock, &fd, NULL, NULL, timeout ? &time : NULL)) < 0) {
            VitaMTP_Log(VitaMTP_ERROR, "Error polling listener\n");
            break;
        } else if (ret == 0) {
            VitaMTP_Log(VitaMTP_INFO, "Listening timed out.\n");
            break;
        }
        slen = sizeof(si_client);
        if ((c_sock = accept(s_sock, (struct sockaddr *)&si_client, &slen)) < 0) {
            VitaMTP_Log(VitaMTP_ERROR, "Error accepting connection\n");
            break;
        }
        VitaMTP_Log(VitaMTP_DEBUG, "Found new client.\n");
        while (1) {
            char resp[RESPONSE_MAX_SIZE];
            if (VitaMTP_Sock_Read_All(c_sock, (unsigned char **)&data, &len, NULL, NULL) < 0) {
                VitaMTP_Log(VitaMTP_ERROR, "Error reading from client\n");
                listen = 0;
                break;
            }
            if (len == 0) {
                pin = -1; // reset any current registration
                break; // connection closed
            }
            if (sscanf(data, "%20s * HTTP/1.1\n%n", method, &read) < 2) {
                VitaMTP_Log(VitaMTP_ERROR, "Device request malformed: %.*s\n", (int)len, data);
                listen = 0;
                break;
            }
            if (strcmp(method, "CONNECT") == 0) {
                if (sscanf(data+read, "device-id:%32s\ndevice-port:%d\n", device->guid, &device->network_device.data_port) < 2) {
                    VitaMTP_Log(VitaMTP_ERROR, "Error parsing device request\n");
                    listen = 0;
                    break;
                }
                if (is_registered(device->guid)) {
                    strcpy(resp, "HTTP/1.1 210 OK\n");
                } else {
                    strcpy(resp, "HTTP/1.1 605 NG\n");
                }
            } else if (strcmp(method, "SHOWPIN") == 0) {
                wireless_vita_info_t info;
                int err;
                VitaMTP_Parse_Device_Headers(data+read, &info, NULL, NULL);
                strncpy(device->guid, info.deviceid, 32);
                device->guid[32] = '\0';
                // TODO: Check if host GUID is actually our GUID
                const char *okay = "HTTP/1.1 200 OK\n";
                if (VitaMTP_Sock_Write_All(c_sock, (const unsigned char *)okay, strlen(okay)+1, NULL, 0) < 0) {
                    VitaMTP_Log(VitaMTP_ERROR, "Error sending request result\n");
                    listen = 0;
                    break;
                }
                if ((pin = create_register_pin(&info, &err)) < 0) {
                    sprintf(resp, "REGISTERCANCEL * HTTP/1.1\nerrorcode:%d\n", err);
                }
            } else if (strcmp(method, "REGISTER") == 0) {
                wireless_vita_info_t info;
                char *pin_try;
                VitaMTP_Parse_Device_Headers(data+read, &info, NULL, &pin_try);
                if (strcmp(device->guid, info.deviceid)) {
                    VitaMTP_Log(VitaMTP_ERROR, "PIN generated for device %s, but response came from %s!\n", device->guid, info.deviceid);
                    strcpy(resp, "HTTP/1.1 610 NG\n");
                } else if (pin < 0) {
                    VitaMTP_Log(VitaMTP_ERROR, "No PIN generated. Cannot register device %s.\n", info.deviceid);
                    strcpy(resp, "HTTP/1.1 610 NG\n");
                } else if (pin != atoi(pin_try)) {
                    VitaMTP_Log(VitaMTP_ERROR, "PIN mismatch. Correct: %08d, Got: %s\n", pin, pin_try);
                    strcpy(resp, "HTTP/1.1 610 NG\n");
                } else {
                    strcpy(resp, "HTTP/1.1 200 OK\n");
                }
            } else if (strcmp(method, "STANDBY") == 0) {
                device->network_device.init_sockfd = c_sock;
                free(data);
                close(s_sock);
                return VitaMTP_Connect_PTPIP_Device(device);
            } else {
                // no response needed
                if (!(strcmp(method, "REGISTERRESULT") || strcmp(method, "REGISTERCANCEL"))) {
                    VitaMTP_Log(VitaMTP_INFO, "Unkown method %s\n", method);
                }
                free(data);
                continue;
            }
            
            if (VitaMTP_Sock_Write_All(c_sock, (const unsigned char *)resp, strlen(resp)+1, NULL, 0) < 0) {
                VitaMTP_Log(VitaMTP_ERROR, "Error sending request result\n");
                listen = 0;
                break;
            }
            free(data);
        }
        close(c_sock);
    }
    free(data);
    close(c_sock);
    close(s_sock);
    return -1;
}

vita_device_t *VitaMTP_Get_First_Wireless_Device(wireless_host_info_t *info, unsigned int host_addr, int timeout, device_registered_callback_t is_registered, register_device_callback_t create_register_pin) {
    vita_device_t *device = malloc(sizeof(vita_device_t));
    if (device == NULL) {
        VitaMTP_Log(VitaMTP_ERROR, "out of memory\n");
        return NULL;
    }
    if (VitaMTP_Get_Wireless_Device(info, device, host_addr, timeout, is_registered, create_register_pin) < 0) {
        VitaMTP_Log(VitaMTP_ERROR, "error locating Vita\n");
        return NULL;
    }
    return device;
}
