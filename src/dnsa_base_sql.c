/* 
 *
 *  dnsa: DNS administration
 *  Copyright (C) 2013  Iain M Conochie <iain-AT-thargoid.co.uk>
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
 *  dnsa_base_sql.c:
 *
 *  Contains functions which will fill up data structs based on the parameters
 *  supplied. Will also contian conditional code base on database type.
 */
#include "../config.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmdb.h"
#include "cmdb_dnsa.h"
#include "dnsa_base_sql.h"
#ifdef HAVE_LIBPCRE
# include "checks.h"
#endif /* HAVE_LIBPCRE */
#ifdef HAVE_MYSQL
# include <mysql.h>
# include "mysqlfunc.h"
#endif /* HAVE_MYSQL */
#ifdef HAVE_SQLITE3
# include <sqlite3.h>
#endif /* HAVE_SQLITE3 */

const char *sql_select[] = { "\
SELECT id, name, pri_dns, sec_dns, serial, refresh, retry, expire, ttl, \
valid, owner, updated FROM zones ORDER BY name","\
SELECT rev_zone_id, net_range, prefix, net_start, net_finish, start_ip, \
finish_ip, pri_dns, sec_dns, serial, refresh, retry, expire, ttl, valid, \
owner, updated FROM rev_zones ORDER BY start_ip","\
SELECT id, zone, host, type, pri, destination, valid FROM records ORDER \
BY zone, type, host","\
SELECT rev_record_id, rev_zone, host, destination, valid FROM rev_records"
};

const char *sql_search[] = { "\
SELECT id FROM zones WHERE name = ?","\
SELECT rev_zone_id FROM rev_zones WHERE net_range = ?"
};

const char *sql_insert[] = {"\
INSERT INTO zones (name, pri_dns, sec_dns, serial, refresh, retry, expire, \
ttl) VALUES (?, ?, ?, ?, ?, ?, ?, ?)","\
INSERT INTO rev_zones (net_range, prefix, net_start, net_finish, start_ip, \
finish_ip, pri_dns, sec_dns, serial, refresh, retry, expire, ttl) VALUES \
(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)","\
INSERT INTO records (zone, host, type, pri, destination) VALUES \
(?, ?, ?, ?, ?)","\
INSERT INTO rev_records (rev_zone, host, destination) VALUES (?, ?, ?)"
};

const char *sql_update[] = {"\
UPDATE zones SET valid = 'yes' WHERE id = ?","\
UPDATE zones SET updated = 'yes' WHERE id = ?","\
UPDATE zones SET updated = 'no' WHERE id = ?"
};

#ifdef HAVE_MYSQL

const int mysql_inserts[][13] = {
{MYSQL_TYPE_STRING, MYSQL_TYPE_STRING, MYSQL_TYPE_STRING, MYSQL_TYPE_LONG, 
    MYSQL_TYPE_LONG, MYSQL_TYPE_LONG, MYSQL_TYPE_LONG, MYSQL_TYPE_LONG,
    0, 0, 0, 0, 0} ,
{MYSQL_TYPE_STRING, MYSQL_TYPE_LONG, MYSQL_TYPE_STRING, MYSQL_TYPE_STRING, 
    MYSQL_TYPE_LONG, MYSQL_TYPE_LONG, MYSQL_TYPE_STRING, MYSQL_TYPE_STRING,
    MYSQL_TYPE_LONG, MYSQL_TYPE_LONG, MYSQL_TYPE_LONG, MYSQL_TYPE_LONG,
    MYSQL_TYPE_LONG} , 
{MYSQL_TYPE_LONG, MYSQL_TYPE_STRING, MYSQL_TYPE_STRING, MYSQL_TYPE_LONG,
    MYSQL_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0, 0}, 
{MYSQL_TYPE_LONG, MYSQL_TYPE_STRING, MYSQL_TYPE_STRING, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0}
};

#endif /* HAVE_MYSQL */

const unsigned int select_fields[] = { 12, 17, 7, 5 };

const unsigned int insert_fields[] = { 8, 13, 5, 3 };

const unsigned int search_fields[] = { 1, 1 };

const unsigned int search_args[] = { 1, 1 };

const unsigned int search_field_type[][1] = { /* What we are selecting */
	{ DBINT } ,
	{ DBINT }
};

const unsigned int search_arg_type[][1] = { /* What we are searching on */
	{ DBTEXT } ,
	{ DBTEXT }
};

const unsigned int update_field_type[][1] = {
	{ NONE } ,
	{ NONE } ,
	{ NONE }
};

const unsigned int update_arg_type[][1] = {
	{ DBINT } ,
	{ DBINT } ,
	{ DBINT } 
};

