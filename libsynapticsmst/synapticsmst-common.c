/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2016 Mario Limonciello <mario.limonciello@dell.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
//#include <string.h>
#include <stdlib.h>
#include "synapticsmst-common.h"

#define UNIT_SIZE       32
#define MAX_WAIT_TIME   3  // second

typedef enum {
    DPCD_SUCCESS = 0,
    DPCD_SEEK_FAIL,
    DPCD_ACCESS_FAIL,
}dpcd_return;

int fd = 0;

unsigned char
synapticsmst_common_read_dpcd(int offset, int *buf, int length) 
{
    if (lseek(fd, offset, SEEK_SET) != offset) {
    //    printf("dpcd read fail in finding address %05x\n", offset);
        return DPCD_SEEK_FAIL;
    }
    
    if (read(fd, buf, length) != length) {
    //    printf("dpcd read fail reading from address %05x\n", offset);
        return DPCD_ACCESS_FAIL;
    }

    return DPCD_SUCCESS;
}

unsigned char
synapticsmst_common_write_dpcd(int offset, int *buf, int length) 
{
    if (lseek(fd, offset, SEEK_SET) != offset) {
    //    printf("dpcd write fail in finding address %05x\n", offset);
        return DPCD_SEEK_FAIL;
    }
    
    if (write(fd, buf, length) != length) {
    //    printf("dpcd write fail in writing to address %05x\n", offset);
        return DPCD_ACCESS_FAIL;
    }

    return DPCD_SUCCESS;
}


unsigned char
synapticsmst_common_open_aux_node(const char* filename) 
{
    unsigned char byte[4];
    fd = open(filename, O_RDWR);

    if (synapticsmst_common_read_dpcd(REG_RC_CAP, (int *)byte, 1) == DPCD_SUCCESS) {
        if (byte[0] & 0x04) {
            synapticsmst_common_read_dpcd(REG_VENDOR_ID, (int *)byte, 3);
            if (byte[0] == 0x90 && byte[1] == 0xCC && byte[2] == 0x24) {
                return 1;
            }
        }
    }

    fd = 0;
    return 0;
}

void
synapticsmst_common_close_aux_node(void) 
{
    close(fd);
}

unsigned char
synapticsmst_common_rc_set_command(int rc_cmd, int length, int offset, unsigned char *buf)
{
    unsigned char nRet = 0;
    int cur_offset = offset;
    int cur_length;
    int data_left = length;
    int cmd;
    int readData = 0;
    long deadline;
    struct timespec t_spec;

    do{
        if (data_left > UNIT_SIZE) {
            cur_length = UNIT_SIZE;
        }
        else {
            cur_length = data_left;
        }

        if (cur_length) {
            // write data
            nRet = synapticsmst_common_write_dpcd(REG_RC_DATA, (int *)buf, cur_length);
            if (nRet) {
                break;
            }

            //write offset
            nRet = synapticsmst_common_write_dpcd(REG_RC_OFFSET, &cur_offset, 4);
            if (nRet) {
                break;
            }

            // write length
            nRet = synapticsmst_common_write_dpcd(REG_RC_LEN, &cur_length, 4);
            if (nRet) {
                break;
            }
        }

        // send command
        cmd = 0x80 | rc_cmd;
        nRet = synapticsmst_common_write_dpcd(REG_RC_CMD, &cmd, 1);
        if (nRet) {
            break;
        }

        // wait command complete
        clock_gettime(CLOCK_REALTIME, &t_spec);
        deadline = t_spec.tv_sec + MAX_WAIT_TIME;

        do {
            nRet = synapticsmst_common_read_dpcd(REG_RC_CMD, &readData, 2);
            clock_gettime(CLOCK_REALTIME, &t_spec);
            if (t_spec.tv_sec > deadline) {
                nRet = -1;
            }
        }while(nRet == 0 && readData & 0x80);

        if (nRet) {
        //    printf("checking result timeout\n");
            break;
        }
        else if (readData & 0xFF00) {
            nRet = (readData >> 8) & 0xFF;
            break;
        }

        buf += cur_length;
        cur_offset += cur_length;
        data_left -= cur_length;
    }while(data_left);

    return nRet;
}

