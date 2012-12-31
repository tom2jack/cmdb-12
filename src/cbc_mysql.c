/* cmdb_mysql.c
 *
 * Contains the functions for mysql access.
 * 
 * Part of the cmdb program
 *
 * (C) 2012 Iain M Conochie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "cmdb.h"
#include "cmdb_cbc.h"
#include "mysqlfunc.h"

const char *error_string;
char *my_error_code;

void cbc_mysql_init(cbc_config_t *dc, MYSQL *cbc_mysql)
{
	const char *unix_socket;
	
	if (!(my_error_code = calloc(RBUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "my_error_code in dnsa_mysql_init");
	
	unix_socket = dc->socket;
	error_string = my_error_code;
	
	if (!(mysql_init(cbc_mysql))) {
		sprintf(my_error_code, "none");
		report_error(MY_INIT_FAIL, error_string);
	}
	if (!(mysql_real_connect(cbc_mysql, dc->host, dc->user, dc->pass,
		dc->db, dc->port, unix_socket, dc->cliflag)))
		report_error(MY_CONN_FAIL, mysql_error(cbc_mysql));
	
	free(my_error_code);
}

void cmdb_mysql_query(MYSQL *mycmdb, const char *query)
{
	int error;
	
	if (!(my_error_code = calloc(RBUFF_S, sizeof(char))))
		report_error(MALLOC_FAIL, "my_error_code in dnsa_mysql_init");
	
	error_string = my_error_code;
	
	error = mysql_query(mycmdb, query);
	if ((error != 0)) {
		report_error(MY_QUERY_FAIL, error_string);
	}
	free(my_error_code);
}