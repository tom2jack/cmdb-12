/*
 *
 *  cmdb: Configuration Management Database
 *  Copyright (C) 2012 - 2014  Iain M Conochie <iain-AT-thargoid.co.uk>
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
 *  cmdbd.c
 *
 *  Contains main() function for cmdbd program
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ailsacmdb.h>
#include "cmdb.h"

int
main(int agrc, char *argv[])
{
	int retval = 0;
	show_ailsacmdb_version();

	return retval;
}
