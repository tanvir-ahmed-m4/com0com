/*
 * cncext.h
 *
 * Copyright (c) 2008 Vyacheslav Frolov
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
 */

#ifndef _INCLUDE_C0C_CNCEXT_H_
#define _INCLUDE_C0C_CNCEXT_H_

#define C0CE_SIGNATURE              "c0c"
#define C0CE_SIGNATURE_SIZE         (sizeof(UCHAR)*4)

/**************************************************************************
  Extended IOCTL_SERIAL_LSRMST_INSERT allows to monitor and get the paired
  port settings as insertion to the data stream
 **************************************************************************/

/*
  Following this value is the baud rate (ULONG) of paired port
*/
#define C0CE_INSERT_RBR             16

/*
  Following this value is the byte size (UCHAR), parity (UCHAR)
  and stop bits (UCHAR) of paired port
*/
#define C0CE_INSERT_RLC             17

/*
  Options (ULONG)
*/

#define C0CE_INSERT_IOCTL_CAPS      0xFFFFFFFF  /* IOCTL returns bitmap of all possible options */

#define C0CE_INSERT_IOCTL_GET       0x01000000  /* IOCTL returns current values of enabled insertions */
#define C0CE_INSERT_IOCTL_RXCLEAR   0x02000000  /* IOCTL clears the driver's input buffer */

#define C0CE_INSERT_ENABLE_LSR      0x00000001  /* enable standard SERIAL_LSRMST_LSR_[NO]DATA insertion */
#define C0CE_INSERT_ENABLE_MST      0x00000002  /* enable standard SERIAL_LSRMST_MST insertion */
#define C0CE_INSERT_ENABLE_RBR      0x00000100  /* enable C0CE_INSERT_RBR insertion */
#define C0CE_INSERT_ENABLE_RLC      0x00000200  /* enable C0CE_INSERT_RLC insertion */
#define C0CE_INSERT_ENABLE_LSR_BI   0x00000400  /* enable SERIAL_LSRMST_LSR_[NO]DATA insertion on BREAK change */

#endif /* _INCLUDE_C0C_CNCEXT_H_ */