int
run_query(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;
	if ((strncmp(config->dbtype, "none", RANGE_S) == 0)) {
		fprintf(stderr, "No database type configured\n");
		return NO_DB_TYPE;
#ifdef HAVE_MYSQL
	} else if ((strncmp(config->dbtype, "mysql", RANGE_S) == 0)) {
		retval = run_query_mysql(config, base, type);
		return retval;
#endif /* HAVE_MYSQL */
#ifdef HAVE_SQLITE3
	} else if ((strncmp(config->dbtype, "sqlite", RANGE_S) == 0)) {
		retval = run_query_sqlite(config, base, type);
		return retval;
#endif /* HAVE_SQLITE3 */
	} else {
		fprintf(stderr, "Unknown database type %s\n", config->dbtype);
		return DB_TYPE_INVALID;
	}
	
	return NONE;
}

int
run_multiple_query(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;
	retval = NONE;
	if ((strncmp(config->dbtype, "none", RANGE_S) == 0)) {
		fprintf(stderr, "No database type configured\n");
		return NO_DB_TYPE;
#ifdef HAVE_MYSQL
	} else if ((strncmp(config->dbtype, "mysql", RANGE_S) == 0)) {
		retval = run_multiple_query_mysql(config, base, type);
		return retval;
#endif /* HAVE_MYSQL */
#ifdef HAVE_SQLITE3
	} else if ((strncmp(config->dbtype, "sqlite", RANGE_S) == 0)) {
		retval = run_multiple_query_sqlite(config, base, type);
		return retval;
#endif /* HAVE_SQLITE3 */
	} else {
		fprintf(stderr, "Unknown database type %s\n", config->dbtype);
		return DB_TYPE_INVALID;
	}
	
	return retval;
}

int
run_search(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;

	if ((strncmp(config->dbtype, "none", RANGE_S) ==0)) {
		fprintf(stderr, "No database type configured\n");
		return NO_DB_TYPE;
#ifdef HAVE_MYSQL
	} else if ((strncmp(config->dbtype, "mysql", RANGE_S) == 0)) {
		retval = run_search_mysql(config, base, type);
		return retval;
#endif /* HAVE_MYSQL */
#ifdef HAVE_SQLITE3
	} else if ((strncmp(config->dbtype, "sqlite", RANGE_S) == 0)) {
		retval = run_search_sqlite(config, base, type);
		return retval;
#endif /* HAVE_SQLITE3 */
	} else {
		fprintf(stderr, "Unknown database type %s\n", config->dbtype);
		return DB_TYPE_INVALID;
	}

	return NONE;
}

int
run_insert(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;
	if ((strncmp(config->dbtype, "none", RANGE_S) == 0)) {
		fprintf(stderr, "No database type configured\n");
		return NO_DB_TYPE;
#ifdef HAVE_MYSQL
	} else if ((strncmp(config->dbtype, "mysql", RANGE_S) == 0)) {
		retval = run_insert_mysql(config, base, type);
		return retval;
#endif /* HAVE_MYSQL */
#ifdef HAVE_SQLITE3
	} else if ((strncmp(config->dbtype, "sqlite", RANGE_S) == 0)) {
		retval = run_insert_sqlite(config, base, type);
		return retval;
#endif /* HAVE_SQLITE3 */
	} else {
		fprintf(stderr, "Unknown database type %s\n", config->dbtype);
		return DB_TYPE_INVALID;
	}

	return NONE;
}

int
get_query(int type, const char **query, unsigned int *fields)
{
	int retval;
	
	retval = NONE;
	switch(type) {
		case ZONE:
			*query = sql_select[ZONES];
			*fields = select_fields[ZONES];
			break;
		case REV_ZONE:
			*query = sql_select[REV_ZONES];
			*fields = select_fields[REV_ZONES];
			break;
		case RECORD:
			*query = sql_select[RECORDS];
			*fields = select_fields[RECORDS];
			break;
		case REV_RECORD:
			*query = sql_select[REV_RECORDS];
			*fields = select_fields[REV_RECORDS];
			break;
		default:
			fprintf(stderr, "Unknown query type %d\n", type);
			retval = 1;
			break;
	}
	
	return retval;
}

void
get_search(int type, size_t *fields, size_t *args, void **input, void **output, dnsa_t *base)
{
	switch(type) {
		case ZONE_ID_ON_NAME:
			*input = &(base->zones->name);
			*output = &(base->zones->id);
			*fields = strlen(base->zones->name);
			*args = sizeof(base->zones->id);
			break;
		case REV_ZONE_ID_ON_NET_RANGE:
			*input = &(base->rev_zones->net_range);
			*output = &(base->rev_zones->rev_zone_id);
			*fields = strlen(base->rev_zones->net_range);
			*args = sizeof(base->rev_zones->rev_zone_id);
			break;
		default:
			fprintf(stderr, "Unknown query %d\n", type);
			exit (NO_QUERY);
	}
}

