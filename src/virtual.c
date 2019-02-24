/* 
 *
 *  mkvm: Make Virtual Machine
 *  Copyright (C) 2018 - 2019  Iain M Conochie <iain-AT-thargoid.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  virtual.c
 *
 *  Contains functions to manipulate Virtual machines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ailsacmdb.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

// Storage data structures and functions

typedef struct ailsa_virt_stor_s {
	virStoragePoolPtr pool;
	virStorageVolPtr vol;
} ailsa_virt_stor_s;


// End of Storage

static int
ailsa_connect_libvirt(virConnectPtr *conn, const char *uri);

static int
ailsa_create_volume_xml(ailsa_mkvm_s *vm);

static int
ailsa_create_domain_xml(ailsa_mkvm_s *vm, ailsa_string_s *dom);

static int
ailsa_get_vol(virStorageVolPtr vol, ailsa_mkvm_s *vm);

static int
ailsa_get_vol_type(virStorageVolInfoPtr info, ailsa_mkvm_s *vm);

#ifndef DEBUG
static void
ailsa_custom_libvirt_err(void *data, virErrorPtr err)
{
	if (!(data) || !(err))
		return;
}
#endif

int
mkvm_create_vm(ailsa_mkvm_s *vm)
{
	int retval = 0;
	virConnectPtr conn;
	virStoragePoolPtr pool = NULL;
	virStorageVolPtr vol = NULL;
	virNetworkPtr net = NULL;
	virDomainPtr dom = NULL;
	ailsa_string_s *domain = NULL;

#ifndef DEBUG
	virSetErrorFunc(NULL, ailsa_custom_libvirt_err);
#endif
	domain = ailsa_calloc(sizeof(ailsa_string_s), "domain in mkvm_create_mv");
	ailsa_init_string(domain);
	if ((retval = ailsa_connect_libvirt(&conn, (const char *)vm->uri)) != 0)
		return retval;
	if ((dom = virDomainLookupByName(conn, (const char *)vm->name))) {
// If the domain is _inactive_ it will not be returned here.
		fprintf(stderr, "Domain %s already exists!\n", vm->name);
		goto cleanup;
	}
	if (!(pool = virStoragePoolLookupByName(conn, vm->pool))) {
		fprintf(stderr, "Cannot find pool %s\n", vm->pool);
		retval = -1;
		goto cleanup;
	}
	if (!(vol = virStorageVolLookupByName(pool, vm->name))) {
		if ((retval = ailsa_create_volume_xml(vm)) != 0) {
			fprintf(stderr, "Unable to create XML to define storage\n");
			retval = -1;
			goto cleanup;
		}
		if (!(vol = virStorageVolCreateXML(pool, (const char *)vm->storxml, 0))) {
			fprintf(stderr, "Unable to create storage volume %s\n", vm->name);
			retval = -1;
			goto cleanup;
		}
	}
	if ((retval = ailsa_get_vol(vol, vm)) != 0) {
		fprintf(stderr, "Unable to get vol info for %s\n", vm->name);
		goto cleanup;
	}
	if (!(net = virNetworkLookupByName(conn, vm->network))) {
		fprintf(stderr, "Network %s not found\n", vm->network);
	}
	if ((retval = ailsa_create_domain_xml(vm, domain)) != 0) {
		fprintf(stderr, "Unable to create XML document for domain\n");
		goto cleanup;
	}
	
	cleanup:
		if (pool)
			virStoragePoolFree(pool);
		if (vol)
			virStorageVolFree(vol);
		if (net)
			virNetworkFree(net);
		if (dom)
			virDomainFree(dom);
		if (domain)
			ailsa_clean_string(domain);
		virConnectClose(conn);
		return retval;
}

static int
ailsa_connect_libvirt(virConnectPtr *conn, const char *uri)
{
	int retval = 0;

	if (!(conn))
		return AILSA_NO_DATA;
	if (!(uri))
		uri = "qemu:///system";
	if (!(*conn = virConnectOpen(uri)))
		return AILSA_NO_CONNECT;
	return retval;
}

static int
ailsa_create_volume_xml(ailsa_mkvm_s *vm)
{
	int retval = 0;
	unsigned long int capacity = 0;

	if (!(vm))
		return AILSA_NO_DATA;
	vm->storxml = ailsa_calloc(FILE_LEN, "vm->xmlstor in ailsa_create_volume_xml");
	capacity = vm->size * 1024 * 1024 * 1024;
	sprintf(vm->storxml, "\
<volume>\n\
  <name>%s</name>\n\
  <capacity unit='bytes'>%lu</capacity>\n\
  <allocation unit='bytes'>%lu</allocation>\n\
</volume>\n", vm->name, capacity, capacity);
	return retval;
}

static int
ailsa_create_domain_xml(ailsa_mkvm_s *vm, ailsa_string_s *dom)
{
	int retval = 0;
	char *uuid = NULL;
	char buf[FILE_LEN];
	unsigned long int ram = 0;
	virStorageVolPtr vol = NULL;

	if (!(vm))
		return -1;
	ram = vm->ram * 1024;
	uuid = ailsa_gen_uuid_str();
	snprintf(buf, FILE_LEN, "\
<domain type='kvm'>\n\
  <name>%s</name>\n\
  <uuid>%s</uuid>\n\
  <memory unit='KiB'>%lu</memory>\n\
  <currentMemory unit='KiB'>%lu</currentMemory>\n\
  <vcpu placement='static'>%lu</vcpu>\n\
  <os>\n\
    <type arch='x86_64' machine='pc-i440fx-2.8'>hvm</type>\n\
  </os>\n\
  <features>\n\
    <acpi/>\n\
    <apic/>\n\
    <pae/>\n\
  </features>\n\
  <cpu mode='custom' match='exact'>\n\
    <model fallback='allow'>Opteron_G3</model>\n\
  </cpu>\n\
  <clock offset='utc'>\n\
    <timer name='rtc' tickpolicy='catchup'/>\n\
    <timer name='pit' tickpolicy='delay'/>\n\
    <timer name='hpet' present='no'/>\n\
  </clock>\n\
  <on_poweroff>destroy</on_poweroff>\n\
  <on_reboot>restart</on_reboot>\n\
  <on_crash>restart</on_crash>\n\
  <pm>\n\
    <suspend-to-mem enabled='no'/>\n\
    <suspend-to-disk enabled='no'/>\n\
  </pm>\n\
", vm->name, uuid, ram, ram, vm->cpus);
	ailsa_fill_string(dom, buf);
	memset(buf, 0, FILE_LEN);
	if ((retval = ailsa_get_vol(vol, vm)) != 0)
		goto cleanup;
	snprintf(buf, FILE_LEN, "\
  <devices>\n\
    <emulator>/usr/bin/kvm</emulator>\n\
    <disk type='%s' device='disk'>\n\
      <driver name='qemu' type='raw'/>\n\
      <source %s='%s'/>\n\
      <target dev='vda' bus='virtio'/>\n\
      <address type='pci' domain='0x0000' bus='0x00' slot='0x07' function='0x0'/>\n\
    </disk>\n\
", vm->vt, vm->vtstr, vm->path);

	cleanup:
		if (uuid)
			my_free(uuid);
	return retval;
}

static int
ailsa_get_vol(virStorageVolPtr uvol, ailsa_mkvm_s *vm)
{
	int retval = 0;
	if (!(uvol) || !(vm))
		return AILSA_NO_DATA;
	virStorageVolPtr vol = uvol;
	virStorageVolInfoPtr vol_info = NULL;

	if ((retval = virStorageVolGetInfo(vol, vol_info)) != 0) {
		fprintf(stderr, "Unable to get vol info for %s\n", vm->name);
		goto cleanup;
	}
	if ((retval = ailsa_get_vol_type(vol_info, vm)) != 0) {
		fprintf(stderr, "Unable to get the volume type for volume %s\n", vm->name);
		retval = -1;
		goto cleanup;
	}
	if (!(vm->path = virStorageVolGetPath(vol))) {
		fprintf(stderr, "Unable to get the volume path for volume %s\n", vm->name);
		retval = -1;
		goto cleanup;
	}
	cleanup:
		return retval;
}

static int
ailsa_get_vol_type(virStorageVolInfoPtr info, ailsa_mkvm_s *vm)
{
	int retval = 0;
	char *str = ailsa_calloc(MAC_LEN, "str in ailsa_get_vol_type");
	char *vt = ailsa_calloc(MAC_LEN, "vt in ailsa_get_vol_type");

	if (info->type == VIR_STORAGE_VOL_FILE) {
		snprintf(str, MAC_LEN, "file");
		snprintf(vt, MAC_LEN, "file");
	} else if (info->type == VIR_STORAGE_VOL_BLOCK) {
		snprintf(str, MAC_LEN, "block");
		snprintf(vt, MAC_LEN, "dev");
	} else {
		my_free(str);
		my_free(vt);
		fprintf(stderr, "Unknown vol type\n");
		retval = -1;
	}
	vm->vt = vt;
	vm->vtstr = str;
	return retval;
}

