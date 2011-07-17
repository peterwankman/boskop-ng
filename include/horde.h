/* 
 * horde.h -- horde.c header
 * 
 * Copyright (C) 2009  Martin Wolters et al.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to 
 * the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 * 
 */

#ifndef HORDE_H_
#define HORDE_H_

#define HORDE_ALL    1
#define HORDE_ONE    2
#define HORDE_BATCH  3

#define HORDE_SENDA(s)  irc_query("horde", HORDE_ALL, s)
#define HORDE_SENDO(s)  irc_query("horde", HORDE_ONE, s)
#define HORDE_SENDB(l)  irc_query("horde", HORDE_BATCH, l)

#endif