#ifdef HAVE_MYSQL
void
cmdb_mysql_init(dnsa_config_t *dc, MYSQL *dnsa_mysql)
{
	const char *unix_socket;
	
	unix_socket = dc->socket;
	
	if (!(mysql_init(dnsa_mysql))) {
		report_error(MY_INIT_FAIL, mysql_error(dnsa_mysql));
	}
	if (!(mysql_real_connect(dnsa_mysql, dc->host, dc->user, dc->pass,
		dc->db, dc->port, unix_socket, dc->cliflag)))
		report_error(MY_CONN_FAIL, mysql_error(dnsa_mysql));
	
}

int
run_query_mysql(dnsa_config_t *config, dnsa_t *base, int type)
{
	MYSQL dnsa;
	MYSQL_RES *dnsa_res;
	MYSQL_ROW dnsa_row;
	my_ulonglong dnsa_rows;
	const char *query;
	int retval;
	unsigned int fields;
	
	retval = 0;
	cmdb_mysql_init(config, &dnsa);
	if ((retval = get_query(type, &query, &fields)) != 0) {
		fprintf(stderr, "Unable to get query. Error code %d\n", retval);
		return retval;
	}
	if ((retval = cmdb_mysql_query_with_checks(&dnsa, query)) != 0) {
		fprintf(stderr, "Query failed with error code %d\n", retval);
		return retval;
	}
	if (!(dnsa_res = mysql_store_result(&dnsa))) {
		cmdb_mysql_cleanup(&dnsa);
		report_error(MY_STORE_FAIL, mysql_error(&dnsa));
	}
	fields = mysql_num_fields(dnsa_res);
	if (((dnsa_rows = mysql_num_rows(dnsa_res)) == 0)) {
		cmdb_mysql_cleanup_full(&dnsa, dnsa_res);
		report_error(NO_SERVERS, "run_query_mysql");
	}
	while ((dnsa_row = mysql_fetch_row(dnsa_res)))
		store_result_mysql(dnsa_row, base, type, fields);
	cmdb_mysql_cleanup_full(&dnsa, dnsa_res);
	return 0;
}

int
run_multiple_query_mysql(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;
	retval = NONE;
	if ((type & ZONE) == ZONE)
		if ((retval = run_query_mysql(config, base, ZONE)) != 0)
			return retval;
	if ((type & REV_ZONE) == REV_ZONE)
		if ((retval = run_query_mysql(config, base, REV_ZONE)) != 0)
			return retval;
	if ((type & RECORD) == RECORD)
		if ((retval = run_query_mysql(config, base, RECORD)) != 0)
			return retval;
	if ((type & REV_RECORD) == REV_RECORD)
		if ((retval = run_query_mysql(config, base, REV_RECORD)) != 0)
			return retval;
	return retval;
}

void
store_result_mysql(MYSQL_ROW row, dnsa_t *base, int type, unsigned int fields)
{
	switch(type) {
		case ZONE:
			if (fields != select_fields[ZONES])
				break;
			store_zone_mysql(row, base);
			break;
		case REV_ZONE:
			if (fields != select_fields[REV_ZONES])
				break;
			store_rev_zone_mysql(row, base);
			break;
		case RECORD:
			if (fields != select_fields[RECORDS])
				break;
			store_record_mysql(row, base);
			break;
		case REV_RECORD:
			if (fields != select_fields[REV_RECORDS])
				break;
			store_rev_record_mysql(row, base);
			break;
		default:
			fprintf(stderr, "Unknown type %d\n",  type);
			break;
	}
			
}

void
store_zone_mysql(MYSQL_ROW row, dnsa_t *base)
{
	int retval;
	zone_info_t *zone, *list;
	
	if (!(zone = malloc(sizeof(zone_info_t))))
		report_error(MALLOC_FAIL, "zone in store_zone_mysql");
	init_zone_struct(zone);
	zone->id = strtoul(row[0], NULL, 10);
	snprintf(zone->name, RBUFF_S, "%s", row[1]);
	snprintf(zone->pri_dns, RBUFF_S - 1, "%s", row[2]);
	if ((retval = add_trailing_dot(zone->pri_dns)) != 0)
		fprintf(stderr, "Unable to add trailing dot to PRI_NS\n");
	snprintf(zone->sec_dns, RBUFF_S - 1, "%s", row[3]);
	if (strncmp(zone->sec_dns, "(null)", COMM_S) != 0)
		if ((retval = add_trailing_dot(zone->sec_dns)) != 0)
			fprintf(stderr, "Unable to add trailing dot to SEC_NS\n");
	zone->serial = strtoul(row[4], NULL, 10);
	zone->refresh = strtoul(row[5], NULL, 10);
	zone->retry = strtoul(row[6], NULL, 10);
	zone->expire = strtoul(row[7], NULL, 10);
	zone->ttl = strtoul(row[8], NULL, 10);
	snprintf(zone->valid, RANGE_S, "%s", row[9]);
	zone->owner = strtoul(row[10], NULL, 10);
	snprintf(zone->updated, RANGE_S, "%s", row[11]);
	list = base->zones;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = zone;
	} else {
		base->zones = zone;
	}
}

