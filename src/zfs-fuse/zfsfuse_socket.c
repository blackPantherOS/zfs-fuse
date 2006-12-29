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
 * Copyright 2006 Ricardo Correia.
 * Use is subject to license terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>	
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include <sys/fs/zfs.h>

#include "zfsfuse_socket.h"

#define LOCKDIR "/var/lock/zfs"
#define LOCKFILE LOCKDIR "/zfs_lock"

int cur_fd = -1;

static int zfsfuse_do_locking()
{
	/* Ignores errors since the directory might already exist */
	mkdir(LOCKDIR, 0700);

	int lock_fd = creat(LOCKFILE, S_IRUSR | S_IWUSR);
	if(lock_fd == -1)
		return -1;

	return lockf(lock_fd, F_TLOCK, 0);
}

int zfsfuse_socket_create()
{
	struct sockaddr_un name;

	int sock;
	size_t size;

	if(zfsfuse_do_locking() != 0) {
		fprintf(stderr, "Error locking " LOCKFILE "\nMake sure there isn't another zfs-fuse process running and that you have appropriate permissions\n");
		return -1;
	}

	/* Create the socket. */
	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if(sock == -1) {
		int err = errno;
		fprintf(stderr, "Error creating UNIX socket: %s\n", strerror(err));
		return -1;
	}

	/* Try to create the directory, ignore errors */
	mkdir(ZPOOL_CACHE_DIR, 0700);

	/* Bind a name to the socket. */
	name.sun_family = AF_LOCAL;
	strncpy(name.sun_path, ZFS_DEV_NAME, sizeof(name.sun_path));

	name.sun_path[sizeof(name.sun_path) - 1] = '\0';

	size = SUN_LEN(&name);

	unlink(ZFS_DEV_NAME);

	if(bind(sock, &name, size) != 0) {
		int err = errno;
		fprintf(stderr, "Error binding UNIX socket to %s: %s\n", ZFS_DEV_NAME, strerror(err));
		return -1;
	}

	if(listen(sock, 5) != 0) {
		int err = errno;
		fprintf(stderr, "Error binding UNIX socket to %s: %s\n", ZFS_DEV_NAME, strerror(err));
		return -1;
	}

	return sock;
}

/*
 * This function is repeated in lib/libzfs/libzfs_zfsfuse.c
 * and in zfs-fuse/fuse_listener.c
 */
int zfsfuse_socket_read_loop(int fd, void *buf, int bytes)
{
	int read_bytes = 0;
	int left_bytes = bytes;

	while(left_bytes > 0) {
		int ret = recvfrom(fd, buf + read_bytes, left_bytes, 0, NULL, NULL);
		if(ret == 0)
			return -1;

		if(ret == -1) {
			if(errno == EINTR)
				continue;
			return -1;
		}
		read_bytes += ret;
		left_bytes -= ret;
	}
	return 0;
}

void zfsfuse_socket_close(int fd)
{
	close(fd);

	unlink(ZFS_DEV_NAME);
}

int zfsfuse_socket_ioctl_write(int fd, int ret)
{
#ifdef DEBUG
	/* Clear valgrind's uninitialized byte(s) warning */
	zfsfuse_cmd_t cmd = { 0 };
#else
	zfsfuse_cmd_t cmd;
#endif

	cmd.cmd_type = IOCTL_ANS;
	cmd.cmd_u.ioctl_ans_ret = ret;

	if(write(fd, &cmd, sizeof(zfsfuse_cmd_t)) != sizeof(zfsfuse_cmd_t))
		return -1;

	return 0;
}

int xcopyin(const void *src, void *dest, size_t size)
{
#ifdef DEBUG
	/* Clear valgrind's uninitialized byte(s) warning */
	zfsfuse_cmd_t cmd = { 0 };
#else
	zfsfuse_cmd_t cmd;
#endif

	/* This should catch stray xcopyin()s in the code.. */
	VERIFY(cur_fd >= 0);

	cmd.cmd_type = COPYIN_REQ;
	cmd.cmd_u.copy_req.ptr = (uint64_t)(uintptr_t) src;
	cmd.cmd_u.copy_req.size = size;

	if(write(cur_fd, &cmd, sizeof(zfsfuse_cmd_t)) != sizeof(zfsfuse_cmd_t))
		return EFAULT;

	if(zfsfuse_socket_read_loop(cur_fd, dest, size) != 0)
		return EFAULT;

	return 0;
}

int xcopyout(const void *src, void *dest, size_t size)
{
#ifdef DEBUG
	/* Clear valgrind's uninitialized byte(s) warning */
	zfsfuse_cmd_t cmd = { 0 };
#else
	zfsfuse_cmd_t cmd;
#endif

	/* This should catch stray xcopyout()s in the code.. */
	VERIFY(cur_fd >= 0);

	cmd.cmd_type = COPYOUT_REQ;
	cmd.cmd_u.copy_req.ptr = (uint64_t)(uintptr_t) dest;
	cmd.cmd_u.copy_req.size = size;

	if(write(cur_fd, &cmd, sizeof(zfsfuse_cmd_t)) != sizeof(zfsfuse_cmd_t))
		return EFAULT;

	if(write(cur_fd, src, size) != size)
		return EFAULT;

	return 0;
}
