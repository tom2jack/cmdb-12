/* 
 *
 *  cmdb: Configuration Management Database
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
 *  cmdb.c
 *
 *  Contains main() function for cmdb program
 *
 *  Command line arguments:
 *
 *  -s: Choose a server
 *  -c: Choose a customer
 *  -t: Choose a contact
 *  -d: Display details
 *  -l: List <customers|contacts|servers>
 *  -n <name>: Name of customer / contact / server
 *  -i <id>: UUID's of servers OR COID of customer OR CONID of contact
 */

#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdb.h"
#include "cmdb_cmdb.h"
#ifdef HAVE_LIBPCRE
# include "checks.h"
#endif /*HAVE_LIBPCRE */

int main(int argc, char *argv[])
{
	cmdb_comm_line_t *cm;
	cmdb_config_t *cmc;
	char *cmdb_config;
	int retval;

	if (!(cmc = malloc(sizeof(cmdb_config_t))))
		report_error(MALLOC_FAIL, "cmc in cmdb.c");
	if (!(cm = malloc(sizeof(cmdb_comm_line_t))))
		report_error(MALLOC_FAIL, "cm in cmdb.c");
	if (!(cmdb_config = malloc(CONF_S * sizeof(char))))
		report_error(MALLOC_FAIL, "cmdb_config in cmdb.c");
	
	init_cmdb_comm_line_values(cm);
	init_cmdb_config_values(cmc);
	retval = parse_cmdb_command_line(argc, argv, cm);
	if (retval < 0) {
		cmdb_main_free(cm, cmc, cmdb_config);
		display_cmdb_command_line_error(retval, argv[0]);
	}
	sprintf(cmdb_config, "%s", cm->config);
	retval = parse_cmdb_config_file(cmc, cmdb_config);
	/* Commented out block was here */
	switch (cm->type) {
		case SERVER:
#ifdef HAVE_LIBPCRE
			if ((strncmp(cm->id, "NULL", CONF_S) != 0)) {
				retval = validate_user_input(cm->id, UUID_REGEX);
				if (retval < 0)
					retval = validate_user_input(cm->id, NAME_REGEX);
			} else if ((strncmp(cm->name, "NULL", CONF_S) != 0)) {
				retval = validate_user_input(cm->name, NAME_REGEX);
			} else {
				printf("Both name and uuid set to NULL??\n");
				exit (NO_NAME_UUID_ID);
			}
			if (retval < 0) {
				printf("User input not valid\n");
				exit (USER_INPUT_INVALID);
			}
#endif /* HAVE_LIBPCRE */
			if (cm->action == DISPLAY) {
				display_server_info(cm->name, cm->id, cmc);
			} else if (cm->action == LIST_OBJ) {
				display_all_servers(cmc);
			} else if (cm->action == ADD_TO_DB) {
				retval = add_server_to_database(cmc, cm);
				if (retval > 0) {
					printf("Error adding to database\n");
					return 1;
				} else {
					printf("Added into database\n");
				}
			} else {
				display_action_error(cm->action);
			}
			break;
		case CUSTOMER:
			if ((strncmp(cm->name, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->id, COID_REGEX);
			} else if ((strncmp(cm->id, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->name, CUSTOMER_REGEX);
			} else {
				printf("Both name and coid set to NULL??\n");
				return 1;
			}
			if (retval < 0) {
				printf("User input not valid\n");
				return 1;
			}
			if (cm->action == DISPLAY) {
				display_customer_info(cm->name, cm->id, cmc);
			} else if (cm->action == LIST_OBJ) {
				display_all_customers(cmc);
			} else {
				display_action_error(cm->action);
			}
			break;
		default:
			display_type_error(cm->type);
			break;
	}
	free(cmc);
	free(cm);
	free(cmdb_config);
	exit (0);
}
/*
	if ((strncmp(cmc->dbtype, "none", RANGE_S) ==0))
		fprintf(stderr, "No Database Driver specified\n");
#ifdef HAVE_MYSQL
	else if ((strncmp(cmc->dbtype, "mysql", RANGE_S) == 0))
		retval = cmdb_use_mysql(cmc, cm, retval);
#endif
#ifdef HAVE_SQLITE3
	else if ((strncmp(cmc->dbtype, "sqlite", RANGE_S) == 0))
		retval = cmdb_use_sqlite(cmc, cm);
#endif
	else
		fprintf(stderr, "Unknown Database Driver %s!\n", cmc->dbtype);

	cmdb_main_free(cm, cmc, cmdb_config);
	exit (retval);
}

#ifdef HAVE_MYSQL
int cmdb_use_mysql(cmdb_config_t *cmc, cmdb_comm_line_t *cm, int retval)
{	
	if (retval == -2) {
		printf("Port value higher that 65535!\n");
		return 1;
	}
	switch (cm->type) {
		case SERVER:
			if ((strncmp(cm->name, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->id, UUID_REGEX);
				if (retval < 0)
					retval = validate_user_input(cm->id, NAME_REGEX);
			} else if ((strncmp(cm->id, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->name, NAME_REGEX);
			} else {
				printf("Both name and uuid set to NULL??\n");
				return 1;
			}
			if (retval < 0) {
				printf("User input not valid\n");
				return 1;
			}
			if (cm->action == DISPLAY) {
				display_server_info(cm->name, cm->id, cmc);
			} else if (cm->action == LIST_OBJ) {
				display_all_mysql_servers(cmc);
			} else if (cm->action == ADD_TO_DB) {
				retval = add_server_to_database(cmc);
				if (retval > 0) {
					printf("Error adding to database\n");
					return 1;
				} else {
					printf("Added into database\n");
				}
			}
			break;
		case CUSTOMER:
			if ((strncmp(cm->name, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->id, COID_REGEX);
			} else if ((strncmp(cm->id, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->name, CUSTOMER_REGEX);
			} else {
				printf("Both name and coid set to NULL??\n");
				return 1;
			}
			if (retval < 0) {
				printf("User input not valid\n");
				return 1;
			}
			if (cm->action == DISPLAY)
				display_customer_info(cm->name, cm->id, cmc);
			else if (cm->action == LIST_OBJ)
				display_all_customers(cmc);
			break;
		default:
			printf("Not implemented yet :(\n");
			break;
	}
	return 0;
}
#endif
#ifdef HAVE_SQLITE3
int cmdb_use_sqlite(cmdb_config_t *cmc, cmdb_comm_line_t *cm)
{
	int retval;
	
	retval = 0;
	printf("Using sqlite database\n");
	switch(cm->type) {
		case SERVER:
			if ((strncmp(cm->name, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->id, UUID_REGEX);
				if (retval < 0)
					retval = validate_user_input(cm->id, NAME_REGEX);
			} else if ((strncmp(cm->id, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->name, NAME_REGEX);
			} else {
				printf("Both name and uuid set to NULL??\n");
				return 1;
			}
			if (retval < 0) {
				printf("User input not valid\n");
				return 1;
			}
			if (cm->action == DISPLAY) {
				display_server_info(cm->name, cm->id, cmc);
			} else if (cm->action == LIST_OBJ) {
				display_all_sqlite_servers(cmc);
			} else if (cm->action == ADD_TO_DB) {
				retval = add_server_to_database(cmc);
				if (retval > 0) {
					printf("Error adding to database\n");
					return 1;
				} else {
					printf("Added into database\n");
				}
			}
			break;
		case CUSTOMER:
			if ((strncmp(cm->name, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->id, COID_REGEX);
			} else if ((strncmp(cm->id, "NULL", CONF_S) == 0)) {
				retval = validate_user_input(cm->name, CUSTOMER_REGEX);
			} else {
				printf("Both name and coid set to NULL??\n");
				return 1;
			}
			if (retval < 0) {
				printf("User input not valid\n");
				return 1;
			}
			if (cm->action == DISPLAY)
				display_customer_info(cm->name, cm->id, cmc);
			else if (cm->action == LIST_OBJ)
				display_all_customers(cmc);
			break;
		default:
			printf("Not implemented yet :(\n");
			break;
	}
	return 0;
}
#endif
*/

