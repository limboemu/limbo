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
#include <jni.h>
#include <stdio.h>
#include "limbo_logutils.h"
#include "vm-executor-jni.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <unwind.h>
#include <dlfcn.h>
#include "limbo_compat.h"
#include "limbo_compat_fd.h"


#define MSG_BUFSIZE 1024
#define MAX_STRING_LEN 1024

static int started = 0;
void * handle;

void loadLib(const char * lib_path_str) {

	char res_msg[MAX_STRING_LEN];
	sprintf(res_msg, "Loading lib: %s", lib_path_str);
	LOGV(res_msg);
	handle = dlopen(lib_path_str, RTLD_LAZY);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *pvt) {
	printf("* JNI_OnLoad called\n");
	return JNI_VERSION_1_2;
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_togglefullscreen(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	LOGV("Toggle Fullscreen\n");

	typedef void (*toggleFullScreen_t)();
	dlerror();
	toggleFullScreen_t toggleFullScreen = (toggleFullScreen_t) dlsym(handle,
			"toggleFullScreen");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'toggleFullScreen': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	toggleFullScreen();
	sprintf(res_msg, "Full Screen");
	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

//FIXME: Snapshots Hanging, for now we use QMP Monitor console
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_save(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "snapshot_name",
			"Ljava/lang/String;");
	jstring snapshot_name = (*env)->GetObjectField(env, thiz, fid);
	const char *snapshot_name_str = NULL;
	if (snapshot_name != NULL)
		snapshot_name_str = (*env)->GetStringUTFChars(env, snapshot_name, 0);

	typedef void (*save_vm_t)();
	dlerror();
	save_vm_t limbo_savevm = (save_vm_t) dlsym(handle, "limbo_savevm");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'hmp_savevm': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	limbo_savevm(snapshot_name_str);
	sprintf(res_msg, "VM State Saved");
	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

//Save only VM no snapshot is faster
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_pausevm(
		JNIEnv* env, jobject thiz, jstring juri) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	char error [MSG_BUFSIZE + 1] = { 0 };
	const char * uri_str = NULL;
	if (juri != NULL)
		uri_str = (*env)->GetStringUTFChars(env, juri, 0);

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);

	typedef int (*pause_vm_t)();
	dlerror();
	pause_vm_t limbo_migrate = (pause_vm_t) dlsym(handle, "limbo_migrate");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		sprintf(res_msg, "Cannot load symbol 'limbo_migrate': %s\n", dlsym_error);
		LOGE(res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}

	int res = limbo_migrate(uri_str, error);

	if(res){
		LOGE(error);
		sprintf(res_msg, error);
	}else
		sprintf(res_msg, "VM State Saving Started");

	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_vncchangepassword(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	Error **err;
	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "vnc_passwd",
			"Ljava/lang/String;");
	jstring jvnc_passwd = (*env)->GetObjectField(env, thiz, fid);

	const char *vnc_passwd_str = NULL;
	if (jvnc_passwd != NULL)
		vnc_passwd_str = (*env)->GetStringUTFChars(env, jvnc_passwd, 0);

	typedef void (*vnc_change_pwd_t)();
	dlerror();
	vnc_change_pwd_t qmp_change_vnc_password = (vnc_change_pwd_t) dlsym(handle,
			"qmp_change_vnc_password");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qmp_change_vnc_password': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	qmp_change_vnc_password(vnc_passwd_str, err);

	sprintf(res_msg, "VNC Password Changed");
	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_dnschangeaddr(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "dns_addr", "Ljava/lang/String;");
	jstring jdns_addr = (*env)->GetObjectField(env, thiz, fid);
	const char * dns_addr_str = NULL;
	if (jdns_addr != NULL)
		dns_addr_str = (*env)->GetStringUTFChars(env, jdns_addr, 0);

	fid = (*env)->GetFieldID(env, c, "libqemu", "Ljava/lang/String;");
	jstring jlib_path = (*env)->GetObjectField(env, thiz, fid);
	const char * lib_path_str = NULL;
	if (jlib_path != NULL)
		lib_path_str = (*env)->GetStringUTFChars(env, jlib_path, 0);

	typedef void (*set_dns_addr_str_t)();
	dlerror();
	//LOAD LIB
	if (handle == NULL) {
		loadLib(lib_path_str);
	}

	if (!handle) {
		sprintf(res_msg, "%s: Error opening lib: %s :%s", __func__,
				lib_path_str, dlerror());
		LOGV(res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}
	dlerror();
	set_dns_addr_str_t set_dns_addr_str = (set_dns_addr_str_t) dlsym(handle,
			"set_dns_addr_str");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'set_dns_addr_str': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	set_dns_addr_str(dns_addr_str);

	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_changedev(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	Error **err;
	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "qemu_dev", "Ljava/lang/String;");
	jstring jdev = (*env)->GetObjectField(env, thiz, fid);
	const char *dev = NULL;
	if (jdev != NULL)
		dev = (*env)->GetStringUTFChars(env, jdev, 0);

	fid = (*env)->GetFieldID(env, c, "qemu_dev_value", "Ljava/lang/String;");
	jstring jdev_value = (*env)->GetObjectField(env, thiz, fid);
	const char *dev_value = NULL;
	if (jdev_value != NULL)
		dev_value = (*env)->GetStringUTFChars(env, jdev_value, 0);

	typedef void (*change_dev_t)();
	dlerror();
	change_dev_t qmp_change = (change_dev_t) dlsym(handle, "qmp_change");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qmp_change': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	printf(res_msg, "Changing Device: %s to: %s", dev, dev_value);
	qmp_change(dev, dev_value, 0, NULL, err);

	sprintf(res_msg, "Device %s changed to: %s", dev, dev_value);
	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_ejectdev(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "qemu_dev", "Ljava/lang/String;");
	jstring jdev = (*env)->GetObjectField(env, thiz, fid);
	const char *dev = NULL;
	if (jdev != NULL)
		dev = (*env)->GetStringUTFChars(env, jdev, 0);

	typedef void (*eject_dev_t)();
	dlerror();
	eject_dev_t eject_dev = (eject_dev_t) dlsym(handle, "eject_dev");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'eject_dev': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	printf(res_msg, "Ejecting Device: %s", dev);
	eject_dev(dev);

	sprintf(res_msg, "Device %s ejected", dev);
	LOGV(res_msg);

	return (*env)->NewStringUTF(env, res_msg);
}

//For saving snapshots
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getsavestate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	typedef int (*get_save_state_t)();
	dlerror();
	get_save_state_t get_save_state = (get_save_state_t) dlsym(handle,
			"get_save_state");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_save_state': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	int state = 0;
	state = get_save_state();


	if (state == 0)
		sprintf(res_msg, "NONE");
	else if (state == 1)
		sprintf(res_msg, "SAVING");
	else if (state == 2)
		sprintf(res_msg, "DONE");
	else if (state == -1)
		sprintf(res_msg, "ERROR");

	return (*env)->NewStringUTF(env, res_msg);
}


