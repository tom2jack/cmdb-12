/* cmdb_cbc.h */

#ifndef __CMDB_CBC_H__
#define __CMDB_CBC_H__


typedef struct cbc_comm_line_t {	/* Hold parsed command line args */
	unsigned long int server_id;
	short int action;
	short int usedb;
	short int build_type;
	char config[CONF_S];
	char name[CONF_S];
	char uuid[CONF_S];
} cbc_comm_line_t;

typedef struct cbc_config_t {		/* Hold CMDB configuration values */
	char db[CONF_S];
	char user[CONF_S];
	char pass[CONF_S];
	char host[CONF_S];
	char socket[CONF_S];
	unsigned int port;
	unsigned long int cliflag;
	char tmpdir[CONF_S];
	char tftpdir[CONF_S];
	char pxe[CONF_S];
	char toplevelos[CONF_S];
	char dhcpconf[CONF_S];
	char kickstart[CONF_S];
	char preseed[CONF_S];
} cbc_config_t;

typedef struct cbc_build_t {		/* Hold build configuration values */
	char ip_address[RANGE_S];
	char mac_address[MAC_S];
	char hostname[CONF_S];
	char domain[RBUFF_S];
	char alias[CONF_S];
	char varient[CONF_S];
	char arch[CONF_S];
	char boot[RBUFF_S];
	char gateway[RANGE_S];
	char nameserver[RANGE_S];
	char netmask[RANGE_S];
} cbc_build_t;

int
parse_cbc_config_file(cbc_config_t *dc, char *config);

void
init_cbc_config_values(cbc_config_t *dc);

void
init_cbc_comm_values(cbc_comm_line_t *cbt);

void
init_cbc_build_values(cbc_build_t *build_config);

void
parse_cbc_config_error(int error);

void
print_cbc_config(cbc_config_t *cbc);

void
print_cbc_build_values(cbc_build_t *build_config);

void
print_cbc_command_line_values(cbc_comm_line_t *command_line);

int
parse_cbc_command_line(int argc, char *argv[], cbc_comm_line_t *cb, cbc_build_t *cbt);

#endif