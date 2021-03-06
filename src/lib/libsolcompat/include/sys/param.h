/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SOL_SYS_PARAM_H
#define _SOL_SYS_PARAM_H

#include_next <sys/param.h>

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller make make some file
 * systems unmountable.
 *
 * Note that the blocked devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 */
#ifndef __APPLE__
#define MAXBSIZE   8192
#endif
#define DEV_BSIZE  512
#define DEV_BSHIFT 9     /* log2(DEV_BSIZE) */

#define MAXNAMELEN 256
#define MAXOFFSET_T 0x7fffffffffffffffl

#define UID_NOBODY      60001   /* user ID no body */
#define GID_NOBODY      UID_NOBODY
#define UID_NOACCESS    60002   /* user ID no access */

#define MAXUID          2147483647      /* max user id */
#define MAXPROJID       MAXUID          /* max project id */

#define	FAMOUS_PID_SCHED	0
#define	FAMOUS_PID_INIT		1
#define	FAMOUS_PID_PAGEOUT	2
#define	FAMOUS_PID_FSFLUSH	3
#define	FAMOUS_PIDS		4

#endif