void
store_record_mysql(MYSQL_ROW row, dnsa_t *base)
{
	record_row_t *rec, *list;
	
	if (!(rec = malloc(sizeof(record_row_t))))
		report_error(MALLOC_FAIL, "rec in store_record_mysql");
	init_record_struct(rec);
	rec->id = strtoul(row[0], NULL, 10);
	rec->zone = strtoul(row[1], NULL, 10);
	snprintf(rec->host, RBUFF_S, "%s", row[2]);
	snprintf(rec->type, RANGE_S, "%s", row[3]);
	rec->pri = strtoul(row[4], NULL, 10);
	snprintf(rec->dest, RBUFF_S, "%s", row[5]);
	snprintf(rec->valid, RANGE_S, "%s", row[6]);
	list = base->records;
	if (list) {
		while(list->next)
			list = list->next;
		list->next = rec;
	} else {
		base->records = rec;
	}
}

void
store_rev_zone_mysql(MYSQL_ROW row, dnsa_t *base)
{
	int retval;
	rev_zone_info_t *rev, *list;
	
	if (!(rev = malloc(sizeof(rev_zone_info_t))))
		report_error(MALLOC_FAIL, "rev in store_rev_zone_mysql");
	init_rev_zone_struct(rev);
	rev->rev_zone_id = strtoul(row[0], NULL, 10);
	snprintf(rev->net_range, RANGE_S, "%s", row[1]);
	rev->prefix = strtoul(row[2], NULL, 10);
	snprintf(rev->net_start, RANGE_S, "%s", row[3]);
	snprintf(rev->net_finish, RANGE_S, "%s", row[4]);
	rev->start_ip = strtoul(row[5], NULL, 10);
	rev->end_ip = strtoul(row[6], NULL, 10);
	snprintf(rev->pri_dns, RBUFF_S - 1, "%s", row[7]);
	if ((retval = add_trailing_dot(rev->pri_dns)) != 0)
		fprintf(stderr, "Unable to add trailing dot to PRI_NS\n");
	snprintf(rev->sec_dns, RBUFF_S - 1, "%s", row[8]);
	if (strncmp(rev->sec_dns, "(null)", COMM_S) != 0)
		if ((retval = add_trailing_dot(rev->sec_dns)) != 0)
			fprintf(stderr, "Unable to add trailing dot to SEC_NS\n");
	rev->serial = strtoul(row[9], NULL, 10);
	rev->refresh = strtoul(row[10], NULL, 10);
	rev->retry = strtoul(row[11], NULL, 10);
	rev->expire = strtoul(row[12], NULL, 10);
	rev->ttl = strtoul(row[13], NULL, 10);
	snprintf(rev->valid, RANGE_S, "%s", row[14]);
	rev->owner = strtoul(row[15], NULL, 10);
	snprintf(rev->updated, RANGE_S, "%s", row[16]);
	list = base->rev_zones;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = rev;
	} else {
		base->rev_zones = rev;
	}
}

void
store_rev_record_mysql(MYSQL_ROW row, dnsa_t *base)
{
	rev_record_row_t *rev, *list;
	
	if (!(rev = malloc(sizeof(rev_record_row_t))))
		report_error(MALLOC_FAIL, "rev in store_rev_record_mysql");
	init_rev_record_struct(rev);
	rev->record_id = strtoul(row[0], NULL, 10);
	rev->rev_zone = strtoul(row[1], NULL, 10);
	snprintf(rev->host, RBUFF_S, "%s", row[2]);
	snprintf(rev->dest, RBUFF_S, "%s", row[3]);
	snprintf(rev->valid, RANGE_S, "%s", row[4]);
	list = base->rev_records;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = rev;
	} else {
		base->rev_records = rev;
	}
}

int
run_search_mysql(dnsa_config_t *config, dnsa_t *base, int type)
{
	MYSQL dnsa;
	MYSQL_STMT *dnsa_stmt;
	MYSQL_BIND my_bind[2];
	const char *query;
	int retval;
	size_t arg, res;
	void *input, *output;

	retval = 0;
	cmdb_mysql_init(config, &dnsa);
	memset(my_bind, 0, sizeof(my_bind));
/* 
Will need to check if we have char or int here. Hard coded char for search,
and int for result, which is OK when searching on name and returning id
*/
	query = sql_search[type];
	get_search(type, &arg, &res, &input, &output, base);
	my_bind[0].buffer_type = MYSQL_TYPE_STRING;
	my_bind[0].buffer = input;
	my_bind[0].buffer_length = arg;
	my_bind[0].is_unsigned = 0;
	my_bind[0].is_null = 0;
	my_bind[0].length = 0;
	my_bind[1].buffer_type = MYSQL_TYPE_LONG;
	my_bind[1].buffer = output;
	my_bind[1].buffer_length = res;
	my_bind[1].is_unsigned = 1;
	my_bind[1].is_null = 0;
	my_bind[1].length = 0;
	
	if (!(dnsa_stmt = mysql_stmt_init(&dnsa)))
		return MY_STATEMENT_FAIL;
	if ((retval = mysql_stmt_prepare(dnsa_stmt, query, strlen(query))) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_bind_param(dnsa_stmt, &my_bind[0])) != 0)
		report_error(MY_BIND_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_bind_result(dnsa_stmt, &my_bind[1])) != 0)
		report_error(MY_BIND_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_execute(dnsa_stmt)) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_store_result(dnsa_stmt)) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_fetch(dnsa_stmt)) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));

	mysql_stmt_free_result(dnsa_stmt);
	mysql_stmt_close(dnsa_stmt);
	cmdb_mysql_cleanup(&dnsa);
	return retval;
}