unsigned char
synapticsmst_common_rc_get_command(int rc_cmd, int length, int offset, unsigned char *buf) 
{
    unsigned char nRet = 0;
    int cur_offset = offset;
    int cur_length;
    int data_need = length;
    int cmd;
    int readData = 0;
    long deadline;
    struct timespec t_spec;

    while (data_need) {
        if (data_need > UNIT_SIZE) {
            cur_length = UNIT_SIZE;
        }
        else {
            cur_length = data_need;
        }

        if (cur_length) {
            //write offset
            nRet = synapticsmst_common_write_dpcd(REG_RC_OFFSET, &cur_offset, 4);
            if (nRet) {
                break;
            }

            // write length
            nRet = synapticsmst_common_write_dpcd(REG_RC_LEN, &cur_length, 4);
            if (nRet) {
                break;
            }
        }

        // send command
        cmd = 0x80 | rc_cmd;
        nRet = synapticsmst_common_write_dpcd(REG_RC_CMD, &cmd, 1);
        if (nRet) {
            break;
        }

        // wait command complete
        clock_gettime(CLOCK_REALTIME, &t_spec);
        deadline = t_spec.tv_sec + MAX_WAIT_TIME;

        do {
            nRet = synapticsmst_common_read_dpcd(REG_RC_CMD, &readData, 2);
            clock_gettime(CLOCK_REALTIME, &t_spec);
            if (t_spec.tv_sec > deadline) {
                nRet = -1;
            }
        }while(nRet == 0 && readData & 0x80);

        if (nRet) {
        //    printf("checking result timeout\n");
            break;
        }
        else if (readData & 0xFF00) {
            nRet = (readData >> 8) & 0xFF;
            break;
        }

        if (cur_length) {
            nRet = synapticsmst_common_read_dpcd(REG_RC_DATA, (int *)buf, cur_length);
            if (nRet) {
                break;
            }
        }

        buf += cur_length;
        cur_offset += cur_length;
        data_need -= cur_length;
    }

    return nRet;
}

unsigned char
synapticsmst_common_rc_special_get_command(int rc_cmd, int cmd_length, int cmd_offset, unsigned char *cmd_data, int length, unsigned char *buf)
{
    unsigned char nRet = 0;
    int readData = 0;
    int cmd;
    long deadline;
    struct timespec t_spec;

    do {
        if (cmd_length) {
            // write cmd data
            if (cmd_data != NULL) {
                nRet = synapticsmst_common_write_dpcd(REG_RC_DATA, cmd_data, cmd_length);
                if (nRet) {
                    break;
                }
            }

            // write offset
            nRet = synapticsmst_common_write_dpcd(REG_RC_OFFSET, &cmd_offset, 4);
            if (nRet) {
                break;
            }

            // write length
            nRet = synapticsmst_common_write_dpcd(REG_RC_LEN, &cmd_length, 4);
            if (nRet) {
                break;
            }
        }

        // send command
        cmd = 0x80 | rc_cmd;
        nRet = synapticsmst_common_write_dpcd(REG_RC_CMD, &cmd, 1);
        if (nRet) {
            break;
        }

        // wait command complete
        clock_gettime(CLOCK_REALTIME, &t_spec);
        deadline = t_spec.tv_sec + MAX_WAIT_TIME;

        do {
            nRet = synapticsmst_common_read_dpcd(REG_RC_CMD, &readData, 2);
            clock_gettime(CLOCK_REALTIME, &t_spec);
            if (t_spec.tv_sec > deadline) {
                nRet = -1;
            }
        }while(nRet == 0 && readData & 0x80);

        if (nRet) {
        //    printf("checking result timeout\n");
            break;
        }
        else if (readData & 0xFF00) {
            nRet = (readData >> 8) & 0xFF;
            break;
        }

        if (length) {
            nRet = synapticsmst_common_read_dpcd(REG_RC_DATA, (int *)buf, length);
            if (nRet) {
                break;
            }
        }
    } while (0);

    return nRet;    
}
