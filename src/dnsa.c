/* 
 * 
 * dnsa.c: First attempt to write a multi function command line program
 * for dnsa.
 * Command line args:
 * 
 * -d :display
 * -w :write
 * -c :write configuration
 * -l :list zones in database.
 *   **At least one of these must be present**
 * -f : forward zone
 * -r : Reverse zone
 *   **One of these must be present but not both**
 * -n : domain name or netblock. only /8 /16 /24 netblocks accepted
 *      needed for -d and -w functionality
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "dnsa.h"
#include "forward.h"
#include "reverse.h"

int main(int argc, char *argv[])
{
	comm_line_t command;
	dnsa_config_t dnsa_c, *dc;
	char *domain, *config;
	int retval, id;

	dc = &dnsa_c;
	
	/* Get command line args. See above */
	retval = parse_command_line(argc, argv, &command);
	if (retval < 0) {
		printf("Usage: %s [-d | -w | -c | -l] [-f | -r] -n <domain/netrange>\n",
			       argv[0]);
		exit (retval);
	}
	
	if (!(domain = malloc(CONF_S * sizeof(char))))
		report_error(MALLOC_FAIL, "domain in dnsa.c");
	if (!(config = malloc(CONF_S * sizeof(char))))
		report_error(MALLOC_FAIL, "config in dnsa.c");
	
	/* Get config values from config file */	
	init_config_values(dc);
	sprintf(config, "/etc/dnsa/dnsa.conf");
	retval = parse_config_file(dc, config);
	if (retval < 0) {
		printf("Config file parsing failed! Using default values\n");
	}

	strncpy(domain, command.domain, CONF_S);
	if ((strncmp(command.action, "write", COMM_S) == 0)) {
		if ((strncmp(command.type, "forward", COMM_S) == 0)) {
			wzf(domain, dc);
		} else if ((strncmp(command.type, "reverse", COMM_S) == 0)) {
			id = get_rev_id(domain, dc);
			if (id < 0) {
				report_error(NO_DOMAIN, domain);
			} else {
				wrzf(id, dc);
			}
		} else {
			retval = WRONG_TYPE;
			printf("We have an invalid type: %s\n",
			       command.type);
			exit(retval);
		}
	} else if ((strncmp(command.action, "display", COMM_S) == 0)) {
		if ((strncmp(command.type, "forward", COMM_S) == 0)) {
			dzf(domain, dc);
		} else if ((strncmp(command.type, "reverse", COMM_S) == 0)) {
			id = get_rev_id(domain, dc);
			if (id < 0) {
				report_error(NO_DOMAIN, domain);
			} else {
				drzf(id, domain, dc);
			}
		} else {
			retval = WRONG_TYPE;
			printf("We have an invalid type: %s\n",
			       command.type);
			exit(retval);
		}
	} else if ((strncmp(command.action, "config", COMM_S) == 0)) {
		if ((strncmp(command.type, "forward", COMM_S) == 0)) {
			wcf(dc);
		} else if ((strncmp(command.type, "reverse", COMM_S) == 0)) {
			wrcf(dc);
		} else {
			retval = WRONG_TYPE;
			printf("We have an invalid type: %s\n",
			       command.type);
			exit(retval);
		}
	} else if ((strncmp(command.action, "list", COMM_S) == 0)) {
		if ((strncmp(command.type, "forward", COMM_S) == 0)) {
			list_zones(dc);
		} else if ((strncmp(command.type, "reverse", COMM_S) == 0)) {
			list_rev_zones(dc);
		} else {
			retval = WRONG_TYPE;
			printf("We have an invalid type: %s\n",
			       command.type);
			exit(retval);
		}
	}
	
	free(config);
	free(domain);
	exit(0);
}