//For pausing vm
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getpausestate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	typedef int (*get_save_state_t)();
	dlerror();
	get_save_state_t get_migration_status = (get_save_state_t) dlsym(handle,
			"get_migration_status");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_migration_status': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	int state = 0;
	state = get_migration_status();

	if (state == 0)
		sprintf(res_msg, "NONE");
	else if (state == 1)
		sprintf(res_msg, "SAVING");
	else if (state == 2)
		sprintf(res_msg, "DONE");
	else if (state == -1)
		sprintf(res_msg, "ERROR");

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getstate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	typedef int (*get_state_t)();
	dlerror();
	get_state_t get_state = (get_state_t) dlsym(handle, "get_state");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_state': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	int state = 0;
	state = get_state();

	if (state)
		sprintf(res_msg, "RUNNING");
	else
		sprintf(res_msg, "READY");

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_stop(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (handle == NULL)
		return (*env)->NewStringUTF(env, "VM not running");

	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "restart", "I");
	int restart_int = (*env)->GetIntField(env, thiz, fid);

	typedef void (*stop_vm_t)();
	dlerror();
	stop_vm_t stop_vm = (stop_vm_t) dlsym(handle, "stop_vm");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'stop_vm': %s\n", dlsym_error);
		return (*env)->NewStringUTF(env, res_msg);
	}

	stop_vm(!restart_int);

	if (restart_int)
		sprintf(res_msg, "VM Restart Request");
	else
		sprintf(res_msg, "VM Stop Request");
	LOGV(res_msg);

	started = restart_int;

	return (*env)->NewStringUTF(env, res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_start(
		JNIEnv* env, jobject thiz) {
	int res;
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (started) {
		sprintf(res_msg, "VM Already started");
		LOGV(res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}

	LOGV("***************** INIT LIMBO ************************");

	/* Read the member values from the Java Object
	 */
	jclass c = (*env)->GetObjectClass(env, thiz);
	jfieldID fid = (*env)->GetFieldID(env, c, "cpu", "Ljava/lang/String;");
	jstring jcpu = (*env)->GetObjectField(env, thiz, fid);
	const char *cpu_str = NULL;
	if (jcpu != NULL)
		cpu_str = (*env)->GetStringUTFChars(env, jcpu, 0);

	LOGV("CPU= %s", cpu_str);

	fid = (*env)->GetFieldID(env, c, "machine_type", "Ljava/lang/String;");
	jstring jmachine_type = (*env)->GetObjectField(env, thiz, fid);
	const char *machine_type_str = NULL;
	if (jmachine_type != NULL)
		machine_type_str = (*env)->GetStringUTFChars(env, jmachine_type, 0);

	LOGV("Machine Type= %s", machine_type_str);

	fid = (*env)->GetFieldID(env, c, "memory", "I");
	int mem = (*env)->GetIntField(env, thiz, fid);

	LOGV("MEM= %d", mem);

	fid = (*env)->GetFieldID(env, c, "cpuNum", "I");
	int cpuNum = (*env)->GetIntField(env, thiz, fid);

	LOGV("CPU Num= %d", cpuNum);

	fid = (*env)->GetFieldID(env, c, "disableacpi", "I");
	int disableacpi = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "disablehpet", "I");
	int disablehpet = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "usbmouse", "I");
	int usbmouse = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "enableqmp", "I");
	int enableqmp = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "enablevnc", "I");
	int enablevnc = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "enablespice", "I");
	int enablespice = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "vnc_allow_external", "I");
	int vnc_allow_external = (*env)->GetIntField(env, thiz, fid);

	LOGV("vnc_allow_external= %d", vnc_allow_external);

	fid = (*env)->GetFieldID(env, c, "disablefdbootchk", "I");
	int disablefdbootchk = (*env)->GetIntField(env, thiz, fid);

	LOGV("disablefdbootchk= %d", disablefdbootchk);

	fid = (*env)->GetFieldID(env, c, "hda_img_path", "Ljava/lang/String;");
	jstring jhda_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * hda_img_path_str = NULL;
	if (jhda_img_path != NULL)
		hda_img_path_str = (*env)->GetStringUTFChars(env, jhda_img_path, 0);
	LOGV("HDA= %s", hda_img_path_str);

	fid = (*env)->GetFieldID(env, c, "hdb_img_path", "Ljava/lang/String;");
	jstring jhdb_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * hdb_img_path_str = NULL;
	if (jhdb_img_path != NULL)
		hdb_img_path_str = (*env)->GetStringUTFChars(env, jhdb_img_path, 0);

	fid = (*env)->GetFieldID(env, c, "hdc_img_path", "Ljava/lang/String;");
	jstring jhdc_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * hdc_img_path_str = NULL;
	if (jhdc_img_path != NULL)
		hdc_img_path_str = (*env)->GetStringUTFChars(env, jhdc_img_path, 0);

	fid = (*env)->GetFieldID(env, c, "hdd_img_path", "Ljava/lang/String;");
	jstring jhdd_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * hdd_img_path_str = NULL;
	if (jhdd_img_path != NULL)
		hdd_img_path_str = (*env)->GetStringUTFChars(env, jhdd_img_path, 0);

	fid = (*env)->GetFieldID(env, c, "cd_iso_path", "Ljava/lang/String;");
	jstring jcdrom_iso_path = (*env)->GetObjectField(env, thiz, fid);
	const char * cdrom_iso_path_str = NULL;
	if (jcdrom_iso_path != NULL)
		cdrom_iso_path_str = (*env)->GetStringUTFChars(env, jcdrom_iso_path, 0);

	fid = (*env)->GetFieldID(env, c, "fda_img_path", "Ljava/lang/String;");
	jstring jfda_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * fda_img_path_str = NULL;
	if (jfda_img_path != NULL)
		fda_img_path_str = (*env)->GetStringUTFChars(env, jfda_img_path, 0);

	fid = (*env)->GetFieldID(env, c, "fdb_img_path", "Ljava/lang/String;");
	jstring jfdb_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * fdb_img_path_str = NULL;
	if (jfdb_img_path != NULL)
		fdb_img_path_str = (*env)->GetStringUTFChars(env, jfdb_img_path, 0);

	fid = (*env)->GetFieldID(env, c, "sd_img_path", "Ljava/lang/String;");
	jstring jsd_img_path = (*env)->GetObjectField(env, thiz, fid);
	const char * sd_img_path_str = NULL;
	if (jsd_img_path != NULL)
		sd_img_path_str = (*env)->GetStringUTFChars(env, jsd_img_path, 0);
	LOGV("SD= %s", sd_img_path_str);

	fid = (*env)->GetFieldID(env, c, "bootdevice", "Ljava/lang/String;");
	jstring jboot_dev = (*env)->GetObjectField(env, thiz, fid);
	const char * boot_dev_str = NULL;
	if (jboot_dev != NULL)
		boot_dev_str = (*env)->GetStringUTFChars(env, jboot_dev, 0);

	fid = (*env)->GetFieldID(env, c, "net_cfg", "Ljava/lang/String;");
	jstring jnet = (*env)->GetObjectField(env, thiz, fid);
	const char * net_str = NULL;
	if (jnet != NULL)
		net_str = (*env)->GetStringUTFChars(env, jnet, 0);

	fid = (*env)->GetFieldID(env, c, "nic_driver", "Ljava/lang/String;");
	jstring jnic_driver = (*env)->GetObjectField(env, thiz, fid);
	const char * nic_driver_str = NULL;
	if (jnic_driver != NULL)
		nic_driver_str = (*env)->GetStringUTFChars(env, jnic_driver, 0);

	fid = (*env)->GetFieldID(env, c, "libqemu", "Ljava/lang/String;");
	jstring jlib_path = (*env)->GetObjectField(env, thiz, fid);
	const char * lib_path_str = NULL;
	if (jlib_path != NULL)
		lib_path_str = (*env)->GetStringUTFChars(env, jlib_path, 0);

	fid = (*env)->GetFieldID(env, c, "vga_type", "Ljava/lang/String;");
	jstring jvga_type = (*env)->GetObjectField(env, thiz, fid);
	const char * vga_type_str = NULL;
	if (jvga_type != NULL)
		vga_type_str = (*env)->GetStringUTFChars(env, jvga_type, 0);

	fid = (*env)->GetFieldID(env, c, "hd_cache", "Ljava/lang/String;");
	jstring jhd_cache = (*env)->GetObjectField(env, thiz, fid);
	const char * hd_cache_str = NULL;
	if (jhd_cache != NULL)
		hd_cache_str = (*env)->GetStringUTFChars(env, jhd_cache, 0);

	fid = (*env)->GetFieldID(env, c, "sound_card", "Ljava/lang/String;");
	jstring jsound_card = (*env)->GetObjectField(env, thiz, fid);
	const char * sound_card_str = NULL;
	if (jsound_card != NULL)
		sound_card_str = (*env)->GetStringUTFChars(env, jsound_card, 0);

	fid = (*env)->GetFieldID(env, c, "snapshot_name", "Ljava/lang/String;");
	jstring jsnapshot_name = (*env)->GetObjectField(env, thiz, fid);
	const char * snapshot_name_str = NULL;
	if (jsnapshot_name != NULL)
		snapshot_name_str = (*env)->GetStringUTFChars(env, jsnapshot_name, 0);

	fid = (*env)->GetFieldID(env, c, "paused", "I");
	int paused = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "fd_save_state", "I");
	int fd_save_state = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "save_state_name", "Ljava/lang/String;");
	jstring jsave_state_name = (*env)->GetObjectField(env, thiz, fid);
	const char * save_state_name_str = NULL;
	if (jsave_state_name != NULL)
		save_state_name_str = (*env)->GetStringUTFChars(env, jsave_state_name, 0);

	fid = (*env)->GetFieldID(env, c, "qmp_port", "I");
	int qmp_port = (*env)->GetIntField(env, thiz, fid);

	fid = (*env)->GetFieldID(env, c, "qmp_server", "Ljava/lang/String;");
	jstring jqmp_server = (*env)->GetObjectField(env, thiz, fid);
	const char * qmp_server_str = NULL;
	if (jqmp_server != NULL)
		qmp_server_str  = (*env)->GetStringUTFChars(env, jqmp_server, 0);

	fid = (*env)->GetFieldID(env, c, "vnc_passwd", "Ljava/lang/String;");
	jstring jvnc_passwd = (*env)->GetObjectField(env, thiz, fid);
	const char * vnc_passwd_str = NULL;
	if (jvnc_passwd != NULL)
		vnc_passwd_str = (*env)->GetStringUTFChars(env, jvnc_passwd, 0);

	fid = (*env)->GetFieldID(env, c, "base_dir", "Ljava/lang/String;");
	jstring jbase_dir = (*env)->GetObjectField(env, thiz, fid);
	const char * base_dir_str = NULL;
	if (jbase_dir != NULL)
		base_dir_str = (*env)->GetStringUTFChars(env, jbase_dir, 0);

	fid = (*env)->GetFieldID(env, c, "dns_addr", "Ljava/lang/String;");
	jstring jdns_addr = (*env)->GetObjectField(env, thiz, fid);
	const char * dns_addr_str = NULL;
	if (jdns_addr != NULL)
		dns_addr_str = (*env)->GetStringUTFChars(env, jdns_addr, 0);

	fid = (*env)->GetFieldID(env, c, "kernel", "Ljava/lang/String;");
	jstring jkernel = (*env)->GetObjectField(env, thiz, fid);
	const char * kernel_str = NULL;
	if (jkernel != NULL)
		kernel_str = (*env)->GetStringUTFChars(env, jkernel, 0);

	fid = (*env)->GetFieldID(env, c, "arch", "Ljava/lang/String;");
	jstring jarch = (*env)->GetObjectField(env, thiz, fid);
	const char * arch_str = NULL;
	if (jarch != NULL)
		arch_str = (*env)->GetStringUTFChars(env, jarch, 0);

	fid = (*env)->GetFieldID(env, c, "append", "Ljava/lang/String;");
	jstring jappend = (*env)->GetObjectField(env, thiz, fid);
	const char * append_str = NULL;
	if (jappend != NULL)
		append_str = (*env)->GetStringUTFChars(env, jappend, 0);

	fid = (*env)->GetFieldID(env, c, "initrd", "Ljava/lang/String;");
	jstring jinitrd = (*env)->GetObjectField(env, thiz, fid);
	const char * initrd_str = NULL;
	if (jinitrd != NULL)
		initrd_str = (*env)->GetStringUTFChars(env, jinitrd, 0);

	LOGV("Finished getting Java fields");

	char mem_str[MAX_STRING_LEN] = "128";
	sprintf(mem_str, "%d", mem);

	char cpu_num_str[MAX_STRING_LEN] = "1";
	sprintf(cpu_num_str, "%d", cpuNum);

	int i = 0;
	LOGV("Setting Params");
	int MAX_PARAMS = 128;
	char ** argv = (char **) malloc(MAX_PARAMS * sizeof(*argv));
	for (i = 0; i < MAX_PARAMS; i++) {
		argv[i] = (char *) malloc(MAX_STRING_LEN * sizeof(char));
	}

	int param = 0;
	strcpy(argv[param++], lib_path_str);

	if (kernel_str != NULL && strcmp(kernel_str, "") != 0) {
		strcpy(argv[param++], "-kernel");
		strcpy(argv[param++], kernel_str);
	}

	if (initrd_str != NULL && strcmp(initrd_str, "") != 0) {
		strcpy(argv[param++], "-initrd");
		strcpy(argv[param++], initrd_str);
	}

	if (append_str != NULL && strcmp(append_str, "") != 0) {
		strcpy(argv[param++], "-append");
		strcpy(argv[param++], append_str);
	}

	if (cpu_str != NULL && strncmp(cpu_str, "Default", 7) != 0) {
		strcpy(argv[param++], "-cpu");
		strcpy(argv[param++], cpu_str);
	}

	strcpy(argv[param++], "-m");
	strcpy(argv[param++], mem_str);

	strcpy(argv[param++], "-L");
	strcpy(argv[param++], base_dir_str);

	if (hda_img_path_str != NULL) {
		strcpy(argv[param++], "-hda");
		strcpy(argv[param++], hda_img_path_str);
	}
	if (hdb_img_path_str != NULL) {
		strcpy(argv[param++], "-hdb");
		strcpy(argv[param++], hdb_img_path_str);
	}

	if (hdc_img_path_str != NULL) {
		strcpy(argv[param++], "-hdc");
		strcpy(argv[param++], hdc_img_path_str);
	}

	if (hdd_img_path_str != NULL) {
		strcpy(argv[param++], "-hdd");
		strcpy(argv[param++], hdd_img_path_str);
	}

	if (cdrom_iso_path_str != NULL)
		if (strcmp(cdrom_iso_path_str, "") == 0) {
			strcpy(argv[param++], "-drive"); //empty
			strcpy(argv[param++], "index=2,media=cdrom");
		} else {
			LOGV("Adding CD");
			strcpy(argv[param++], "-cdrom");
			strcpy(argv[param++], cdrom_iso_path_str);

		}

	if (fda_img_path_str != NULL)
		if (strcmp(fda_img_path_str, "") == 0) {
			strcpy(argv[param++], "-drive"); //empty
			strcpy(argv[param++], "index=0,if=floppy");
		} else {
			LOGV("Adding FDA");
			strcpy(argv[param++], "-fda");
			strcpy(argv[param++], fda_img_path_str);
		}

	if (fdb_img_path_str != NULL)
		if (strcmp(fdb_img_path_str, "") == 0) {
			strcpy(argv[param++], "-drive"); //empty
			strcpy(argv[param++], "index=1,if=floppy");
		} else {
			LOGV("Adding FDB");
			strcpy(argv[param++], "-fdb");
			strcpy(argv[param++], fdb_img_path_str);
		}

	if (sd_img_path_str != NULL) {
		if (strcmp(sd_img_path_str, "") == 0) {
			strcpy(argv[param++], "-drive"); //empty sd
			strcpy(argv[param++], "index=0,if=sd");
		} else {
			LOGV("Adding SD");
			strcpy(argv[param++], "-sd");
			strcpy(argv[param++], sd_img_path_str);
		}

	}

	if (vga_type_str != NULL) {
		LOGV("Adding vga: %s", vga_type_str);
		strcpy(argv[param++], "-vga");
		strcpy(argv[param++], vga_type_str);
	}

	if (boot_dev_str != NULL) {
		LOGV("Adding boot device: %s", boot_dev_str);
		strcpy(argv[param++], "-boot");
		strcpy(argv[param++], boot_dev_str);
	}

	if (net_str != NULL) {
		LOGV("Adding Net: %s", net_str);
		strcpy(argv[param++], "-net");
		if (strcmp(net_str, "user") == 0) {
			strcpy(argv[param], net_str);
		} else if (strcmp(net_str, "tap") == 0) {
			strcpy(argv[param], "tap,vlan=0,ifname=tap0,script=no");
		} else if (strcmp(net_str, "none") == 0) {
			strcpy(argv[param], "none");
		} else {
			LOGV("Unknown iface: %s", net_str);
			strcpy(argv[param], "none");
		}
		//FIXME: NOT WORKING setting DNS workaround below
//        LOGV("DNS=%s",dns_addr_str);
//        if(dns_addr_str!=NULL){
//        	strcat(argv[param], ",dns=");
//        	strcat(argv[param], dns_addr_str);
//        }
		param++;

	}

	if (nic_driver_str != NULL) {
		LOGV("Adding NIC: %s", nic_driver_str);
		strcpy(argv[param++], "-net");
		if (strcmp(net_str, "user") == 0) {
			strcpy(argv[param], "nic,model=");
			strcat(argv[param++], nic_driver_str);
		} else if (strcmp(net_str, "tap") == 0) {
			strcpy(argv[param], "nic,vlan=0,model=");
			strcat(argv[param++], nic_driver_str);
		}
	}

	if (sound_card_str != NULL && strcmp(sound_card_str, "None") != 0) {
		LOGV("Adding Sound: %s", sound_card_str);
		strcpy(argv[param++], "-soundhw");
		strcpy(argv[param++], sound_card_str);
	}

	if (snapshot_name_str != NULL && strcmp(snapshot_name_str, "") != 0) {
		LOGV("Adding snapshot: %s", snapshot_name_str);
		strcpy(argv[param++], "-loadvm");
		strcpy(argv[param++], snapshot_name_str);
	}

	if (paused ==1 && strcmp(save_state_name_str, "") != 0) {
		LOGV("Loading VM State: %s", save_state_name_str);

		int fd_tmp = open(save_state_name_str, O_RDWR | O_CLOEXEC);
		if (fd_tmp < 0) {
			LOGE("Error while getting fd for: %s", save_state_name_str);
		}else {
			LOGI("Got new fd %d for: %s", fd_tmp, save_state_name_str);
			fd_save_state = fd_tmp;
		}

		strcpy(argv[param++], "-incoming");

		char fd[5];
		sprintf(fd,"%d",fd_save_state);
		strcpy(argv[param], "fd:");
		strcat(argv[param++], fd);

	}

	if (usbmouse) {
		LOGV("Adding USB MOUSE");
		strcpy(argv[param++], "-usb");
		strcpy(argv[param++], "-usbdevice");
		strcpy(argv[param++], "tablet");
	}
	if (disableacpi) {
		LOGV("Disabling ACPI");
		strcpy(argv[param++], "-no-acpi"); //disable ACPI
	}
	if (disablehpet) {
		LOGV("Disabling HPET");
		strcpy(argv[param++], "-no-hpet"); //        disable HPET
	}

	if (disablefdbootchk) {
		LOGV("Disabling FD Boot Check");
		strcpy(argv[param++], "-no-fd-bootchk"); //        disable FD Boot Check
	}

	if (enableqmp) {
		char port_num_str[MAX_STRING_LEN] = "";
		sprintf(port_num_str, "%d", qmp_port);

		LOGV("Enable qmp server");
		strcpy(argv[param++], "-qmp");
		strcpy(argv[param], "tcp:");
		strcat(argv[param], qmp_server_str);
		strcat(argv[param], ":");
		strcat(argv[param], port_num_str);
		strcat(argv[param++], ",server,nowait");
	}

	//XXX: Extra options
	//    strcpy(argv[param++], "-D");
	//    strcpy(argv[param++], "/sdcard/limbo/log.txt");
	//    strcpy(argv[param++], "-win2k-hack");     //use it when installing Windows 2000 to avoid a disk full bug
	//    strcpy(argv[param++], "--trace");
	//    strcpy(argv[param++], "events=/sdcard/limbo/tmp/events");
	//    strcpy(argv[param++], "--trace");
	//    strcpy(argv[param++], "file=/sdcard/limbo/tmp/trace");
	//    strcpy(argv[param++], "-nographic"); //DO NOT USE //      disable graphical output and redirect serial I/Os to console

	if (enablevnc) {
		LOGV("Enable VNC server");
		strcpy(argv[param++], "-vnc");
		if (vnc_allow_external) {
			strcpy(argv[param++], ":1,password");
			//TODO: Allow connections from External
			//this is still not secure
			// Use with x509 auth and TLS for encryption
		} else
			strcpy(argv[param++], "localhost:1"); // Allow only connections from localhost without password
	} else if (enablespice) {
		//Not working right now
		LOGV("Enable SPICE server");
		strcpy(argv[param++], "-spice");
		strcpy(argv[param], "port=5902");

		if (vnc_allow_external && vnc_passwd_str != NULL) {
			strcat(argv[param], ",password=");
			strcat(argv[param], vnc_passwd_str);
		} else
			strcat(argv[param], ",addr=localhost"); // Allow only connections from localhost without password

		strcat(argv[param++], ",disable-ticketing");
		//strcpy(argv[param++], "-chardev");
		//strcpy(argv[param++], "spicevm");
	} else {
		LOGV("Disabling VNC server, using SDL instead");
		//SDL needs explicit keyboard layout
		strcpy(argv[param++], "-k");
		strcpy(argv[param++], "en-us");
	}

	LOGV("Setting multi core: %s", cpu_num_str);
	strcpy(argv[param++], "-smp");
	strcpy(argv[param++], cpu_num_str);

	LOGV("Setting machine type: %s", machine_type_str);
	strcpy(argv[param++], "-M");
	strcpy(argv[param++], machine_type_str);

	LOGV("Setting tb memory");
	strcpy(argv[param++], "-tb-size");
	strcpy(argv[param++], "32M"); //Don't increase it crashes

	LOGV("Setting real time");
	strcpy(argv[param++], "-realtime");
	strcpy(argv[param++], "mlock=off");

	LOGV("Setting clock");
	strcpy(argv[param++], "-rtc");
	strcpy(argv[param++], "base=localtime");

	//XXX: Usb redir not working under User mode
	//Redirect ports (SSH)
	//	strcpy(argv[param++], "-redir");
	//	strcpy(argv[param++], "5555::22");

	LOGV("Preparing args param=%d", param);
	param++;
	argv[param] = NULL;
	int argc = param - 1;

	int k = 0;
	char ** argvs = (char **) malloc(param * sizeof(*argvs));
	for (k = 0; k < argc; k++) {
		argvs[k] = argv[k];
		LOGV("param[%d]=%s", k, argvs[k]);
	}
	argvs[param] = NULL;

	// XXX: install our debug handler
	//signal(SIGSEGV, print_stack_trace);

	LOGV("***************** INIT QEMU ************************");
	started = 1;
	LOGV("Starting VM...");

	//LOAD LIB
	if (handle == NULL) {
		loadLib(lib_path_str);
	}

	if (!handle) {
		sprintf(res_msg, "Error opening lib: %s :%s", lib_path_str, dlerror());
		LOGV(res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}

	//FIXME: below option should use the QMP Client instead from java
	//Set DNS Workaround
	typedef void (*set_dns_addr_str_t)();
	dlerror();
	set_dns_addr_str_t set_dns_addr_str = (set_dns_addr_str_t) dlsym(handle,
			"set_dns_addr_str");
	const char *dlsym_error2 = dlerror();
	if (dlsym_error2) {
		LOGE("Cannot load symbol 'set_dns_addr_str': %s\n", dlsym_error2);
		return (*env)->NewStringUTF(env, res_msg);
	}
	set_dns_addr_str(dns_addr_str);

	//setup jni env in qemu
	typedef void (*set_jni_t)();
	dlerror();
	set_jni_t set_jni = (set_jni_t) dlsym(handle, "set_jni");
	const char *dlsym_error3 = dlerror();
	if (dlsym_error3) {
		LOGE("Cannot load symbol 'set_jni': %s\n", dlsym_error3);
		return (*env)->NewStringUTF(env, res_msg);
	}
	set_jni(env, thiz);

	LOGV("Loading symbol qemu_start...\n");
	typedef void (*qemu_start_t)();

	// reset errors
	dlerror();
	qemu_start_t qemu_start = (qemu_start_t) dlsym(handle, "qemu_start");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qemu_start': %s\n", dlsym_error);
		dlclose(handle);
		handle = NULL;
		return (*env)->NewStringUTF(env, res_msg);
	}

	qemu_start(argc, argvs, NULL);
	//UNLOAD LIB
	sprintf(res_msg, "Closing lib: %s", lib_path_str);
	LOGV(res_msg);
	dlclose(handle);
	handle = NULL;

	//XXX: JVM will anyway exit below
	//	(*env)->ReleaseStringUTFChars(env, jcdrom_iso_path, cdrom_iso_path_str);

	sprintf(res_msg, "VM has shutdown");
	LOGV(res_msg);

	exit(1);
}

