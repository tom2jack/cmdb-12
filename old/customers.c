/* 
 *
 *  cmdb: Configuration Management Database
 *  Copyright (C) 2012 - 2020  Iain M Conochie <iain-AT-thargoid.co.uk>
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
 *  customers.c
 * 
 *  Functions relating to customers in the database for the cmdb program
 * 
 *  (C) Iain M Conochie 2012 - 2013
 * 
 */
#include <config.h>
#include <configmake.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "cmdb.h"
#include "cmdb_cmdb.h"
#include "base_sql.h"
#include "cmdb_base_sql.h"


void
display_customer_info(char *name, char *coid, cmdb_config_s *config)
{
	int retval, i;
	cmdb_customer_s *list;
	cmdb_s *cmdb;
	retval = i = 0;

	if (!(cmdb = malloc(sizeof(cmdb_s))))
		report_error(MALLOC_FAIL, "cmdb_s in display_customer_info");
	cmdb_init_struct(cmdb);

	cmdb->customer = NULL;
	if ((retval = cmdb_run_multiple_query(config, cmdb,
CUSTOMER | CONTACT | SERVICE | SERVER)) != 0) {
		cmdb_clean_list(cmdb);
		return;
	}
	list = cmdb->customer;
	if (name) {
		while(list) {
			if ((strncmp(list->name, name, MAC_S) == 0)) {
				print_customer_details(list, cmdb);
				list = list->next;
				i++;
			} else {
				list = list->next;
			}
		}
	} else if (coid) {
		while (list) {
			if ((strncmp(list->coid, coid, CONF_S) == 0)) {
				print_customer_details(list, cmdb);
				list = list->next;
				i++;
			} else {
				list = list->next;
			}
		}
	} else {
		fprintf(stderr, "No customer name or coid specified!\n");
		i++;
	}
	cmdb_clean_list(cmdb);
	if (i == 0)
		printf("No customer found\n");
}

void
display_all_customers(cmdb_config_s *config)
{
	int retval;
	cmdb_customer_s *list;
	cmdb_s *cmdb;
	size_t len;
	retval = 0;

	if (!(cmdb = malloc(sizeof(cmdb_s))))
		report_error(MALLOC_FAIL, "cmdb_s in display_customer_info");
	cmdb_init_struct(cmdb);

	cmdb->customer = NULL;
	if ((retval = cmdb_run_query(config, cmdb, CUSTOMER)) != 0) {
		cmdb_clean_list(cmdb);
		return;
	}
	list = cmdb->customer;
	while(list) {
		if ((len = strlen(list->coid)) < 8)
			printf("%s\t %s\n", list->coid, list->name);
		else
			printf("%s %s\n", list->coid, list->name);
		list = list->next;
	}
	cmdb_clean_list(cmdb);
	return;
}

int
add_customer_to_database(cmdb_config_s *config, cmdb_s *cmdb)
{
	int retval = 0;
	dbdata_s data;

	memset(&data, 0, sizeof(dbdata_s));
	snprintf(data.args.text, HOST_S, "%s", cmdb->customer->name);
	if ((retval = cmdb_run_search(config, &data, CUST_ID_ON_NAME)) != 0) {
		fprintf(stderr, "Customer name %s exists in database\n", cmdb->customer->name);
		return CUSTOMER_EXISTS;
	}
	snprintf(data.args.text, RANGE_S, "%s", cmdb->customer->coid);
	if ((retval = cmdb_run_search(config, &data, CUST_ID_ON_COID)) != 0) {
		fprintf(stderr, "Customer COID %s exists in database\n", cmdb->customer->coid);
		return COID_EXISTS;
	}
	cmdb->customer->cuser = cmdb->customer->muser = (unsigned long int)getuid();
	retval = cmdb_run_insert(config, cmdb, CUSTOMERS);
	return retval;
}

int
remove_customer_from_database(cmdb_config_s *config, cmdb_comm_line_s *cm)
{
	int retval = 0, type = 0;
	dbdata_s data;
	if (strncmp(cm->name, "NULL", COMM_S) != 0) {
		snprintf(data.args.text, CONF_S, "%s", cm->name);
		type = CUST_ID_ON_NAME;
	} else if (strncmp(cm->id, "NULL", COMM_S) != 0) {
		snprintf(data.args.text, CONF_S, "%s", cm->id);
		type = CUST_ID_ON_COID;
	} else
		return NO_NAME;
	if ((retval = cmdb_run_search(config, &data, type)) == 0) {
		fprintf(stderr, "No customers found\n");
		return CUSTOMER_NOT_FOUND;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple customers found\n");
		return MULTIPLE_CUSTOMERS;
	} else
		data.args.number = data.fields.number;
	if ((retval = cmdb_run_delete(config, &data, CUSTOMERS)) == 0) {
		fprintf(stderr, "Customer not removed from DB\n");
		return CUSTOMER_NOT_FOUND;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple customers removed from DB\n");
		return MULTIPLE_CUSTOMERS;
	} else
		printf("Customer removed from database\n");
	return NONE;
}

