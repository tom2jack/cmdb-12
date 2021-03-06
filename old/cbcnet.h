/*
 *  cbc: Create Build Config
 *  (C) 2014 Iain M Conochie <iain-AT-thargoid.co.uk>
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
 *  cbcnet.h: Contains function definitions for cbcnet.c
 *
 */

#ifndef __CBC_NET_H
# define __CBC_NET_H
# include <config.h>
# include "ifaddrs.h"
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <ailsacmdb.h>
# define MAXDATASIZE     1048576    // Maximum buffer length

extern const char *fed_tld;
extern const char *fed_boot;
extern const char *deb_i386_boot;
extern const char *deb_amd64_boot;
extern const char *ubu_i386_boot;
extern const char *ubu_amd64_boot;

void
fill_addrtcp(struct addrinfo *c);

int
get_net_list_for_dhcp(cbc_build_domain_s *bd, cbc_dhcp_s **dh);

void
get_iface_info(cbc_iface_s **info);

int
fill_iface_info(struct ifaddrs *list, cbc_iface_s *info);

int
get_dhcp_server_info(cbc_build_domain_s *bd, cbc_dhcp_s **dh, cbc_iface_s *i);

void
insert_into_dhcp_list(cbc_dhcp_s **list);

void
remove_from_dhcp_list(cbc_dhcp_s **list);

int
fill_dhcp_server(cbc_build_domain_s *bd, cbc_iface_s *i, cbc_dhcp_s *dh);

int
decode_http_header(FILE *rx, unsigned long int *len);

int
cbc_get_boot_files(ailsa_cmdb_s *cbc, char *os, char *ver, char *arch, char *vail);

#endif /* __CBC_NET_H */

