/*
 * $Id$
 *
 * Copyright (c) 2005-2008 Vyacheslav Frolov
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
 * Revision 1.1  2008/12/02 16:07:37  vfrolov
 * Initial revision
 *
 */

#ifndef _HALT_H_
#define _HALT_H_

#if DBG

#define HALT_UNLESS3(exp, p1, p2, p3) \
  if (!(exp)) \
    KeBugCheckEx(0xDEADC0CD, (FILE_ID << 16) + __LINE__, p1, p2, p3)

#define HALT_UNLESS2(exp, p1, p2) HALT_UNLESS3(exp, p1, p2, 0)
#define HALT_UNLESS1(exp, p1)     HALT_UNLESS3(exp, p1, 0, 0)
#define HALT_UNLESS(exp)          HALT_UNLESS3(exp, 0, 0, 0)

#else /* DBG */

#define HALT_UNLESS3(exp, p1, p2, p3)
#define HALT_UNLESS2(exp, p1, p2)
#define HALT_UNLESS1(exp, p1)
#define HALT_UNLESS(exp)

#endif /* DBG */

#endif /* _HALT_H_ */