int
remove_contact_from_database(cmdb_config_s *config, cmdb_comm_line_s *cm)
{
	const unsigned int a = cmdb_search_args[CONTACT_ID_ON_COID_NAME];
	const unsigned int f = cmdb_search_fields[CONTACT_ID_ON_COID_NAME];
	int retval = NONE;
	unsigned int max = cmdb_get_max(a, f);
	dbdata_s *data;

	if (!(cm->name) || !(cm->id))
		return NO_CONTACT_INFO;
	init_multi_dbdata_struct(&data, max);
	snprintf(data->args.text, CONF_S, "%s", cm->name);
	snprintf(data->next->args.text, CONF_S, "%s", cm->id);
	if ((retval  = cmdb_run_search(config, data, CONTACT_ID_ON_COID_NAME)) == 0) {
		fprintf(stderr, "No contact found\n");
		return NO_CONTACT;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple contacts found\n");
		return MULTI_CONTACT;
	} else
		data->args.number = data->fields.number;
	if ((retval = cmdb_run_delete(config, data, CONTACTS)) == 0) {
		fprintf(stderr, "No contact removed\n");
		return NO_CONTACT;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple contacts removed\n");
		return MULTI_CONTACT;
	} else 
		printf("Contact removed\n");
	return NONE;
}

int
remove_service_from_database(cmdb_config_s *config, cmdb_comm_line_s *cm)
{
	const unsigned int a = cmdb_search_args[SERVER_ID_ON_NAME];
	const unsigned int f = cmdb_search_fields[SERVER_ID_ON_NAME];
	int retval = NONE, type = NONE;
	unsigned int max = cmdb_get_max(a, f);
	unsigned long int id = NONE;
	dbdata_s *data, *list;

	init_multi_dbdata_struct(&data, max);
	if (cm->name) {
		snprintf(data->args.text, HOST_S, "%s", cm->name);
		if ((retval = cmdb_run_search(config, data, SERVER_ID_ON_NAME)) == 0) {
			clean_dbdata_struct(data);
			return SERVER_ID_NOT_FOUND;
		} else if (retval > 1) {
			clean_dbdata_struct(data);
			return MULTIPLE_SERVER_IDS;
		} else {
			id = data->fields.number;
			type = SERVER;
		}
	} else if ((cm->coid) || (cm->id)) {
		if (cm->coid)
			snprintf(data->args.text, RANGE_S, "%s", cm->coid);
		else if (cm->id)
			snprintf(data->args.text, RANGE_S, "%s", cm->id);
		else
			return NO_COID;
		if ((retval = cmdb_run_search(config, data, CUST_ID_ON_COID)) == 0) {
			clean_dbdata_struct(data);
			return CUSTOMER_NOT_FOUND;
		} else if (retval > 1) {
			clean_dbdata_struct(data);
			return MULTIPLE_CUSTOMERS;
		} else {
			id = data->fields.number;
			type = CUSTOMER;
		}
	} else {
		return NO_NAME_COID;
	}
	clean_dbdata_struct(data);
	init_multi_dbdata_struct(&data, max);
	if (cm->url) {
		if (cm->service) {
			snprintf(data->args.text, RBUFF_S, "%s", cm->url);
			snprintf(data->next->args.text, RANGE_S, "%s", cm->service);
			retval = cmdb_run_search(config, data, SERVICE_ID_ON_URL_SERVICE);
		} else {
			snprintf(data->args.text, HOST_S, "%s", cm->url);
			retval = cmdb_run_search(config, data, SERVICE_ID_ON_URL);
		}
		if (retval == 0) {
			fprintf(stderr, "No services found\n");
			clean_dbdata_struct(data);
			return NO_SERVICES;
		} else if (retval > 1) {
			fprintf(stderr, "Multiple services returned. Deleting first one\n");
		}
		data->args.number = data->fields.number;
		if ((retval = cmdb_run_delete(config, data, SERVICES)) == 0) {
			clean_dbdata_struct(data);
			fprintf(stderr, "No services deleted\n");
			return NO_SERVICES;
		} else if (retval > 1) {
			clean_dbdata_struct(data);
			fprintf(stderr, "Multiple services deleted\n");
			return MULTI_SERVICES;
		} else {
			printf("Service deleted\n");
			retval = NONE;
		}
	} else if (cm->service) {
		data->args.number = id;
		snprintf(data->next->args.text, RANGE_S, "%s", cm->service);
		if (type == SERVER)
			retval = cmdb_run_search(config, data, SERVICE_ID_ON_SERVER_ID_SERVICE);
		else if (type == CUSTOMER)
			retval = cmdb_run_search(config, data, SERVICE_ID_ON_CUST_ID_SERVICE);
		printf("%d services to delete\n", retval);
		list = data;
		while (list) {
			list->args.number = list->fields.number;
			retval = cmdb_run_delete(config, list, SERVICES);
			list = list->next;
		}
		retval = NONE;
	} else
		retval = NO_SERVICE_URL;
	clean_dbdata_struct(data);
	return retval;
}