int
run_insert_mysql(dnsa_config_t *config, dnsa_t *base, int type)
{
	MYSQL dnsa;
	MYSQL_STMT *dnsa_stmt;
	MYSQL_BIND my_bind[insert_fields[type]];
	const char *query;
	int retval;
	unsigned int i;

	retval = 0;
	memset(my_bind, 0, sizeof(my_bind));
	for (i = 0; i < insert_fields[type]; i++)
		if ((retval = setup_insert_mysql_bind(&my_bind[i], i, type, base)) != 0)
			return retval;
	query = sql_insert[type];
	cmdb_mysql_init(config, &dnsa);
	if (!(dnsa_stmt = mysql_stmt_init(&dnsa)))
		return MY_STATEMENT_FAIL;
	if ((retval = mysql_stmt_prepare(dnsa_stmt, query, strlen(query))) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_bind_param(dnsa_stmt, &my_bind[0])) != 0)
		report_error(MY_BIND_FAIL, mysql_stmt_error(dnsa_stmt));
	if ((retval = mysql_stmt_execute(dnsa_stmt)) != 0)
		report_error(MY_STATEMENT_FAIL, mysql_stmt_error(dnsa_stmt));
	
	mysql_stmt_close(dnsa_stmt);
	cmdb_mysql_cleanup(&dnsa);
	
	return retval;
}

int
setup_insert_mysql_bind(MYSQL_BIND *bind, unsigned int i, int type, dnsa_t *base)
{
	int retval;

	retval = 0;
	void *buffer;
	bind->buffer_type = mysql_inserts[type][i];
	bind->is_null = 0;
	bind->length = 0;
	if ((retval = setup_insert_mysql_bind_buffer(type, &buffer, base, i)) != 0)
		return retval;
	bind->buffer = buffer;
	if (bind->buffer_type == MYSQL_TYPE_STRING) {
		bind->is_unsigned = 0;
		bind->buffer_length = strlen(buffer);
	} else if (bind->buffer_type == MYSQL_TYPE_LONG) {
		bind->is_unsigned = 1;
		bind->buffer_length = sizeof(unsigned long int);
	} else {
		retval = WRONG_TYPE;
	}
	return retval;
}

int
setup_insert_mysql_bind_buffer(int type, void **input, dnsa_t *base, unsigned int i)
{
	int retval = 0;
	
	if (type == RECORDS)
		setup_insert_mysql_bind_buff_record(input, base, i);
	else if (type == ZONES)
		setup_insert_mysql_bind_buff_zone(input, base, i);
	else
		retval = WRONG_TYPE;
	
	return retval;
}

void
setup_insert_mysql_bind_buff_record(void **input, dnsa_t *base, unsigned int i)
{
	if (i == 0)
		*input = &(base->records->zone);
	else if (i == 1)
		*input = &(base->records->host);
	else if (i == 2)
		*input = &(base->records->type);
	else if (i == 3)
		*input = &(base->records->pri);
	else if (i == 4)
		*input = &(base->records->dest);
}

void
setup_insert_mysql_bind_buff_zone(void **input, dnsa_t *base, unsigned int i)
{
	if (i == 0)
		*input = &(base->zones->name);
	else if (i == 1)
		*input = &(base->zones->pri_dns);
	else if (i == 2)
		*input = &(base->zones->sec_dns);
	else if (i == 3)
		*input = &(base->zones->serial);
	else if (i == 4)
		*input = &(base->zones->refresh);
	else if (i == 5)
		*input = &(base->zones->retry);
	else if (i == 6)
		*input = &(base->zones->expire);
	else if (i == 7)
		*input = &(base->zones->ttl);
}

#endif /* HAVE_MYSQL */

#ifdef HAVE_SQLITE3

