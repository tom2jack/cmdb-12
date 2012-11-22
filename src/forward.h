/* forward.h: Function and data definitions for forward zones */

#include <mysql/mysql.h>
#include "cmdb.h"
#include "cmdb_dnsa.h"
#ifndef	__WRITE_ZONE_H__
#define __WRITE_ZONE_H__

typedef struct zone_info_t { /* Hold DNS zone */
	int id;
	char name[RBUFF_S];
	char pri_dns[RBUFF_S];
	char sec_dns[RBUFF_S];
	int serial;
	int refresh;
	int retry;
	int expire;
	int ttl;
	char valid[RBUFF_S];
	int owner;
	char updated[RBUFF_S];
} zone_info_t;

typedef struct record_row_t { /* Hold dns record */
	int id;
	int zone;
	char host[RBUFF_S];
	char type[RBUFF_S];
	int pri;
	char dest[RBUFF_S];
	char valid[RBUFF_S];
} record_row_t;
/* Return struct containing the DNS zone data */
zone_info_t
fill_zone_data(MYSQL_ROW my_row);
/* Return struct containing DNS Record data */
record_row_t
fill_record_data(MYSQL_ROW my_row);
/* Create the header of the DNS zone in a string */
void
create_zone_header(char *output, zone_info_t);
/* Add the MX records to the header string */
void
add_mx_to_header(char *output, MYSQL_ROW results);
/* Add the A records for the NS servers to the zonefile */
void
add_ns_A_records_to_header(zone_info_t *zi, dnsa_config_t *dc, char *out);
/* Add the A records for the MX servers to the zonefile; return no added */
int
add_MX_A_records_to_header(zone_info_t *zi, dnsa_config_t *dc, char *out);
/* Add the individual DNS Records to the zonefile string; return size */
size_t
add_records(record_row_t, char *output, size_t offset);
/* Check forward zone file for errors */
void
check_fwd_zone(char *filename, char *domain, dnsa_config_t *dc);
/* Write fwd zonefile to filesystem */
void
write_fwd_zonefile(char *filename, char *output);
/* Write out the forward zone file */
int
wzf (char *domain, dnsa_config_t *dc);
/* Display zone data */
int
dzf (char *domain, dnsa_config_t *dc);
/* Write the dnsa forward config file */
int
wcf(dnsa_config_t *dc);
/* List the forward zones in the database */
int
list_zones (dnsa_config_t *dc);
#endif