int
add_contact_to_database(cmdb_config_s *config, cmdb_s *cmdb)
{
	int retval = NONE;
	cmdb_contact_s *cont;
	dbdata_s *data = NULL;

	cont = cmdb->contact;
	init_multi_dbdata_struct(&data, cmdb_search_args[CUST_ID_ON_COID]);
	snprintf(data->args.text, RBUFF_S, "%s", cmdb->customer->coid);
	if ((retval = cmdb_run_search(config, data, CUST_ID_ON_COID)) < 1) {
		fprintf(stderr, "Unable to retrieve cust_id for COID %s\n",
		 cmdb->customer->coid);
		retval = 1;
	} else if (retval > 1) {
		fprintf(stderr, "More than one cust_id returned for %s",
		 data->args.text);
		retval = 1;
	} else {
		cont->cust_id = data->fields.number;
		cont->cuser = cont->muser = (unsigned long int)getuid();
		cmdb->customer->muser = cont->muser;
		cmdb->customer->cust_id = cont->cust_id;
		retval = cmdb_run_insert(config, cmdb, CONTACTS);
		set_customer_updated(config, cmdb);
	}
	clean_dbdata_struct(data);
	return retval;
}

int
add_service_to_database(cmdb_config_s *config, cmdb_s *cmdb)
{
	int retval = NONE;
	unsigned long int ids[2];
	dbdata_s *data = NULL;

	init_multi_dbdata_struct(&data, cmdb_search_args[CUST_ID_ON_COID]);
	snprintf(data->args.text, RBUFF_S, "%s", cmdb->customer->coid);
	if ((retval = cmdb_run_search(config, data, CUST_ID_ON_COID)) < 1) {
		fprintf(stderr, "Unable to retrieve cust_id for COID %s\n",
		 cmdb->customer->coid);
		return 1;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple cust_id's returned for %s\n",
		 cmdb->customer->coid);
		return 1;
	} else  {
		cmdb->customer->cust_id = data->fields.number;
/* FIXME: We should not have to clean here - this is the db search routine */
		clean_dbdata_struct(data->next);
	}
	init_dbdata_struct(data);
	snprintf(data->args.text, RBUFF_S, "%s", cmdb->server->name);
	if ((retval = cmdb_run_search(config, data, SERVER_ID_ON_NAME)) < 1) {
		fprintf(stderr, "Unable to retrieve server_id for server %s\n",
		 cmdb->server->name);
		return 1;
	} else if (retval > 1) {
		fprintf(stderr, "Multiple server_id's returned for %s\n",
		 cmdb->server->name);
		return 1;
	} else {
		cmdb->server->server_id = data->fields.number;
/* FIXME: We should not have to clean here - this is the db search routine */
		clean_dbdata_struct(data->next);
	}
	init_dbdata_struct(data);
	if ((retval = strncmp(cmdb->servicetype->service, "NULL", COMM_S)) != 0) {
		snprintf(data->args.text, RBUFF_S, "%s", cmdb->servicetype->service);
		if ((retval = cmdb_run_search(config, data, SERV_TYPE_ID_ON_SERVICE)) < 1) {
			fprintf(stderr, "Unable to retrieve service_type_id for %s\n",
			 cmdb->servicetype->service);
			return 1;
		} else if (retval > 1) {
			fprintf(stderr, "Multiple service_type_id's returned for %s\n",
			cmdb->servicetype->service);
			return 1;
		} else {
			cmdb->service->service_type_id = data->fields.number;
		}
	} else {
		fprintf(stderr, "No service provided??\n");
		return 1;
	}
	clean_dbdata_struct(data);
	cmdb->service->server_id = cmdb->server->server_id;
	cmdb->service->cust_id = cmdb->customer->cust_id;
	cmdb->service->cuser = cmdb->service->muser = (unsigned long int)getuid();
	cmdb->customer->muser = cmdb->server->muser = cmdb->service->muser;
	ids[0] = cmdb->server->muser;
	ids[1] = cmdb->server->server_id;
	retval = cmdb_run_insert(config, cmdb, SERVICES);
	set_server_updated(config, ids);
	set_customer_updated(config, cmdb);
	return retval;
}
void
display_service_types(cmdb_config_s *config)
{
	int retval;
	cmdb_service_type_s *list;
	size_t len;
	cmdb_s *cmdb;

	retval = 0;

	if (!(cmdb = malloc(sizeof(cmdb_s))))
		report_error(MALLOC_FAIL, "cmdb_s in display_service_types");
	cmdb_init_struct(cmdb);

	cmdb->servicetype = NULL;
	if ((retval = cmdb_run_query(config, cmdb, SERVICE_TYPE)) != 0) {
		cmdb_clean_list(cmdb);
		return;
	}
	list = cmdb->servicetype;
	printf("ID\tService\t\tDescription\n");
	while(list) {
		if ((len = strlen(list->service)) < 8)
			printf("%lu\t%s\t\t%s\n",
			list->service_id, list->service, list->detail);
		else
			printf("%lu\t%s\t%s\n",
			list->service_id, list->service, list->detail);
		list = list->next;
	}
	cmdb_clean_list(cmdb);
	return;
}

