/*
 Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef VM_EXECUTOR_JNI_H
#define VM_EXECUTOR_JNI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "qemu/typedefs.h"

typedef struct Error Error;

extern int qemu_start(int argc, char **argv, char **envp);

extern void qmp_change_vnc_password(const char *password, Error **errp);

extern void qmp_change(const char *device, const char *target, bool has_arg,
		const char *arg, Error **errp);

extern int limbo_savevm(char * limbo_snapshot_name);

extern int get_save_state();

extern int limbo_migrate(const char * uri, char * error);
extern int get_migration_status ();
extern int set_dns_addr_str(const char *dns_addr_str1);

//FIXME: Not supported yet
extern int get_state(int *state);
extern void sendMonitorCommand(const char *cmdline);
extern void toggleFullScreen();

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_stop(
		JNIEnv* env, jobject thiz);

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_start(
		JNIEnv* env, jobject thiz);

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_save(
		JNIEnv* env, jobject thiz);
void print_stack_trace();

#endif