int
run_query_sqlite(dnsa_config_t *config, dnsa_t *base, int type)
{
	const char *query, *file;
	int retval;
	unsigned int fields;
	sqlite3 *dnsa;
	sqlite3_stmt *state;
	
	retval = 0;
	file = config->file;
	if ((retval = get_query(type, &query, &fields)) != 0) {
		fprintf(stderr, "Unable to get query. Error code %d\n", retval);
		return retval;
	}
	if ((retval = sqlite3_open_v2(file, &dnsa, SQLITE_OPEN_READONLY, NULL)) > 0) {
		report_error(CANNOT_OPEN_FILE, file);
	}
	if ((retval = sqlite3_prepare_v2(dnsa, query, BUFF_S, &state, NULL)) > 0) {
		retval = sqlite3_close(dnsa);
		report_error(SQLITE_STATEMENT_FAILED, "run_query_sqlite");
	}
	fields = (unsigned int) sqlite3_column_count(state);
	while ((retval = sqlite3_step(state)) == SQLITE_ROW)
		store_result_sqlite(state, base, type, fields);
	
	retval = sqlite3_finalize(state);
	retval = sqlite3_close(dnsa);
	
	return 0;
}

int
run_multiple_query_sqlite(dnsa_config_t *config, dnsa_t *base, int type)
{
	int retval;
	retval = NONE;
	if ((type & ZONE) == ZONE)
		if ((retval = run_query_sqlite(config, base, ZONE)) != 0)
			return retval;
	if ((type & REV_ZONE) == REV_ZONE)
		if ((retval = run_query_sqlite(config, base, REV_ZONE)) != 0)
			return retval;
	if ((type & RECORD) == RECORD)
		if ((retval = run_query_sqlite(config, base, RECORD)) != 0)
			return retval;
	if ((type & REV_RECORD) == REV_RECORD)
		if ((retval = run_query_sqlite(config, base, REV_RECORD)) != 0)
			return retval;
	return retval;
}

void
store_result_sqlite(sqlite3_stmt *state, dnsa_t *base, int type, unsigned int fields)
{
	switch(type) {
		case ZONE:
			if (fields != select_fields[ZONES])
				break;
			store_zone_sqlite(state, base);
			break;
		case REV_ZONE:
			if (fields != select_fields[REV_ZONES])
				break;
			store_rev_zone_sqlite(state, base);
			break;
		case RECORD:
			if (fields != select_fields[RECORDS])
				break;
			store_record_sqlite(state, base);
			break;
		case REV_RECORD:
			if (fields != select_fields[REV_RECORDS])
				break;
			store_rev_record_sqlite(state, base);
			break;
		default:
			fprintf(stderr, "Unknown type %d\n",  type);
			break;
	}
			
}

void
store_zone_sqlite(sqlite3_stmt *state, dnsa_t *base)
{
	int retval;
	zone_info_t *zone, *list;
	
	if (!(zone = malloc(sizeof(zone_info_t))))
		report_error(MALLOC_FAIL, "zone in store_zone_sqlite");
	init_zone_struct(zone);
	zone->id = (unsigned long int) sqlite3_column_int(state, 0);
	snprintf(zone->name, RBUFF_S, "%s", sqlite3_column_text(state, 1));
	snprintf(zone->pri_dns, RBUFF_S -1, "%s", sqlite3_column_text(state, 2));
	if ((retval = add_trailing_dot(zone->pri_dns)) != 0)
		fprintf(stderr, "Unable to add trailing dot to PRI_NS\n");
	snprintf(zone->sec_dns, RBUFF_S - 1, "%s", sqlite3_column_text(state, 3));
	if (strncmp(zone->sec_dns, "(null)", COMM_S) != 0)
		if ((retval = add_trailing_dot(zone->sec_dns)) != 0)
			fprintf(stderr, "Unable to add trailing dot to SEC_NS\n");
	zone->serial = (unsigned long int) sqlite3_column_int(state, 4);
	zone->refresh = (unsigned long int) sqlite3_column_int(state, 5);
	zone->retry = (unsigned long int) sqlite3_column_int(state, 6);
	zone->expire = (unsigned long int) sqlite3_column_int(state, 7);
	zone->ttl = (unsigned long int) sqlite3_column_int(state, 8);
	snprintf(zone->valid, RANGE_S, "%s", sqlite3_column_text(state, 9));
	zone->owner = (unsigned long int) sqlite3_column_int(state, 10);
	snprintf(zone->updated, RANGE_S, "%s", sqlite3_column_text(state, 11));
	list = base->zones;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = zone;
	} else {
		base->zones = zone;
	}
}

