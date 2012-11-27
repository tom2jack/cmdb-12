/* cbc.c
 * 
 * main function for the cbc program
 * 
 * (C) 2012 Iain M. Conochie
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdb.h"
#include "cmdb_cbc.h"
#include "checks.h"

int main(int argc, char *argv[])
{
	cbc_config_t cbc_c, *cmc;
	cbc_comm_line_t cbt_c, *cml;
	cbc_build_t cb_t, *cbt;
	char *cbc_config;
	int retval;
	
	retval = 0;
	if (!(cbc_config = malloc(CONF_S * sizeof(char))))
		report_error(MALLOC_FAIL, "cbc_config in cbc.c");
	
	cmc = &cbc_c;
	cml = &cbt_c;
	cbt = &cb_t;
	strncpy(cbc_config, "/etc/dnsa/dnsa.conf", CONF_S - 1);
	
	init_cbc_config_values(cmc);
	init_cbc_comm_values(cml);
	init_cbc_build_values(cbt);
	
	retval = parse_cbc_command_line(argc, argv, cml, cbt);
	retval = parse_cbc_config_file(cmc, cbc_config);
	
	free(cbc_config);
	if (retval > 1) {
		parse_cbc_config_error(retval);
		exit(retval);
	}
	print_cbc_config(cmc);
	print_cbc_command_line_values(cml);
	print_cbc_build_values(cbt);
	exit(retval);
}