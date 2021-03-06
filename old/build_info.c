/* 
 *
 *  cbc: Create Build Configuration
 *  Copyright (C) 2012 - 2013  Iain M Conochie <iain-AT-thargoid.co.uk>
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
 *  build_info.c
 * 
 *  Functions to retrieve build information from the database and
 *  to create the specific build files required.
 * 
 *  Part of the cbc program
 * 
 *  (C) Iain M. Conochie 2012 - 2013
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>	/* required for IP address conversion */
#include <sys/stat.h>
#include <sys/types.h>
#include <mysql.h>
#include "cmdb.h"
#include "cmdb_cbc.h"
#include "cbc_mysql.h"
#include "build.h"

int
check_buffer_length(char *tmp, char *output);

void
fill_build_info(cbc_build_t *cbt, MYSQL_ROW br);

void
write_config_file(char *filename, char *output);

void
read_dhcp_hosts_file(cbc_build_t *cbcbt, char *from, char *content, char *new_content);

void
add_append_line(cbc_build_t *cbcbt, char *output, char *tmp);

void
add_preseed_append_line(cbc_build_t *cbt, char *tmp);

void
add_kickstart_append_line(cbc_build_t *cbt, char *tmp);

int
write_preseed_config(cbc_config_t *cmc, cbc_build_t *cbt);

int
write_kickstart_config(cbc_config_t *cmc, cbc_build_t *cbt);

int
add_kick_network_config(cbc_build_t *cbt, char *out, char *buff);

void
get_kickstart_auth_line(cbc_build_t *cbt, char *out, pre_app_config_t *aconf);

int
add_kick_ldap_config(pre_app_config_t *conf, char *output, char *tmp);

int
add_kick_nfs_config(pre_app_config_t *conf, char *output, char *tmp);

int
add_kick_var_tmp_scripts(char *output, char *tmp);

int
write_disk_config(cbc_config_t *cmc, cbc_build_t *cbt, char *out, char *buff);

int
add_build_packages(cbc_config_t *cmc, cbc_build_t *cbt, char *out, char *buff);

int
write_application_preconfig(cbc_config_t *cmc, cbc_build_t *cbt, char *out, char *buff);

int
write_regular_preheader(char *output, char *tmp);

int
write_lvm_preheader(char *output, char *tmp, char *device);

int
add_partition_to_preseed(pre_disk_part_t *part_info, char *output, char *buff, int special, int lvm);

int
add_partition_to_kickstart(pre_disk_part_t *part_info, char *output, char *buff, int lvm);

void
init_app_config(pre_app_config_t *configuration);

void
get_app_config(cbc_config_t *cmc, cbc_build_t *cbt, pre_app_config_t *configuration);

