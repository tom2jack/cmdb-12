 /*
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
 *  cmdb_base_sql.h
 *
 *  This file contains the SQL statement list for the cmdb suite of programs
 */

#ifndef __CMDB_BASE_SQL_H
#define __CMDB_BASE_SQL_H
#include "../config.h"

int
run_query(cmdb_config_t *config, cmdb_t *base, int type);

# ifdef HAVE_MYSQL
#include <mysql.h>

void
cmdb_mysql_init(cmdb_config_t *dc, MYSQL *cmdb_mysql);
int
run_query_mysql(cmdb_config_t *config, cmdb_t *base, int type);
int
get_query_mysql(int type, const char **query, unsigned int *fields);
void
store_result_mysql(MYSQL_ROW row, cmdb_t *base, int type, unsigned int *fields);
void
store_server_mysql(MYSQL_ROW row, cmdb_t *base);

# endif /* HAVE_MYSQL */

# ifdef HAVE_SQLITE3

int
run_query_sqlite(cmdb_config_t *config, cmdb_t *base, int type);

# endif /* HAVE_SQLITE3 */

#endif /* __CMDB_BASE_SQL_H */

