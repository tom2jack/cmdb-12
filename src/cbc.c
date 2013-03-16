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
 *  cbc.c
 * 
 *  main function for the cbc program
 * 
 *  (C) Iain M. Conochie 2012 - 2013
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdb.h"
#include "cmdb_cbc.h"
#include "cbc_data.h"
#include "build.h"
/* Added for testing
#include "cbc_base_sql.h"
 End testing additions */
#include "checks.h"

int
main(int argc, char *argv[])
{
	cbc_config_s *cmc;
	cbc_comm_line_s *cml;
	char sretval[MAC_S];
	const char *config = "/etc/dnsa/dnsa.conf";
	int retval;
	
	retval = 0;
	if (!(cmc = malloc(sizeof(cbc_config_s))))
		report_error(MALLOC_FAIL, "cmc in cbc.c");
	if (!(cml = malloc(sizeof(cbc_comm_line_s))))
		report_error(MALLOC_FAIL, "cml in cbc.c");
	
	
	init_all_config(cmc, cml);
	
	retval = parse_cbc_command_line(argc, argv, cml);
	if (retval < 0) {
		free(cmc);
		free(cml);
		display_cmdb_command_line_error(retval, argv[0]);
	}
	retval = parse_cbc_config_file(cmc, config);
	
	if (retval > 1) {
		parse_cbc_config_error(retval);
		free(cml);
		free(cmc);
		exit(retval);
	}
	if (cml->action == DISPLAY_CONFIG)
		retval = display_build_config(cmc, cml);
	else
		printf("Case %d not implemented yet\n", cml->action);
/*	print_cbc_command_line_values(cml); */
/*	switch (cml->action) {
		case WRITE_CONFIG:
			if ((retval = get_server_name(cml, cmc)) != 0) {
				free(cmc);
				free(cml);
				free(cbt);
				report_error(retval, "NULL");
			}
			retval = get_build_info(cmc, cbt, cml->server_id);
			write_tftp_config(cmc, cbt);
			write_dhcp_config(cmc, cbt);
			retval = write_build_config(cmc, cbt);
			break;
		case DISPLAY_CONFIG:
			if ((cml->server > 0) && 
			 (strncmp(cml->action_type, "NULL", MAC_S) == 0)) {
				retval = get_server_name(cml, cmc);
				retval = get_build_info(cmc, cbt, cml->server_id);
				print_cbc_build_values(cbt);
			} else if (cml->server == 0) {
				if ((strncmp(cml->action_type, "partition", MAC_S) == 0))
					display_partition_schemes(cmc);
				else if ((strncmp(cml->action_type, "os", MAC_S) == 0))
					display_build_operating_systems(cmc);
				else if ((strncmp(cml->action_type, "os_version", MAC_S) == 0))
					display_build_os_versions(cmc);
				else if ((strncmp(cml->action_type, "build_domain", MAC_S) == 0))
					display_build_domains(cmc);
				else if ((strncmp(cml->action_type, "varient", MAC_S) == 0))
					display_build_varients(cmc);
				else if ((strncmp(cml->action_type, "locale", MAC_S) == 0))
					display_build_locales(cmc);
				else
					printf("Unknown action type %s\n", cml->action_type);
			} else if (cml->server > 0) {
				printf("Cannot handle server and type actions\n");
			}
			break;
		case ADD_CONFIG:
			if (strncmp(cml->action_type, "partition", MAC_S) == 0)
				retval = add_partition_scheme(cmc);
			else if (strncmp(cml->action_type, "os", MAC_S) == 0)
				printf("Adding OS not yet implemented\n");
			else if (strncmp(cml->action_type, "os_version", MAC_S) == 0)
				printf("Adding OS version not yet implemented\n");
			else if (strncmp(cml->action_type, "build_domain", MAC_S) == 0)
				printf("Adding build_domain not yet implemented\n");
			else if (strncmp(cml->action_type, "varient", MAC_S) == 0)
				printf("Adding varient not yet implemented\n");
			else
				printf("Case %s not implemented yet\n",
				 cml->action_type);
			break;
		case CREATE_CONFIG:
			retval = create_build_config(cmc, cml, cbt);
			if (retval == 0) {
				printf("Config created in database\n");
			} else {
				get_error_string(retval, sretval);
				printf("Retval from create_build_config was: %d\n", retval);
				free(cmc);
				free(cml);
				free(cbt);
				report_error(CREATE_BUILD_FAILED, sretval);
			}
			break;
		default:
			printf("Case %d not implemented yet\n", cml->action);
			break;
	} */
	free(cmc);
	free(cml);
	if (retval == DISPLAY_USAGE)
		retval = NONE;
	if (retval != NONE) {
		get_error_string(retval, sretval);
		report_error(CREATE_BUILD_FAILED, sretval);
	}
	exit(retval);
}