void
store_rev_zone_sqlite(sqlite3_stmt *state, dnsa_t *base)
{
	int retval;
	rev_zone_info_t *rev, *list;
	
	if (!(rev = malloc(sizeof(rev_zone_info_t))))
		report_error(MALLOC_FAIL, "rev in store_rev_zone_sqlite");
	init_rev_zone_struct(rev);
	rev->rev_zone_id = (unsigned long int) sqlite3_column_int(state, 0);
	snprintf(rev->net_range, RANGE_S, "%s", sqlite3_column_text(state, 1));
	rev->prefix = (unsigned long int) sqlite3_column_int(state, 2);
	snprintf(rev->net_start, RANGE_S, "%s", sqlite3_column_text(state, 3));
	snprintf(rev->net_finish, RANGE_S, "%s", sqlite3_column_text(state, 4));
	rev->start_ip = (unsigned long int) sqlite3_column_int(state, 5);
	rev->end_ip = (unsigned long int) sqlite3_column_int(state, 6);
	snprintf(rev->pri_dns, RBUFF_S -1, "%s", sqlite3_column_text(state, 7));
	if ((retval = add_trailing_dot(rev->pri_dns)) != 0)
		fprintf(stderr, "Unable to add trailing dot to PRI_NS\n");
	snprintf(rev->sec_dns, RBUFF_S -1, "%s", sqlite3_column_text(state, 8));
	if (strncmp(rev->sec_dns, "(null)", COMM_S) != 0)
		if ((retval = add_trailing_dot(rev->sec_dns)) != 0)
			fprintf(stderr, "Unable to add trailing dot to SEC_NS\n");
	rev->serial = (unsigned long int) sqlite3_column_int(state, 9);
	rev->refresh = (unsigned long int) sqlite3_column_int(state, 10);
	rev->retry = (unsigned long int) sqlite3_column_int(state, 11);
	rev->expire = (unsigned long int) sqlite3_column_int(state, 12);
	rev->ttl = (unsigned long int) sqlite3_column_int(state, 13);
	snprintf(rev->valid, RANGE_S, "%s", sqlite3_column_text(state, 14));
	rev->owner = (unsigned long int) sqlite3_column_int(state, 15);
	snprintf(rev->updated, RANGE_S, "%s", sqlite3_column_text(state, 16));
	list = base->rev_zones;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = rev;
	} else {
		base->rev_zones = rev;
	}
}

void
store_record_sqlite(sqlite3_stmt *state, dnsa_t *base)
{
	record_row_t *rec, *list;
	if (!(rec = malloc(sizeof(record_row_t))))
		report_error(MALLOC_FAIL, "rec in store_record_sqlite");
	init_record_struct(rec);
	rec->id = (unsigned long int) sqlite3_column_int(state, 0);
	rec->zone = (unsigned long int) sqlite3_column_int(state, 1);
	snprintf(rec->host, RBUFF_S, "%s", sqlite3_column_text(state, 2));
	snprintf(rec->type, RANGE_S, "%s", sqlite3_column_text(state, 3));
	rec->pri = (unsigned long int) sqlite3_column_int(state, 4);
	snprintf(rec->dest, RBUFF_S, "%s", sqlite3_column_text(state, 5));
	snprintf(rec->valid, RANGE_S, "%s", sqlite3_column_text(state, 6));
	list = base->records;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = rec;
	} else {
		base->records = rec;
	}
}

void
store_rev_record_sqlite(sqlite3_stmt *state, dnsa_t *base)
{
	rev_record_row_t *rev, *list;

	if (!(rev = malloc(sizeof(rev_record_row_t))))
		report_error(MALLOC_FAIL, "rev in store_rev_record_sqlite");
	init_rev_record_struct(rev);
	rev->record_id = (unsigned long int) sqlite3_column_int(state, 0);
	rev->rev_zone = (unsigned long int) sqlite3_column_int(state, 1);
	snprintf(rev->host, RBUFF_S, "%s", sqlite3_column_text(state, 2));
	snprintf(rev->dest, RBUFF_S, "%s", sqlite3_column_text(state, 3));
	snprintf(rev->valid, RANGE_S, "%s", sqlite3_column_text(state, 4));
	list = base->rev_records;
	if (list) {
		while (list->next)
			list = list->next;
		list->next = rev;
	} else {
		base->rev_records = rev;
	}
}

int
run_search_sqlite(dnsa_config_t *config, dnsa_t *base, int type)
{
	const char *query, *file;
	int retval;
	unsigned long int result;
	size_t fields, args;
	void *input, *output;
	sqlite3 *dnsa;
	sqlite3_stmt *state;
	
	retval = 0;
	query = sql_search[type];
	file = config->file;
	if ((retval = sqlite3_open_v2(file, &dnsa, SQLITE_OPEN_READONLY, NULL)) > 0) {
		report_error(CANNOT_OPEN_FILE, file);
	}
	if ((retval = sqlite3_prepare_v2(dnsa, query, BUFF_S, &state, NULL)) > 0) {
		retval = sqlite3_close(dnsa);
		report_error(SQLITE_STATEMENT_FAILED, "run_search_sqlite");
	}
/*
   As in the MySQL function we assume that we are sending text and recieving
   numerical data. Searching on name for ID is ok for this
*/
	get_search(type, &fields, &args, &input, &output, base);
	if ((retval = sqlite3_bind_text(state, 1, input, (int)strlen(input), SQLITE_STATIC)) > 0) {
		retval = sqlite3_close(dnsa);
		report_error(SQLITE_STATEMENT_FAILED, "run_search_sqlite");
	}
	if ((retval = sqlite3_step(state)) == SQLITE_ROW) {
		result = (unsigned long int)sqlite3_column_int(state, 0);
		switch(type) {
			case ZONE_ID_ON_NAME:
				base->zones->id = result;
				break;
			case REV_ZONE_ID_ON_NET_RANGE:
				base->rev_zones->rev_zone_id = result;
				break;
		}
	}
	retval = sqlite3_finalize(state);
	retval = sqlite3_close(dnsa);
	return retval;
}