void
display_customer_services(cmdb_config_s *config, char *coid)
{
	int retval;
	cmdb_s *cmdb;
	cmdb_customer_s *cust;
	cmdb_service_s *service;

	if (!(cmdb = malloc(sizeof(cmdb_s))))
		report_error(MALLOC_FAIL, "cmdb in display_customer_services");

	cmdb_init_struct(cmdb);
	if ((retval = cmdb_run_multiple_query(config, cmdb, CUSTOMER | SERVICE)) != 0) {
		cmdb_clean_list(cmdb);
		printf("Query for customer %s services failed\n", coid);
		return;
	}
	cust = cmdb->customer;
	service = cmdb->service;
	printf("Customer %s\n", coid);
	while (cust) {
		if ((strncmp(cust->coid, coid, COMM_S) == 0)) {
			retval = print_services(service, cust->cust_id, CUSTOMER);
			cust = cust->next;
		} else {
			cust = cust->next;
		}
	}
	if (retval == 0)
		printf("No services\n");
	cmdb_clean_list(cmdb);
}

void
display_customer_contacts(cmdb_config_s *config, char *coid)
{
	int retval, i;
	cmdb_s *cmdb;
	cmdb_customer_s *cust;
	cmdb_contact_s *contact;

	if (!(cmdb = malloc(sizeof(cmdb_s))))
		report_error(MALLOC_FAIL, "cmdb in display_customer_contacts");

	i = 0;
	cmdb_init_struct(cmdb);
	if ((retval = cmdb_run_multiple_query(config, cmdb, CUSTOMER | CONTACT)) != 0) {
		cmdb_clean_list(cmdb);
		printf("Query for customer %s contacts failed\n", coid);
		return;
	}
	cust = cmdb->customer;
	contact = cmdb->contact;
	while (cust) {
		if (strncmp(cust->coid, coid, COMM_S) == 0) {
			if (i == 0) {
				printf("Customer %s\n", coid);
			}
			i++;
			retval = print_customer_contacts(contact, cust->cust_id);
			cust = cust->next;
		} else {
			cust = cust->next;
		}
	}
	if (retval == 0)
		printf("No contacts\n");
	cmdb_clean_list(cmdb);
}

