/*
 * $Id$
 *
 * Copyright (c) 2004-2005 Vyacheslav Frolov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * $Log$
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_PRECOMP_H_
#define _C0C_PRECOMP_H_

#pragma warning(push, 3)

#include <ntddk.h>
//#include <wdm.h>
#include <ntddser.h>

#pragma warning(pop)

#include <stdio.h>
#include <stdarg.h>

#include "com0com.h"
#include "trace.h"
#include "syslog.h"

#pragma warning(disable:4514) // unreferenced inline function has been removed

#endif /* _C0C_PRECOMP_H_ */