int get_server_name(cbc_comm_line_t *info, cbc_config_t *config)
{
	MYSQL cbc;
	MYSQL_RES *cbc_res;
	MYSQL_ROW cbc_row;
	my_ulonglong cbc_rows;
	char *query, server_id[40];
	const char *cbc_query;
	int retval;
	
	if (!(query = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in get_server_name");
	retval = 0;
	
	if (info->server_id != 0)
		sprintf(query, "SELECT server_id, name, uuid FROM server WHERE server_id = %ld", info->server_id);
	else if ((strncmp(info->name, "NULL", CONF_S)))
		sprintf(query, "SELECT server_id, name, uuid FROM server WHERE name = '%s'", info->name);
	else if ((strncmp(info->uuid, "NULL", CONF_S)))
		sprintf(query, "SELECT server_id, name, uuid FROM server WHERE uuid = '%s'", info->uuid);
	
	cbc_query = query;
	cbc_mysql_init(config, &cbc);
	cmdb_mysql_query(&cbc, cbc_query);
	if (!(cbc_res = mysql_store_result(&cbc))) {
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(MY_STORE_FAIL, mysql_error(&cbc));
	}
	sprintf(server_id, "%ld", info->server_id);
	if (((cbc_rows = mysql_num_rows(cbc_res)) == 0)){
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		if (strncmp(info->name, "NULL", CONF_S)) {
			retval = SERVER_NOT_FOUND;
			return retval;
		} else if (strncmp(info->uuid, "NULL", CONF_S)) {
			retval = SERVER_UUID_NOT_FOUND;
			return retval;
		} else if (info->server_id > 0) {
			retval = SERVER_ID_NOT_FOUND;
			return retval;
		} else {
			retval = NO_NAME_UUID_ID;
			return retval;
		}
	} else if (cbc_rows > 1) {
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		retval = MULTIPLE_SERVERS;
		return retval;
	}
	cbc_row = mysql_fetch_row(cbc_res);
	info->server_id = strtoul(cbc_row[0], NULL, 10);
	strncpy(info->name, cbc_row[1], CONF_S);
	strncpy(info->uuid, cbc_row[2], CONF_S);
	
	mysql_free_result(cbc_res);
	mysql_close(&cbc);
	mysql_library_end();
	free(query);
	return retval;
}

int get_build_info(cbc_config_t *config, cbc_build_t *build_info, unsigned long int server_id)
{
	MYSQL build;
	MYSQL_RES *build_res;
	MYSQL_ROW build_row;
	my_ulonglong build_rows;
	int retval;
	char *query;
	char sserver_id[40];
	const char *build_query;
	
	retval = NONE;
	sprintf(sserver_id, "%ld", server_id);
	if (!(query = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in get_build_info");
	build_info->server_id = server_id;
	
	sprintf(query,
"SELECT arch, bo.alias, os_version, INET_NTOA(ip), mac_addr,\
 INET_NTOA(netmask), INET_NTOA(gateway), INET_NTOA(ns), hostname, domainname,\
 boot_line, valias,ver_alias, build_type, arg, url, l.country, locale, \
 l.language, l.keymap, net_inst_int, mirror, device, lvm, bd.bd_id, \
 config_ntp, l.timezone, bo.os_id, ntp_server FROM build_ip bi LEFT JOIN (build_domain bd, \
 build_os bo, build b, build_type bt, server s, boot_line bootl, varient v, \
 locale l, disk_dev dd) ON (bi.bd_id = bd.bd_id  AND bt.bt_id = bootl.bt_id \
 AND dd.server_id = s.server_id AND b.ip_id = bi.ip_id AND bo.os_id = b.os_id \
 AND s.server_id = b.server_id AND bootl.boot_id = bo.boot_id AND \
 b.varient_id = v.varient_id AND l.os_id = b.os_id AND l.locale_id = \
 b.locale_id) WHERE s.server_id = %ld", server_id);
	build_query = query;
	cbc_mysql_init(config, &build);
	cmdb_mysql_query(&build, build_query);
	if (!(build_res = mysql_store_result(&build))) {
		mysql_close(&build);
		mysql_library_end();
		free(query);
		report_error(MY_STORE_FAIL, mysql_error(&build));
	}
	if (((build_rows = mysql_num_rows(build_res)) == 0)){
		mysql_close(&build);
		mysql_library_end();
		free(query);
		report_error(SERVER_BUILD_NOT_FOUND, sserver_id);
	} else if (build_rows > 1) {
		mysql_free_result(build_res);
		mysql_close(&build);
		mysql_library_end();
		free(query);
		report_error(MULTIPLE_SERVER_BUILDS, sserver_id);
	}
	build_row = mysql_fetch_row(build_res);
	fill_build_info(build_info, build_row);
	mysql_free_result(build_res);
	mysql_close(&build);
	mysql_library_end();
	free(query);
	return retval;
}

void write_tftp_config(cbc_config_t *cct, cbc_build_t *cbt)
{
	uint32_t ip_addr;
	size_t len;
	char ip_address[16];
	char hex_ip[10];
	char filename[TBUFF_S];
	char *file_content;
	char *tmp;
	
	if (!(file_content = calloc(FILE_S, sizeof(char))))
		report_error(MALLOC_FAIL, "file_content in write_tftp_config");
	if (!(tmp = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "tmp in write_tftp_config");

	sprintf(ip_address, "%s", cbt->ip_address);
	inet_pton(AF_INET, ip_address, &ip_addr);
	ip_addr = htonl(ip_addr);
	sprintf(hex_ip, "%X", ip_addr);
	sprintf(filename, "%s%s%s", cct->tftpdir, cct->pxe, hex_ip);
	sprintf(tmp, "default %s\n\nlabel %s\nkernel vmlinuz-%s-%s-%s\n",
		cbt->hostname,
		cbt->hostname,
		cbt->alias,
		cbt->version,
		cbt->arch);
	len = strlen(tmp);
	strncpy(file_content, tmp, len);
	if ((strncmp("none", cbt->boot, CONF_S)) != 0) {
		add_append_line(cbt, file_content, tmp);
	} else {
		sprintf(tmp, "append initrd=initrd-%s-%s-%s.img\n",
		cbt->alias,
		cbt->version,
		cbt->arch);
		len = strlen(tmp);
		strncat(file_content, tmp, len);
	}
	write_config_file(filename, file_content);
	printf("TFTP file written\n");
	free(file_content);
	free(tmp);
}

void add_append_line(cbc_build_t *cbt, char *output, char *tmp)
{
	size_t len;
	sprintf(tmp, "append initrd=initrd-%s-%s-%s.img ",
		cbt->alias,
		cbt->version,
		cbt->arch);
	len = strlen(tmp);
	strncat(output, tmp, len);
	if (strncmp(cbt->build_type, "preseed", RANGE_S) == 0)
		add_preseed_append_line(cbt, tmp);
	else if (strncmp(cbt->build_type, "kickstart", RANGE_S) == 0)
		add_kickstart_append_line(cbt, tmp);
	len = strlen(tmp);
	strncat(output, tmp, len);
}

void add_preseed_append_line(cbc_build_t *cbt, char *tmp)
{
	if ((strncmp(cbt->alias, "ubuntu", CONF_S)) ==0) {
		snprintf(tmp, BUFF_S,
"country=%s console-setup/layoutcode=%s %s %s=%s%s.cfg\n",
		 cbt->country,
		 cbt->country,
		 cbt->boot,
		 cbt->arg,
		 cbt->url,
		 cbt->hostname);
	} else {
		sprintf(tmp, "locale=%s keymap=%s %s %s=%s%s.cfg\n",
		 cbt->locale,
		 cbt->keymap,
		 cbt->boot,
		 cbt->arg,
		 cbt->url,
		 cbt->hostname);
	}
}

void add_kickstart_append_line(cbc_build_t *cbt, char *tmp)
{
	snprintf(tmp, RBUFF_S,
"%s %s=%s%s.cfg\n", cbt->boot, cbt->arg, cbt->url, cbt->hostname);
}

void fill_build_info(cbc_build_t *cbt, MYSQL_ROW br)
{
	sprintf(cbt->arch, "%s", br[0]);
	sprintf(cbt->alias, "%s", br[1]);
	sprintf(cbt->version, "%s", br[2]);
	sprintf(cbt->ip_address, "%s", br[3]);
	sprintf(cbt->mac_address, "%s", br[4]);
	sprintf(cbt->netmask, "%s", br[5]);
	sprintf(cbt->gateway, "%s", br[6]);
	sprintf(cbt->nameserver, "%s", br[7]);
	sprintf(cbt->hostname, "%s", br[8]);
	sprintf(cbt->domain, "%s", br[9]);
	sprintf(cbt->boot, "%s", br[10]);
	sprintf(cbt->varient, "%s", br[11]);
	sprintf(cbt->ver_alias, "%s", br[12]);
	sprintf(cbt->build_type, "%s", br[13]);
	sprintf(cbt->arg, "%s", br[14]);
	sprintf(cbt->url, "%s", br[15]);
	sprintf(cbt->country, "%s", br[16]);
	sprintf(cbt->locale, "%s", br[17]);
	sprintf(cbt->language, "%s", br[18]);
	sprintf(cbt->keymap, "%s", br[19]);
	sprintf(cbt->netdev, "%s", br[20]);
	sprintf(cbt->mirror, "%s", br[21]);
	sprintf(cbt->diskdev, "%s", br[22]);
	cbt->use_lvm = ((strncmp(br[23], "0", CH_S)));
	cbt->bd_id = strtoul(br[24], NULL, 10);
	cbt->config_ntp = ((strncmp(br[25], "0", CH_S)));
	sprintf(cbt->timezone, "%s", br[26]);
	cbt->os_id = strtoul(br[27], NULL, 10);
	if (br[28]) {
		sprintf(cbt->ntpserver, "%s", br[28]);
	}
}

void write_dhcp_config(cbc_config_t *cct, cbc_build_t *cbt)
{
	FILE *restart;
	size_t len;
	char *dhcp_content;
	char *dhcp_new_content;
	char buff[RBUFF_S];
	long int dhcp_size;
	struct stat dhcp_stat;
	
	stat(cct->dhcpconf, &dhcp_stat);

	dhcp_size = dhcp_stat.st_size;
	
	if (!(dhcp_content = calloc((size_t)dhcp_size + 1, sizeof(char))))
		report_error(MALLOC_FAIL, "dhcp_content in write_dhcp_config");
	if (!(dhcp_new_content = calloc((size_t)dhcp_size + TBUFF_S + 1, sizeof(char))))
		report_error(MALLOC_FAIL, "dhcp_new_content in write_dhcp_config");
	
	read_dhcp_hosts_file(cbt, cct->dhcpconf, dhcp_content, dhcp_new_content);
	
	sprintf(buff, "host %s { hardware ethernet %s; fixed-address %s; option domain-name \"%s\"; }\n",
		cbt->hostname,
		cbt->mac_address,
		cbt->ip_address,
		cbt->domain);
	len = strlen(buff);
	strncat (dhcp_new_content, buff, len);
	write_config_file(cct->dhcpconf, dhcp_new_content);
	
	snprintf(buff, RBUFF_S, "%s/restart", cct->tmpdir); 
	printf("DHCP file written\n");
	if (stat(cct->tmpdir, &dhcp_stat) == 0) {
		if (!(restart = fopen(buff, "w"))) {
			free(cct);
			free(cbt);
			free(dhcp_content);
			free(dhcp_new_content);
			report_error(FILE_O_FAIL, buff);
		} else {
			fputs("restart", restart);
			fclose(restart);
		}
	} else {
		snprintf(buff, RBUFF_S, "%s", cct->tmpdir);
		if (mkdir(buff, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != 0) {
			free(cct);
			free(cbt);
			free(dhcp_content);
			free(dhcp_new_content);
			report_error(DIR_C_FAIL, buff);
		} else {
			snprintf(buff, RBUFF_S, "%s/restart", cct->tmpdir);
			if (!(restart = fopen(buff, "w"))) {
				free(cct);
				free(cbt);
				free(dhcp_content);
				free(dhcp_new_content);
				report_error(FILE_O_FAIL, buff);
			} else {
				fclose(restart);
			}
		}
	}
	free(dhcp_content);
	free(dhcp_new_content);
}

void write_config_file(char *filename, char *output)
{
	FILE *configfile;
	if (!(configfile = fopen(filename, "w"))) {
		report_error(FILE_O_FAIL, filename);
	} else {
		fputs(output, configfile);
		fclose(configfile);
	}
}

void read_dhcp_hosts_file(cbc_build_t *cbcbt, char *from, char *content, char *new_content)
{
	FILE *dhcp;
	size_t len, conlen;
	time_t now;
	struct tm *lctime;
	int success;
	char time_string[18];
	char file_to[CONF_S];
	char buff[TBUFF_S];
	
	now = time(0);
	lctime = localtime(&now);
	sprintf(time_string, "%d%d%d%d%d%d",
		lctime->tm_year + 1900,
		lctime->tm_mon + 1,
		lctime->tm_mday,
		lctime->tm_hour,
		lctime->tm_min,
		lctime->tm_sec);
	
	sprintf(file_to, "%s-%s", from, time_string);
	if (!(dhcp = fopen(from, "r")))
		report_error(FILE_O_FAIL, from);
	while (fgets(buff, TBUFF_S, dhcp)) {
		len = strlen(buff) + 1;
		if ((conlen = strlen(content) == 0))
			strncpy(content, buff, len);
		else
			strncat(content, buff, len);
		if (!(strstr(buff, cbcbt->hostname))) {
			if (!(strstr(buff, cbcbt->ip_address))) {
				if (!(strstr(buff, cbcbt->mac_address))) {
					if ((conlen = strlen(new_content) == 0))
						strncpy(new_content, buff, len);
					else
						strncat(new_content, buff, len);
				}
			}
		}
	}
	fclose(dhcp);
	success = rename(from, file_to);
	if (success < 0)
		printf("Error backing up old config! Check permissions!\n");
}

int write_build_config(cbc_config_t *cmc, cbc_build_t *cbt)
{
	int retval;
	retval = 0;
	if ((strncmp(cbt->build_type, "preseed", RANGE_S) == 0))
		retval = write_preseed_config(cmc, cbt);
	else if ((strncmp(cbt->build_type, "kickstart", RANGE_S) == 0))
		retval = write_kickstart_config(cmc, cbt);
	else
		return retval;
	return retval;
}

int delete_build_if_exists(cbc_config_t *cmc, cbc_build_t *cbt)
{
	MYSQL build;
	my_ulonglong build_rows;
	int retval;
	char *query;
	const char *build_query;
	
	retval = 0;
	if (!(query = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in get_build_info");
	snprintf(query, RBUFF_S,
"DELETE FROM disk_dev WHERE server_id = %lu", cbt->server_id);
	build_query = query;
	cbc_mysql_init(cmc, &build);
	cmdb_mysql_query(&build, build_query);
	snprintf(query, RBUFF_S,
"DELETE FROM build_ip WHERE hostname = '%s' AND domainname = '%s'",
		cbt->hostname,
		cbt->domain);
	cmdb_mysql_query(&build, build_query);
	snprintf(query, RBUFF_S,
"DELETE FROM build WHERE server_id = %lu", cbt->server_id);
	cmdb_mysql_query(&build, build_query);
	snprintf(query, RBUFF_S,
"DELETE FROM seed_part WHERE server_id = %lu", cbt->server_id);
	cmdb_mysql_query(&build, build_query);
	build_rows = mysql_affected_rows(&build);
	if (build_rows > 0)
		printf("Deleted previous build from database\n");
	cmdb_mysql_clean(&build, query);
	return retval;
}

int write_preseed_config(cbc_config_t *cmc, cbc_build_t *cbt)
{
	FILE *precnf;
	size_t len, total;
	int retval;
	char *output, *tmp, *preseed;
	
	len = total = 0;
	retval = 0;
	
	if (!(output = calloc(BUILD_S, sizeof(char))))
		report_error(MALLOC_FAIL, "output in write_preseed_config");
	if (!(tmp = calloc(FILE_S, sizeof(char))))
		report_error(MALLOC_FAIL, "tmp in write_preseed_config");
	if (!(preseed = calloc(HOST_S, sizeof(char))))
		report_error(MALLOC_FAIL, "preseed in write_preseed_config");
	snprintf(preseed, HOST_S, "%s%s/%s.cfg",
		 cmc->toplevelos,
		 cbt->alias,
		 cbt->hostname);
	snprintf(tmp, FILE_S,
	"d-i console-setup/ask_detect boolean false\n\
d-i debian-installer/locale string %s\n\
d-i debian-installer/language string %s\n\
d-i console-keymaps-at/keymap select %s\n\
d-i keymap select %s\n",
		cbt->locale,
		cbt->language,
		cbt->keymap,
		cbt->keymap);
	len = strlen(tmp);
	if (len + total < BUILD_S) {
		strncat(output, tmp, FILE_S);
		total = strlen(output);
	} else {
		free(output);
		free(tmp);
		retval = BUFFER_FULL;
		return retval;
	}
	
	snprintf(tmp, FILE_S,
	"d-i preseed/early_command string /bin/killall.sh; /bin/netcfg\n\
d-i netcfg/enable boolean true\n\
d-i netcfg/confirm_static boolean true\n\
d-i netcfg/disable_dhcp boolean true\n\
d-i netcfg/choose_interface select %s\n\
d-i netcfg/get_nameservers string %s\n\
d-i netcfg/get_ipaddress string %s\n\
d-i netcfg/get_netmask string %s\n\
d-i netcfg/get_gateway string %s\n\
d-i netcfg/get_hostname string %s\n\
d-i netcfg/get_domain string %s\n",
		cbt->netdev,
		cbt->nameserver,
		cbt->ip_address,
		cbt->netmask,
		cbt->gateway,
		cbt->hostname,
		cbt->domain);
	len = strlen(tmp);
	if (len + total < BUILD_S) {
		strncat(output, tmp, FILE_S);
		total = strlen(output);
	} else {
		free(output);
		free(tmp);
		retval = BUFFER_FULL;
		return retval;
	}
	
	snprintf(tmp, FILE_S,
	"d-i netcfg/wireless_wep string\n\
d-i hw-detect/load_firmware boolean true\n\
d-i mirror/country string manual\n\
d-i mirror/http/hostname string %s\n\
d-i mirror/http/directory string /%s\n\
d-i mirror/suite string %s\n\
d-i passwd/root-password-crypted password $1$d/0w8MHb$tdqENqvXIz53kZp2svuak1\n\
d-i passwd/user-fullname string Monkey User\n\
d-i passwd/username string monkey\n\
d-i passwd/user-password-crypted password $1$Hir6Ul13$.T1tAO.yfK5g7WDKSw0nI/\n\
d-i clock-setup/utc boolean true\n\
d-i time/zone string %s\n",
		cbt->mirror,
		cbt->alias,
		cbt->ver_alias,
		cbt->country);
	len = strlen(tmp);
	if (len + total < BUILD_S) {
		strncat(output, tmp, FILE_S);
		total = strlen(output);
	} else {
		free(output);
		free(tmp);
		retval = BUFFER_FULL;
		return retval;
	}
	if (cbt->config_ntp == 0)
		snprintf(tmp, HOST_S, "d-i clock-setup/ntp boolean false");
	else
		snprintf(tmp, RBUFF_S, "d-i clock-setup/ntp boolean \
true\nd-i clock-setup/ntp-server string %s\n",
			cbt->ntpserver);
	len = strlen(tmp);
	if (len + total < BUILD_S) {
		strncat(output, tmp, FILE_S);
		total = strlen(output);
	} else {
		free(output);
		free(tmp);
		retval = BUFFER_FULL;
		return retval;
	}
	snprintf(tmp, FILE_S, "d-i partman-auto/disk string %s\n",
		 cbt->diskdev);
	len = strlen(tmp);
	if (len + total < BUILD_S) {
		strncat(output, tmp, FILE_S);
		total = strlen(output);
	} else {
		free(output);
		free(tmp);
		retval = BUFFER_FULL;
		return retval;
	}
	retval = write_disk_config(cmc, cbt, output, tmp);
	if (retval == BUFFER_FULL) {
		free(tmp);
		free(output);
		return retval;
	}
	/* Hard coded arch. Maybe setup DB entry?? */
	if (strncmp(cbt->arch, "i386", COMM_S) == 0)
		snprintf(tmp, HOST_S, "d-i base-installer/kernel/image string linux-image-2.6-686\n");
	else if (strncmp(cbt->arch, "x86_64", COMM_S) ==0)
		snprintf(tmp, HOST_S, "d-i base-installer/kernel/image string linux-image-2.6-amd64\n");
	len = strlen(tmp);
	total = strlen(output);
	if ((total + len) > BUILD_S) {
		free(tmp);
		free(output);
		return BUFFER_FULL;
	} else {
		strncat(output, tmp, len);
	}
	snprintf(tmp, FILE_S,
"d-i apt-setup/non-free boolean true\n\
d-i apt-setup/contrib boolean true\n\
d-i apt-setup/services-select multiselect security, volatile\n\
d-i apt-setup/security_host string security.%s.org\n\
d-i apt-setup/volatile_host string volatile.%s.org\n\
tasksel tasksel/first multiselect standard, web-server\n\
d-i pkgsel/include string ", cbt->alias, cbt->alias);
	retval = add_build_packages(cmc, cbt, output, tmp);
	if (retval == BUFFER_FULL) {
		free(tmp);
		free(output);
		return retval;
	}
	snprintf(tmp, FILE_S,
"d-i pkgsel/upgrade select none\n\
popularity-contest popularity-contest/participate boolean false\n\
d-i finish-install/keep-consoles boolean true\n\
d-i finish-install/reboot_in_progress note\n\
d-i cdrom-detect/eject boolean false\n");
	len = strlen(tmp);
	total = strlen(output);
	if ((total + len) > BUILD_S) {
		free(tmp);
		free(output);
		return BUFFER_FULL;
	} else {
		strncat(output, tmp, len);
	}
	snprintf(tmp, FILE_S,
"d-i preseed/late_command string cd /target/root; wget %sscripts/base.sh \
&& chmod 755 base.sh && ./base.sh\n\n", cbt->url);
	len = strlen(tmp);
	total = strlen(output);
	if ((total + len) > BUILD_S) {
		free(tmp);
		free(output);
		return BUFFER_FULL;
	} else {
		strncat(output, tmp, len);
	}
	retval = write_application_preconfig(cmc, cbt, output, tmp);
	if (retval == BUFFER_FULL) {
		free(tmp);
		free(output);
		return retval;
	}

	printf("Preseed file: %s\n", preseed);
	if (!(precnf = fopen(preseed, "w"))) {
		fprintf(stderr, "Cannot open config file %s for writing\n", preseed);
                return FILE_O_FAIL;
	} else {
		fputs(output, precnf);
		fclose(precnf);
	}
	free(preseed);
	free(tmp);
	free(output);
	return retval;
}

int write_disk_config(
			cbc_config_t *cmc,
			cbc_build_t *cbt,
			char *output,
			char *tmp)
{
	MYSQL cbc;
	MYSQL_RES *cbc_res;
	MYSQL_ROW cbc_row;
	my_ulonglong cbc_rows;
	pre_disk_part_t *node, *saved;
	pre_disk_part_t *head_part = 0;
	char *query;
	char sserver_id[HOST_S];
	const char *cbc_query;
	int retval, parti;
	size_t len;
	retval = parti = 0;
	if ((strncmp(cbt->build_type, "preseed", RANGE_S)) == 0) {
		if (cbt->use_lvm) {
			retval = write_lvm_preheader(output, tmp, cbt->diskdev);
		} else {
			retval = write_regular_preheader(output, tmp);
		}
		if (retval > 0) {
			mysql_library_end();
			return retval;
		}
	}
	snprintf(sserver_id, HOST_S - 1, "%ld", cbt->server_id);
	
	if (!(query = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in write_reg_disk_preconfig");
	snprintf(query, BUFF_S - 1,
"SELECT minimum, maximum, priority, mount_point, filesystem, part_id, \
logical_volume FROM seed_part WHERE server_id = %ld ORDER BY mount_point\n",
cbt->server_id);
	cbc_query = query;
	cbc_mysql_init(cmc, &cbc);
	cmdb_mysql_query(&cbc, cbc_query);
	if (!(cbc_res = mysql_store_result(&cbc))) {
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(MY_STORE_FAIL, mysql_error(&cbc));
	}
	if (((cbc_rows = mysql_num_rows(cbc_res)) == 0)){
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(SERVER_PART_NOT_FOUND, sserver_id);
	}
	while ((cbc_row = mysql_fetch_row(cbc_res))){
		head_part = part_node_add(head_part, cbc_row);
	}
	mysql_free_result(cbc_res);
	mysql_close(&cbc);
	mysql_library_end();
	node = head_part;
	if ((strncmp(cbt->build_type, "preseed", RANGE_S)) == 0) {
		while ((node)) {
			parti = check_for_special_partition(node);
			retval = add_partition_to_preseed(
				node,
				output,
				tmp,
				parti,
				cbt->use_lvm);
			if (retval == BUFFER_FULL)
				return BUFFER_FULL;
			node = node->nextpart;
		}
	} else if ((strncmp(cbt->build_type, "kickstart", RANGE_S)) == 0) {
		add_partition_to_kickstart(node, output, tmp, cbt->use_lvm);
	}
	len = strlen(output);
	len--;
	*(output + len) = '\0';
	strncat(output, "\n\n", 3);
	for (node = head_part; node; ){
		saved = node->nextpart;
		free(node);
		node = saved;
	}
	free(node);
	free(query);
	return retval;
}
int write_regular_preheader(char *output, char *tmp)
{
	size_t len, full_len;
	snprintf(tmp, FILE_S,
"d-i partman-auto/method string regular\n\
d-i partman-auto/purge_lvm_from_device boolean true\n\
d-i partman-auto-lvm/guided_size    string 100%%\n\
d-i partman-lvm/device_remove_lvm boolean true\n\
d-i partman-lvm/device_remove_lvm_span boolean true\n\
d-i partman-lvm/confirm boolean true\n\
d-i partman-auto/choose_recipe select monkey\n\
d-i partman-md/device_remove_md boolean true\n\
d-i partman-partitioning/confirm_write_new_label boolean true\n\
d-i partman/confirm_nooverwrite boolean true\n\
d-i partman-lvm/confirm_nooverwrite boolean true\n\
d-i partman/choose_partition select Finish partitioning and write changes to disk\n\
d-i partman/confirm boolean true\n\
\n\
d-i partman-auto/expert_recipe string                         \\\n\
      monkey ::                                               \\");
	full_len = strlen(output);
	len = strlen(tmp);
	if ((len + full_len) < BUILD_S)
		strncat(output, tmp, len);
	else
		return BUFFER_FULL;
	
	return 0;
}

int write_lvm_preheader(char *output, char *tmp, char *device)
{
	size_t len, full_len;
	snprintf(tmp, FILE_S, 
"d-i partman-auto/method string lvm\n\
d-i partman-auto/purge_lvm_from_device boolean true\n\
d-i partman-auto-lvm/guided_size    string 100%%\n\
d-i partman-lvm/device_remove_lvm boolean true\n\
d-i partman-lvm/device_remove_lvm_span boolean true\n\
d-i partman-lvm/confirm boolean true\n\
d-i partman-auto/choose_recipe select monkey\n\
d-i partman-md/device_remove_md boolean true\n\
d-i partman-partitioning/confirm_write_new_label boolean true\n\
d-i partman/confirm_nooverwrite boolean true\n\
d-i partman-lvm/confirm_nooverwrite boolean true\n\
d-i partman/choose_partition select Finish partitioning and write changes to disk\n\
d-i partman/confirm boolean true\n\
\n\
d-i partman-auto/expert_recipe string                         \\\n\
      monkey ::                                               \\\n\
              100 1000 1000000000 ext3                        \\\n\
                       $defaultignore{ }                      \\\n\
                       $primary{ }                            \\\n\
                       method{ lvm }                          \\\n\
                       device{ %s }                     \\\n\
                       vg_name{ systemlv }                    \\\n\
      . \\", device);
	full_len = strlen(output);
	len = strlen(tmp);
	if ((len + full_len) < BUILD_S)
		strncat(output, tmp, len);
	else
		return BUFFER_FULL;
	
	return 0;
}

int check_for_special_partition(pre_disk_part_t *part)
{
	int retval;
	retval = NONE;
	if (strncmp(part->mount_point, "/boot", COMM_S) == 0)
		retval = BOOT;
	else if (strncmp(part->mount_point, "/", COMM_S) ==0)
		retval = ROOT;
	else if (strncmp(part->mount_point, "swap", COMM_S) == 0)
		retval = SWAP;
	
	return retval;
}

int
add_partition_to_preseed(pre_disk_part_t *part, char *output, char *buff, int special, int lvm)
{
	int retval;
	size_t len, total_len;
	len = total_len = 0;
	retval = NONE;
	
	switch (special) {
		case ROOT: case NONE:
			if (lvm > 0) {
			snprintf(buff, FILE_S - 1,
"\n\t%ld %ld %ld %s\t\t\t\\\n\
\t$lvmok\t\t\t\t\\\n\
\tin_vg{ systemlv }\t\t\t\\\n\
\tlv_name{ %s }\t\t\t\\\n\
\tmethod{ format } format{ }\t\t\t\\\n\
\tuse_filesystem{ } filesystem{ %s }\t\t\\\n\
\tmountpoint{ %s }\t\t\t\\\n\t. \\",
	part->min,
	part->pri,
	part->max,
	part->filesystem,
	part->log_vol,
	part->filesystem,
	part->mount_point);
			} else {
			snprintf(buff, FILE_S - 1,
"\n\t%ld %ld %ld %s\t\t\t\\\n\
\tmethod{ format } format{ }\t\t\t\\\n\
\tuse_filesystem{ } filesystem{ %s }\t\t\\\n\
\tmountpoint{ %s }\t\t\t\\\n\t. \\",
	part->min,
	part->pri,
	part->max,
	part->filesystem,
	part->filesystem,
	part->mount_point);
			}
			break;
		case BOOT:
			snprintf(buff, FILE_S - 1,
"\n\t%ld %ld %ld %s\t\t\t\\\n\
\t$primary{ } $bootable{ }\t\t\t\\\n\
\tmethod{ format } format{ }\t\t\t\\\n\
\tuse_filesystem{ } filesystem{ %s }\t\t\\\n\
\tmountpoint{ %s }\t\t\t\t\\\n\t. \\",
	part->min,
	part->pri,
	part->max,
	part->filesystem,
	part->filesystem,
	part->mount_point);
			break;
		case SWAP:
			if (lvm > 0) {
			snprintf(buff, FILE_S - 1,
"\n\t%ld %ld %ld%% linux-swap\t\t\t\\\n\
\t$lvmok\t\t\t\t\\\n\
\tin_vg{ systemlv }\t\t\t\\\n\
\tlv_name{ swap }\t\t\t\\\n\
\tmethod{ swap } format{ }\t\t\t\\\n\t. \\",
	part->min,
	part->pri,
	part->max);
			} else {
			snprintf(buff, FILE_S - 1,
"\n\t%ld %ld %ld%% linux-swap\t\t\t\t\\\n\
\tmethod{ swap } format{ }\t\t\t\\\n\t. \\",
	part->min,
	part->pri,
	part->max);
			}
			break;
		default:
			retval = 1;
			return retval;
			break;
	}
	
	len = strlen(buff);
	total_len = strlen(output);
	if ((len + total_len) > BUILD_S) {
		retval = 1;
	} else {
		strncat(output, buff, len);
		retval = 0;
	}

	return retval;
}

int add_partition_to_kickstart(pre_disk_part_t *part_info, char *output, char *buff, int lvm)
{
	int retval;
	pre_disk_part_t *node;
	
	node = part_info;
	if (lvm > 0) {
		snprintf(buff, RBUFF_S,
"clearpart --all\n");
		if ((retval = check_buffer_length(buff, output)) > 0)
			return retval;
		while (node) {
			if ((strncmp(node->mount_point, "/boot", HOST_S)) == 0) {
				break;
			} else {
				node = node->nextpart;
			}
		}
		if (!node)
			return NO_BOOT_PARTITION;
		else
			snprintf(buff, RBUFF_S,
"part %s --fstype \"%s\" --size %lu\n", node->mount_point, node->filesystem, node->min);
		if ((retval = check_buffer_length(buff, output)) > 0)
			return retval;
		snprintf(buff, RBUFF_S,
"part pv.2 --size=1 --grow\nvolgroup vg00 --pesize=32768 pv.2\n");
		if ((retval = check_buffer_length(buff, output)) > 0)
			return retval;
		node = part_info;
		while (node) {
			if ((strncmp(node->mount_point, "/boot", HOST_S)) == 0) {
				node = node->nextpart;
				continue;
			} else if ((strncmp(node->mount_point, "swap", HOST_S)) == 0) {
				snprintf(buff, RBUFF_S,
"logvol %s --name=%s --vgname=vg00 --size %lu\n", node->mount_point, node->log_vol, node->min);
				if ((retval = check_buffer_length(buff, output)) > 0) {
					return retval;
				}
				node = node->nextpart;
			} else {
				snprintf(buff, RBUFF_S,
"logvol %s --fstype %s --name=%s --vgname=vg00 --size=%lu\n",
				 node->mount_point,
				 node->filesystem,
				 node->log_vol,
				 node->min);
				if ((retval = check_buffer_length(buff, output)) > 0) {
					return retval;
				}
				node = node->nextpart;
			}
		}
	}
	return 0;
}

int add_build_packages(cbc_config_t *cmc, cbc_build_t *cbt, char *out, char *buff)
{
	int retval;
	char *query, *package;
	const char *cbc_query;
	size_t len, total, full, row_len;
	MYSQL cbc;
	MYSQL_RES *cbc_res;
	MYSQL_ROW cbc_row;
	my_ulonglong cbc_rows;
	
	retval = 0;
	len = strlen(buff);
	full = strlen(out);
	if(!(query = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in add_build_packages");
	if (!(package = calloc(CONF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "package in add_build_packages");
	snprintf(query, BUFF_S - 1,
"select package from packages p, varient v, build_os bo WHERE bo.os_id =\
p.os_id AND alias = '%s' AND os_version = '%s' AND valias = '%s'\
AND arch = '%s' AND v.varient_id = p.varient_id",
cbt->alias, cbt->version, cbt->varient, cbt->arch);
	cbc_query = query;
	cbc_mysql_init(cmc, &cbc);
	cmdb_mysql_query(&cbc, cbc_query);
	if (!(cbc_res = mysql_store_result(&cbc))) {
		mysql_close(&cbc);
		mysql_library_end();
		free(package);
		free(query);
		report_error(MY_STORE_FAIL, mysql_error(&cbc));
	}
	if (((cbc_rows = mysql_num_rows(cbc_res)) == 0)){
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(package);
		free(query);
		report_error(SERVER_PACKAGES_NOT_FOUND, cbt->hostname);
	}
	while ((cbc_row = mysql_fetch_row(cbc_res))){
		row_len = strlen(cbc_row[0]);
		if ((strncmp(cbt->build_type, "preseed", RANGE_S)) == 0)
			snprintf(package, row_len + 2, "%s ", cbc_row[0]);
		else if ((strncmp(cbt->build_type, "kickstart", RANGE_S)) == 0)
			snprintf(package, row_len + 2, "%s\n", cbc_row[0]);
		row_len = strlen(package);
		total = len + 1 + row_len;
		if (total < FILE_S) {
			strncat(buff, package, row_len);
			len = strlen(buff);
		} else {
			retval = BUFFER_FULL;
			mysql_free_result(cbc_res);
			mysql_close(&cbc);
			mysql_library_end();
			free(package);
			free(query);
			return retval;
		}
	}
	snprintf(package, COMM_S, "\n");
	if ((len + 2) < FILE_S)
		strncat(buff, package, CH_S);
	len = strlen(buff);
	if ((len + 1 + full) < BUILD_S)
		strncat(out, buff, len); 
	
	mysql_free_result(cbc_res);
	mysql_close(&cbc);
	mysql_library_end();
	free(package);
	free(query);
	return retval;
}

int write_application_preconfig(cbc_config_t *cmc, cbc_build_t *cbt, char *out, char *buff)
{
	size_t buff_len, out_len;
	pre_app_config_t *app_conf;
	int retval;
	retval = 0;
	
	out_len = strlen(out);
	if (!(app_conf = malloc(sizeof(pre_app_config_t))))
		report_error(MALLOC_FAIL, "app_config in write_application_preconfig");
	
	init_app_config(app_conf);
	get_app_config(cmc, cbt, app_conf);
	
	if (out_len + MAC_S < BUILD_S)
		strncat(out, "\n\n#Application Configuration\n\n", MAC_S);
	else
		return BUFFER_FULL;
	
	out_len = strlen(out);
	if (app_conf->config_email > 0) {
		snprintf(buff, FILE_S,
"postfix postfix/mailname  string  %s.%s\n\
postfix postfix/main_mailer_type  select  Internet with smarthost\n\
postfix postfix/destinations string  %s.%s, localhost.%s, localhost\n\
postfix postfix/relayhost    string  %s\n\n",
			 cbt->hostname,
			 cbt->domain,
			 cbt->hostname,
			 cbt->domain,
			 cbt->domain,
			 app_conf->smtp_server);
		buff_len = strlen(buff);
		if (out_len + buff_len < BUILD_S) {
			strncat(out, buff, buff_len);
		} else {
			return BUFFER_FULL;
		}
		out_len = strlen(out);
	}

	if (app_conf->config_xymon > 0) {
		snprintf(buff, FILE_S,
"xymon-client    hobbit-client/HOBBITSERVERS     string  %s\n\
xymon-client    hobbit-client/CLIENTHOSTNAME    string  %s.%s\n\n",
			 app_conf->xymon_server,
			 cbt->hostname,
			 cbt->domain);
		buff_len = strlen(buff);
		if (out_len + buff_len < BUILD_S) {
			strncat(out, buff, buff_len);
		} else {
			return BUFFER_FULL;
		}
		out_len = strlen(out);
	}

	if (app_conf->config_ldap > 0) {
		snprintf(buff, FILE_S,
"libnss-ldap     libnss-ldap/bindpw      password\n\
libnss-ldap     libnss-ldap/rootbindpw  password\n\
libnss-ldap     libnss-ldap/dblogin     boolean false\n\
libnss-ldap     libnss-ldap/override    boolean true\n\
libnss-ldap     shared/ldapns/base-dn   string  %s\n\
libnss-ldap     shared/ldapns/ldap-server       string %s\n\
libnss-ldap     libnss-ldap/binddn      string  %s\n\
libnss-ldap     libnss-ldap/rootbinddn  string  %s\n\
libnss-ldap     shared/ldapns/ldap_version      select  3\n\
libnss-ldap     libnss-ldap/nsswitch    note\n\
libnss-ldap     libnss-ldap/confperm    boolean false\n\
libnss-ldap     libnss-ldap/dbrootlogin boolean true\n\
\n\
libpam-ldap     libpam-ldap/rootbindpw  password\n\
libpam-ldap     libpam-ldap/bindpw      password\n\
libpam-ldap     shared/ldapns/base-dn   string  %s\n\
libpam-ldap     shared/ldapns/ldap-server       string  %s\n\
libpam-ldap     libpam-ldap/pam_password        select  crypt\n\
libpam-ldap     libpam-ldap/binddn      string  %s\n\
libpam-ldap     libpam-ldap/rootbinddn  string  %s\n\
libpam-ldap     libpam-ldap/dblogin     boolean false\n\
libpam-ldap     libpam-ldap/override    boolean true\n\
libpam-ldap     shared/ldapns/ldap_version      select  3\n\
libpam-ldap     libpam-ldap/dbrootlogin boolean true \n\
libpam-runtime  libpam-runtime/profiles multiselect     unix, winbind, ldap\n\
",
			 app_conf->ldap_dn,
			 app_conf->ldap_url,
			 app_conf->ldap_bind,
			 app_conf->ldap_bind,
			 app_conf->ldap_dn,
			 app_conf->ldap_url,
			 app_conf->ldap_bind,
			 app_conf->ldap_bind);
		buff_len = strlen(buff);
		if (out_len + buff_len < BUILD_S) {
			strncat(out, buff, buff_len);
		} else {
			return BUFFER_FULL;
		}
		out_len = strlen(out);
		if ((strncmp(cbt->alias, "ubuntu", CONF_S)) == 0) {
			snprintf(buff, FILE_S, "\
\n\
ldap-auth-config        ldap-auth-config/bindpw password\n\
ldap-auth-config        ldap-auth-config/rootbindpw     password\n\
ldap-auth-config        ldap-auth-config/binddn string  cn=%s\n\
ldap-auth-config        ldap-auth-config/dbrootlogin    boolean true\n\
ldap-auth-config        ldap-auth-config/rootbinddn     string  cn=%s\n\
ldap-auth-config        ldap-auth-config/pam_password   select  md5\n\
ldap-auth-config        ldap-auth-config/move-to-debconf        boolean true\n\
ldap-auth-config        ldap-auth-config/ldapns/ldap-server     string  %s\n\
ldap-auth-config        ldap-auth-config/ldapns/base-dn string  %s\n\
ldap-auth-config        ldap-auth-config/override       boolean true\n\
ldap-auth-config        ldap-auth-config/ldapns/ldap_version    select  3\n\
ldap-auth-config        ldap-auth-config/dblogin        boolean false\n",
			 app_conf->ldap_bind,
			 app_conf->ldap_bind,
			 app_conf->ldap_url,
			 app_conf->ldap_dn);
			buff_len = strlen(buff);
			if (out_len + buff_len < BUILD_S) {
				strncat(out, buff, buff_len);
			} else {
				return BUFFER_FULL;
			}
			out_len = strlen(out);
		}
	}
	free(app_conf);
	return retval;
}

void init_app_config(pre_app_config_t *config)
{
	snprintf(config->ldap_url, COMM_S, "NULL");
	snprintf(config->ldap_dn, COMM_S, "NULL");
	snprintf(config->ldap_bind, COMM_S, "NULL");
	snprintf(config->log_server, COMM_S, "NULL");
	snprintf(config->smtp_server, COMM_S, "NULL");
	snprintf(config->xymon_server, COMM_S, "NULL");
	config->config_ldap = 0;
	config->config_log = 0;
	config->config_email = 0;
	config->config_xymon = 0;
}

void get_app_config(cbc_config_t *cmc, cbc_build_t *cbt, pre_app_config_t *configuration)
{
	char *query, sserver_id[10];
	const char *cbc_query;
	unsigned long int ldapssl;
	MYSQL cbc;
	MYSQL_RES *cbc_res;
	MYSQL_ROW cbc_row;
	my_ulonglong cbc_rows;
	if (!(query = calloc(RBUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "query in get_app_config");
	snprintf(query, RBUFF_S,
"SELECT ldap_url, ldap_ssl, ldap_dn, ldap_bind, config_ldap, log_server, \
config_log, smtp_server, config_email, xymon_server, config_xymon, nfs_domain \
FROM build_domain WHERE bd_id = %ld", cbt->bd_id);
	snprintf(sserver_id, 10, "%ld", cbt->server_id);
	cbc_query = query;
	cbc_mysql_init(cmc, &cbc);
	cmdb_mysql_query(&cbc, cbc_query);
	if (!(cbc_res = mysql_store_result(&cbc))) {
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(MY_STORE_FAIL, mysql_error(&cbc));
	}
	if (((cbc_rows = mysql_num_rows(cbc_res)) == NONE)){
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(SERVER_ID_NOT_FOUND, sserver_id);
	}
	if (cbc_rows > 1){
		mysql_free_result(cbc_res);
		mysql_close(&cbc);
		mysql_library_end();
		free(query);
		report_error(MULTIPLE_SERVER_IDS, sserver_id);
	}
	cbc_row = mysql_fetch_row(cbc_res);
	configuration->config_ldap = strtoul(cbc_row[4], NULL, 10);
	configuration->config_log = strtoul(cbc_row[6], NULL, 10);
	configuration->config_email = strtoul(cbc_row[8], NULL, 10);
	configuration->config_xymon = strtoul(cbc_row[10], NULL, 10);
	ldapssl = strtoul(cbc_row[1], NULL, 10);
	if (configuration->config_ldap > NONE) {
		if (ldapssl > NONE) {
			snprintf(configuration->ldap_url, URL_S, "ldaps://%s",
			 cbc_row[0]);
			configuration->ldap_ssl = 1;
		} else {
			snprintf(configuration->ldap_url, URL_S, "ldap://%s",
			 cbc_row[0]);
			configuration->ldap_ssl = 0;
		}
		snprintf(configuration->ldap_host, URL_S, "%s", cbc_row[0]);
		snprintf(configuration->ldap_dn, URL_S, "%s", cbc_row[2]);
		snprintf(configuration->ldap_bind, URL_S, "%s", cbc_row[3]);
	}
	if (configuration->config_log > NONE)
		snprintf(configuration->log_server, CONF_S, "%s", cbc_row[5]);
	if (configuration->config_email > NONE)
		snprintf(configuration->smtp_server, CONF_S, "%s", cbc_row[7]);
	if (configuration->config_xymon > NONE)
		snprintf(configuration->xymon_server, CONF_S, "%s", cbc_row[9]);
	snprintf(configuration->nfs_domain, CONF_S, "%s", cbc_row[11]);
	free(query);
	mysql_free_result(cbc_res);
	mysql_close(&cbc);
	mysql_library_end();
}

int write_kickstart_config(cbc_config_t *cmc, cbc_build_t *cbt)
{
	FILE *kick;
	int retval;
	char *output, *tmp, *url, *newurl;
	pre_app_config_t *aconf;

	retval = 0;
	if (!(aconf = malloc(sizeof(pre_app_config_t))))
		report_error(MALLOC_FAIL, "aconf in write_kickstart_config");
	if (!(output = calloc(BUILD_S, sizeof(char))))
		report_error(MALLOC_FAIL, "output in write_kickstart_config");
	if (!(tmp = calloc(FILE_S, sizeof(char))))
		report_error(MALLOC_FAIL, "tmp in write_kickstart_config");
	if (!(url = calloc(RBUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "url in write_kickstart_config");
	get_app_config(cmc, cbt, aconf);
	get_kickstart_auth_line(cbt, tmp, aconf);
	snprintf(output, RBUFF_S, "%s", tmp);
	snprintf(tmp, RBUFF_S,
"bootloader --location=mbr\n\
text\n\
firewall --disabled\n\
firstboot --disable\n");
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	snprintf(tmp, RBUFF_S,
"keyboard %s\n\
lang %s\n\
timezone  %s\n", cbt->keymap, cbt->locale, cbt->timezone);
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	snprintf(tmp, RBUFF_S,
"logging --level=info\nreboot\n\
rootpw --iscrypted $6$OILHHW/y$vDpY5YosWhQnI/XO3wipIrrcAAag9tHPqPh31i.6r0hkauX2LVNYIzwWl/YvFqtVUYR7XWyep3spzeT.Q5Be0/\n\
selinux --disabled\n\
skipx\n\
install\n\n");
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	if ((retval = write_disk_config(cmc, cbt, output, tmp)) != 0)
		return retval;
	if ((retval = add_kick_network_config(cbt, output, tmp)) > 0)
		return retval;
	snprintf(tmp, CONF_S,
"%%packages\n@Base\n\n");
	if ((retval = add_build_packages(cmc, cbt, output, tmp)) != 0)
		return retval;
	snprintf(tmp, FILE_S,
"%%post\n\ncd /tmp\nwget %sscripts/mounts.sh\n\
wget %sscripts/motd.sh\n\
wget %sscripts/ntp.sh\n\
wget %sscripts/postfix-config.sh\nfor i in *.sh; do chmod 755 $i; done\n\n\
cd /var/tmp\n\n\
wget %sscripts/config.sh\n\
wget %sscripts/hobbit-client-install.sh\n\
wget %sscripts/xymon.sh\nfor i in *.sh; do chmod 755 $i; done\n\n", cbt->url, cbt->url, cbt->url, cbt->url, cbt->url,
			cbt->url, cbt->url);
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	snprintf(url, RBUFF_S, "%s", cbt->url);
	newurl = strrchr(url, '/');
	*newurl = '\0';
	newurl = strrchr(url, '/');
	newurl++;
	*newurl = '\0';
	snprintf(tmp, FILE_S,
"cd /tmp\n\nwget %sdisable_install.php  > /root/disable.log\n\n\
./config.sh > /root/config.sh.log 2>&1\n\
./mounts.sh > /root/mounts.sh.log 2>&1\n\
./motd.sh > /root/motd.sh.log 2>&1\n\
./ntp.sh > /root/ntp.log 2>&1\n\
/sbin/chkconfig sendmail off\n\
/sbin/chkconfig postfix on\n\
./postfix-config.sh -h %s -d %s -i %s > /root/postfix.log 2>&1\n\n",
	  url, cbt->hostname, cbt->domain, cbt->ip_address);
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	if (aconf->config_ldap > 0)
		if ((retval = add_kick_ldap_config(aconf, output, tmp)) > 0)
			return retval;
	free(url);
	if ((retval = add_kick_nfs_config(aconf, output, tmp)) > 0)
		return retval;
	if ((retval = add_kick_var_tmp_scripts(output, tmp)) > 0)
		return retval;
	snprintf(tmp, RBUFF_S, "%s%s/%s.cfg", cmc->toplevelos, cbt->alias, cbt->hostname);
	if (!(kick = fopen(tmp, "w"))) {
		free(aconf);
		free(output);
		free(tmp);
		return FILE_O_FAIL;
	} else {
		fputs(output, kick);
		fclose(kick);
	}
	return retval;
}
int add_kick_network_config(cbc_build_t *cbt, char *out, char *buff)
{
	int retval;
	
	retval = 0;
	snprintf(buff, FILE_S,
"url --url=http://%s/%s/%s/%s\n\
network --bootproto=static --device=%s --ip %s --netmask %s --gateway %s --nameserver %s --hostname=%s.%s --onboot=on\n\n",	cbt->mirror, cbt->alias, cbt->arch, cbt->version,
		cbt->netdev, cbt->ip_address, cbt->netmask, cbt->gateway, cbt->nameserver,
		cbt->hostname, cbt->domain);
	if ((retval = check_buffer_length(buff, out)) > 0)
		retval = BUFFER_FULL;
	return retval;
}

void get_kickstart_auth_line(cbc_build_t *cbt, char *out, pre_app_config_t *aconf)
{
	if (cbt->use_lvm == 0) {
		snprintf(out, RBUFF_S,
"auth --useshadow --enablemd5\n");
	} else if (cbt->use_lvm > 0) {
		if (aconf->ldap_ssl > 0) {
			snprintf(out, RBUFF_S,
"auth --useshadow --enablemd5 --enableldap --enableldapauth  --enableldaptls \
--ldapserver=%s --ldapbasedn=%s\n", aconf->ldap_host, aconf->ldap_dn);
		} else {
			snprintf(out, RBUFF_S,
"auth --useshadow --enablemd5 --enableldap --enableldapauth \
--ldapserver=%s --ldapbasedn=%s\n", aconf->ldap_host, aconf->ldap_dn);
		}
	}
}

int add_kick_ldap_config(pre_app_config_t *conf, char *output, char *tmp)
{
	char *url;
	int retval;
	retval = 0;
	url = strchr(conf->ldap_url, '/');
	url++;
	url++;
	if (conf->ldap_ssl > 0) {
		snprintf(tmp, FILE_S,
			 /* Hard coded certificate location */
"wget http://www.shihad.org/Buka-Root-CA.pem\n\
cp Buka-Root-CA.pem /etc/pki/tls/certs\n\
cat >> /etc/openldap/ldap.conf<<EOF\n\
TLS_CACERTFILE  /etc/pki/tls/certs/Buka-Root-CA.pem\n\
EOF\n\
cat >> /etc/ldap.conf <<EOF\n\
tls_cacertfile /etc/pki/tls/certs/Buka-Root-CA.pem\n\
EOF\n\
yum install nss-pam-ldapd pam_ldap -y\n\
/sbin/chkconfig --levels 2345 nscd on\n\
authconfig --update --enableldap --enableldapauth --enableldaptls --ldapserver=%s --ldapbasedn=%s\n\n",
		 url,
		 conf->ldap_dn);
	} else if (conf->ldap_ssl == 0) {
		snprintf(tmp, FILE_S,
"yum install nss-pam-ldapd pam_ldap -y\n\
/sbin/chkconfig --levels 2345 nscd on\n\
authconfig --update --enableldap --enableldapauth --ldapserver=%s --ldapbasedn=%s\n\n",
		 url,
		 conf->ldap_dn);
	}
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	return retval;
}

int add_kick_nfs_config(pre_app_config_t *conf, char *output, char *tmp)
{
	char *url;
	int retval;
	
	url = strchr(conf->ldap_url, '/');
	url++;
	url++;
	retval = 0;
	snprintf(tmp, FILE_S,
"if [ -f /etc/idmapd.conf ]; then\n\
    cp /etc/idmapd.conf /etc/idmapd.bak\n\
fi\n\
\n\
cat > /etc/idmapd.conf <<EOF\n\
[General]\n\
Domain = %s\n\
\n\
[Mapping]\n\
\n\
[Translation]\n\
\n\
Method = nsswitch\n\
\n\
[Static]\n\
\n\
[UMICH_SCHEMA]\n\
\n\
LDAP_server = %s\n\
\n\
LDAP_base = %s\n\
EOF\n", conf->nfs_domain, url, conf->ldap_dn);
	if ((retval = check_buffer_length(tmp, output)) > 0)
		return BUFFER_FULL;
	return retval;
}

int add_kick_var_tmp_scripts(char *output, char *tmp)
{
	int retval;
	
	retval = 0;
	snprintf(tmp, FILE_S,
"OUTPUT=/etc/rc.d/rc.local\n\n\
echo \"if [ -x /var/tmp/config.sh ]; then\" >> $OUTPUT\n\
echo \"  /var/tmp/config.sh > /root/config.log 2>&1\" >> $OUTPUT\n\
echo \"  chmod 644 /var/tmp/config.sh\" >> $OUTPUT\n\
echo \"fi\" >> $OUTPUT\n\
echo \" \" >> $OUTPUT\n\
echo \"if [ -x /var/tmp/hobbit-client-install.sh ]; then\" >> $OUTPUT\n\
echo \"  HOSTNAME=\\`/bin/hostname\\`\" >> $OUTPUT\n\
echo \"  /var/tmp/hobbit-client-install.sh -h \\$HOSTNAME > /root/hobb-cli-inst.log 2>&1\" >> $OUTPUT\n\
echo \"  chmod 644 /var/tmp/hobbit-client-install.sh\" >> $OUTPUT\n\
echo \"fi\" >> $OUTPUT\n\
echo \" \" >> $OUTPUT\n\
echo \"if [ -x /var/tmp/xymon.sh ]; then\" >> $OUTPUT\n\
echo \"  /var/tmp/xymon.sh > /root/xymon.log 2>&1\" >> $OUTPUT\n\
echo \"  chmod 644 /var/tmp/xymon.sh\" >> $OUTPUT\n\
echo \"fi\" >> $OUTPUT\n");
	if ((retval = check_buffer_length(tmp, output)) > 0)
		retval = BUFFER_FULL;
	return retval;
}


int check_buffer_length(char *tmp, char *output)
{
	size_t tmp_len, out_len;
		tmp_len = strlen(tmp);
	out_len = strlen(output);
	if ((tmp_len + out_len) < BUILD_S) {
		strncat(output, tmp, tmp_len);
	} else {
		return BUFFER_FULL;
	}
	return 0;
}