void
print_customer_details(cmdb_customer_s *cust, cmdb_s *cmdb)
{
	char *uname, *crtime;
	int city = NONE, addr = NONE, post = NONE;
	struct passwd *user;
	time_t cmtime;
	uid_t uid;

	addr = strncmp(cust->address, "NULL", COMM_S);
	city = strncmp(cust->city, "NULL", COMM_S);
	post = strncmp(cust->postcode, "NULL", COMM_S);
	printf("%s: Coid %s\n", cust->name, cust->coid);
	if ((addr != 0) || (city != 0) || (post != 0)) {
		printf("Address:\n");
		if (strncmp(cust->address, "NULL", COMM_S) != 0)
			printf("%s\n", cust->address);
		else
			printf("No Address for customer\n");
		if ((city != 0) && (post != 0))
			printf("%s, %s\n", cust->city, cust->postcode);
		else if (city != 0)
			printf("%s\n", cust->city);
		else
			printf("%s\n", cust->postcode);
	} else {
		printf("No address details for customer\n");
	}
	uid = (uid_t)cust->cuser;
	user = getpwuid(uid);
	uname = user->pw_name;
        cmtime = (time_t)cust->ctime;
        crtime = ctime(&cmtime);
	printf("Create by user %s at %s", uname, crtime);
	uid = (uid_t)cust->muser;
	user = getpwuid(uid);
	uname = user->pw_name;
	cmtime = (time_t)cust->mtime;
	crtime = ctime(&cmtime);
	printf("Modified by user %s at %s", uname, crtime);
	if ((print_customer_contacts(cmdb->contact, cust->cust_id)) == 0)
		printf("\nCustomer has no associated contacts\n");
	if ((print_customer_servers(cmdb->server, cust->cust_id)) == 0)
		printf("\nCustomer has no associated servers\n");
	if ((print_services(cmdb->service, cust->cust_id, CUSTOMER)) == 0)
		printf("\nCustomer has no associated services\n");
}

int
print_customer_contacts(cmdb_contact_s *contacts, unsigned long int cust_id)
{
	int i = 0;
	cmdb_contact_s *list;
	list = contacts;
	while (list) {
		if (list->cust_id == cust_id) {
			i++;
			if (i == 1)
				printf("\nContacts:\n");
			printf("%s, %s, %s\n", list->name, list->phone, list->email);
		}
		list = list->next;
	}
	return i;
}

int
print_customer_servers(cmdb_server_s *server, unsigned long int cust_id)
{
	int i = 0;
	cmdb_server_s *list = server;
	while (list) {
		if (list->cust_id == cust_id) {
			i++;
			if (i == 1)
				printf("\nServers:\n");
			printf("%s\n", list->name);
		}
		list = list->next;
	}
	return i;
}

int
get_customer(cmdb_config_s *config, cmdb_s *cmdb, char *coid)
{
	int retval;
	cmdb_customer_s *cust, *next;

	if ((retval = cmdb_run_query(config, cmdb, CUSTOMER)) != 0)
		return retval;

	cust = cmdb->customer;
	next = cust->next;
	while (cust) {
		if (strncmp(coid, cust->coid, RANGE_S) != 0) {
			free(cust);
			cust = next;
			if (cust)
				next = cust->next;
		} else {
			break;
		}
	}
	if (cust) {
		cmdb->customer = cust;
		cust = cust->next;
		while (cust) {
			next = cust->next;
			free(cust);
			cust = next;
		}
	} else {
		cmdb->customer = NULL;
		return CUSTOMER_NOT_FOUND;
	}
	cmdb->customer->next = NULL;
	return 0;
}

unsigned long int
cmdb_get_customer_id(cmdb_config_s *config, char *coid)
{
	int retval = 0;
	unsigned long int cust_id = 0;
	dbdata_s *data;

	if (!coid)
		return cust_id;
	init_multi_dbdata_struct(&data, cmdb_search_args[CUST_ID_ON_COID]);
	snprintf(data->args.text, RBUFF_S, "%s", coid);
	if ((retval = cmdb_run_search(config, data, CUST_ID_ON_COID)) == 0)
		fprintf(stderr, "Customer COID %s not found\n", coid);
	else if (retval > 1)
		fprintf(stderr, "Multiple results for COID %s\n", coid);
	else
		cust_id = data->fields.number;
	clean_dbdata_struct(data);
	return cust_id;
}

void
set_customer_updated(cmdb_config_s *config, cmdb_s *cmdb)
{
	int retval = NONE;
	unsigned int num = cmdb_update_args[UP_CUST_MUSER];
	dbdata_s *data = NULL;

	init_multi_dbdata_struct(&data, num);
	data->args.number = cmdb->customer->muser;
	data->next->args.number = cmdb->customer->cust_id;
	if ((retval = cmdb_run_update(config, data, UP_CUST_MUSER)) < 1) 
		fprintf(stderr, "Unable to set customer updated\n");
	else if (retval > 1)
		fprintf(stderr, "Multiple customers updated???\n");
	else
		printf("Customer set to updated\n");
	clean_dbdata_struct(data);
}