int
run_insert_sqlite(dnsa_config_t *config, dnsa_t *base, int type)
{
	const char *query, *file;
	int retval;
	sqlite3 *dnsa;
	sqlite3_stmt *state;

	retval = 0;
	query = sql_insert[type];
	file = config->file;
	if ((retval = sqlite3_open_v2(file, &dnsa, SQLITE_OPEN_READWRITE, NULL)) > 0) {
		report_error(CANNOT_OPEN_FILE, file);
	}
	if ((retval = sqlite3_prepare_v2(dnsa, query, BUFF_S, &state, NULL)) > 0) {
		retval = sqlite3_close(dnsa);
		report_error(SQLITE_STATEMENT_FAILED, "run_search_sqlite");
	}
	if ((retval = setup_insert_sqlite_bind(state, base, type)) != 0) {
		printf("Error binding result! %d\n", retval);
		sqlite3_close(dnsa);
		return retval;
	}
	if ((retval = sqlite3_step(state)) != SQLITE_DONE) {
		printf("Recieved error: %s\n", sqlite3_errmsg(dnsa));
		retval = sqlite3_finalize(state);
		retval = sqlite3_close(dnsa);
		retval = SQLITE_INSERT_FAILED;
		return retval;
	}
	retval = sqlite3_finalize(state);
	retval = sqlite3_close(dnsa);
	return retval;
}

int
setup_insert_sqlite_bind(sqlite3_stmt *state, dnsa_t *base, int type)
{
	int retval;
	if (type == RECORDS) {
		retval = setup_bind_sqlite_records(state, base->records);
	} else if (type == ZONES) {
		retval = setup_bind_sqlite_zones(state, base->zones);
	} else {
		retval = WRONG_TYPE;
	}
	return retval;
}

int
setup_bind_sqlite_records(sqlite3_stmt *state, record_row_t *record)
{
	int retval;

	retval = 0;
	if ((retval = sqlite3_bind_int(state, 1, (int)record->zone)) > 0) {
		fprintf(stderr, "Cannot bind zone %lu\n", record->zone);
		return retval;
	}
	if ((retval = sqlite3_bind_text(
state, 2, record->host, (int)strlen(record->host), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind host %s\n", record->host);
		return retval;
	}
	if ((retval = sqlite3_bind_text(
state, 3, record->type, (int)strlen(record->type), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind type %s\n", record->type);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 4, (int)record->pri)) > 0) {
		fprintf(stderr, "Cannot bind pri %lu\n", record->pri);
		return retval;
	}
	if ((retval = sqlite3_bind_text(
state, 5, record->dest, (int)strlen(record->dest), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind destination %s\n", record->dest);
		return retval;
	}
	return retval;
}

int
setup_bind_sqlite_zones(sqlite3_stmt *state, zone_info_t *zone)
{
	int retval;

	retval = 0;
	if ((retval = sqlite3_bind_text(
state, 1, zone->name, (int)strlen(zone->name), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind name %s\n", zone->name);
		return retval;
	}
	if ((retval = sqlite3_bind_text(
state, 2, zone->pri_dns, (int)strlen(zone->pri_dns), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind primary dns %s\n", zone->pri_dns);
		return retval;
	}
	if ((retval = sqlite3_bind_text(
state, 3, zone->sec_dns, (int)strlen(zone->sec_dns), SQLITE_STATIC)) > 0) {
		fprintf(stderr, "Cannot bind primary dns %s\n", zone->sec_dns);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 4, (int)zone->serial)) > 0) {
		fprintf(stderr, "Cannot bind serial %lu\n", zone->serial);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 5, (int)zone->refresh)) > 0) {
		fprintf(stderr, "Cannot bind refresh %lu\n", zone->refresh);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 6, (int)zone->retry)) > 0) {
		fprintf(stderr, "Cannot bind retry %lu\n", zone->retry);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 7, (int)zone->expire)) > 0) {
		fprintf(stderr, "Cannot bind serial %lu\n", zone->expire);
		return retval;
	}
	if ((retval = sqlite3_bind_int(state, 8, (int)zone->ttl)) > 0) {
		fprintf(stderr, "Cannot bind serial %lu\n", zone->ttl);
		return retval;
	}
	return retval;
}
#endif /* HAVE_SQLITE3 */
