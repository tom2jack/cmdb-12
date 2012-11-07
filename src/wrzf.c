/* wrzf.c:
 * 
 * Contains the function to write out the reverse zonefile. 
 * 
 * This has some basic error checking, and if these checks are passed
 * then the zone is updated as valid in the database.
 * 
 * You still need to write out the configuration file for the zones
 * to be implemented in BIND
 * 
 * (C) Iain M Conochie 2012 <iain@ailsatech.net>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dnsa.h"
#include "reverse.h"

int wrzf(int reverse, dnsa_config_t *dc)
{
	FILE *cnf;
	MYSQL dnsa;
	MYSQL_RES *dnsa_res;
	MYSQL_ROW dnsa_row;
	size_t len;
	rev_zone_info_t rev_zone_info;
	rev_record_row_t rev_row;
	my_ulonglong dnsa_rows;
	int error, i;
	unsigned int port;
	unsigned long int client_flag;
	char *tmp, *rout, *dquery, *domain;
	const char *unix_socket, *dnsa_query, *syscom;
	
	port = 3306;
	client_flag = 0;
	
	if (!(dquery = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "dquery in wrzf");
	if (!(tmp = calloc(BUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "tmp in wrzf");
	if (!(rout = calloc(FILE_S, sizeof(char))))
		report_error(MALLOC_FAIL, "rout in wrzf");
	if (!(domain = calloc(CONF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "domain in wrzf");
	
	dnsa_query = dquery;
	unix_socket = dc->socket;
	
	/* Initialise MYSQL connection and query */
	if (!(mysql_init(&dnsa))) {
		fprintf(stderr, "Cannot init. Out of memory?\n");
		return MY_INIT_FAIL;
	}
	if (!(mysql_real_connect(&dnsa,
		dc->host, dc->user, dc->pass, dc->db, dc->port, unix_socket, dc->cliflag ))) {
		fprintf(stderr, "Connect failed. Error: %s\n",
			mysql_error(&dnsa));
		return MY_CONN_FAIL;
	}
	
	sprintf(dquery, "SELECT * FROM rev_zones WHERE rev_zone_id = '%d'", reverse);
	error = mysql_query(&dnsa, dnsa_query);
	
	if ((error != 0)) {
		fprintf(stderr, "Query not successful: error code %d\n", error);
		return MY_QUERY_FAIL;
	}
	if (!(dnsa_res = mysql_store_result(&dnsa))) {
		fprintf(stderr, "Cannot store result set\n");
		return MY_STORE_FAIL;
	}
	if (((dnsa_rows = mysql_num_rows(dnsa_res)) == 0)) {
		fprintf(stderr, "Reverse zone id %d not found\n", reverse);
		return NO_DOMAIN;
	} else if (dnsa_rows > 1) {
		fprintf(stderr, "Multiple rows found for reverse zone id %d\n", reverse);
		return MULTI_DOMAIN;
	}
	
	/* Get the information for the reverse zone */
	while ((dnsa_row = mysql_fetch_row(dnsa_res))) {
		rev_zone_info = fill_rev_zone_data(dnsa_row);
	}
	
	/* Start the output string with the zonefile header */
	len = create_rev_zone_header(rev_zone_info, rout);
	
	sprintf(dquery, "SELECT host, destination FROM rev_records WHERE rev_zone = '%d'", reverse);
	error = mysql_query(&dnsa, dnsa_query);
	
	if ((error != 0)) {
		fprintf(stderr, "Rev record query unsuccessful: error code %d\n", error);
		return MY_QUERY_FAIL;
	}
	if (!(dnsa_res = mysql_store_result(&dnsa))) {
		fprintf(stderr, "Cannot store result set\n");
		return MY_STORE_FAIL;
	}
	if (((dnsa_rows = mysql_num_rows(dnsa_res)) == 0)) {
		fprintf(stderr, "No reverse records for zone %d\n", reverse);
		return NO_RECORDS;
	}
	
	/* Add the reverse zone records */
	while ((dnsa_row = mysql_fetch_row(dnsa_res))) {
		rev_row = get_rev_row(dnsa_row);
		add_rev_records(rout, rev_row);
	}
	
	/* Build the config filename from config values */
	len = strlen(dc->dir);
	strncpy(domain, dc->dir, len);
	len = strlen(rev_zone_info.net_range);
	strncat(domain, rev_zone_info.net_range, len);
	
	/* Write out the reverse zone to the zonefile */
	if (!(cnf = fopen(domain, "w"))) {
		fprintf(stderr, "Cannot open config file %s for writing\n", domain);
		return FILE_O_FAIL;
	} else {
		fputs(rout, cnf);
		fclose(cnf);
	}
	
	for (i=0; i< CONF_S; i++)	/* zero tmp buffer */
		*(tmp + i) = '\0';
	for (i=0; i < FILE_S; i++)	/* zero rout file output buffer */
		*(rout + i) = '\0';
	
	/* Check if the zonefile is valid. If so update the DB to say zone
	 * is valid. */
	get_in_addr_string(tmp, rev_zone_info.net_range);
	sprintf(rout, "%s %s %s", dc->chkz, tmp, domain);
	syscom = rout;
	error = system(syscom);
	if (error == 0) {
		printf("check of zone %s ran successfully\n", domain);
		sprintf(dquery, "UPDATE rev_zones SET valid = 'yes', updated = 'no' WHERE rev_zone_id = %d",
			reverse);
		error = mysql_query(&dnsa, dnsa_query);
		dnsa_rows = mysql_affected_rows(&dnsa);
		if ((dnsa_rows == 1)) {
			fprintf(stderr, "Rev Zone id %d set to valid in DB\n", reverse);
		} else if ((dnsa_rows == 0)) {
			if ((error == 0)) {
				fprintf(stderr, "Rev zone id %d already valid in DB\n", reverse);
			} else {
				fprintf(stderr, "Rev Zone id %d not validated in DB\n", reverse);
				fprintf(stderr, "Error code from query is: %d\n", error);
			}
		} else {
			fprintf(stderr, "More than one zone update?? Multiple ID's %d??\n",
				reverse);
		}
	} else {
		printf("Check of zone %s failed. Error code %d\n", domain, error);
	}
	
	mysql_close(&dnsa);
	free(tmp);
	free(rout);
	free(dquery);
	return 0;
}
