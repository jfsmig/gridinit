/*
gridinit, a monitor for non-daemon processes.
Copyright (C) 2013 AtoS Worldline, original work aside of Redcurrant
Copyright (C) 2015-2018 OpenIO SAS

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <glib.h>

#include "gridinit-utils.h"
#include "gridinit_internals.h"

int
__open_unix_client(const char *path)
{
	int sock;
	struct sockaddr_un local = {};

	if (!path || strlen(path) >= sizeof(local.sun_path)) {
		errno = EINVAL;
		return -1;
	}

	/* Create ressources to monitor */
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	/* Bind to file */
	local.sun_family = AF_UNIX;
	g_strlcpy(local.sun_path, path, sizeof(local.sun_path)-1);

	if (-1 == connect(sock, (struct sockaddr *)&local, sizeof(local)))
		goto label_error;

	errno = 0;
	return sock;

label_error:
	if (sock >= 0) {
		typeof(errno) errsav;
		errsav = errno;
		close(sock);
		errno = errsav;
	}
	return -1;
}

int
__open_unix_server(const char *path)
{
	struct sockaddr_un local = {};

	if (!path || strlen(path) >= sizeof(local.sun_path)) {
		errno = EINVAL;
		return -1;
	}

	/* Create ressources to monitor */
#ifdef SOCK_CLOEXEC
# define SOCK_FLAGS SOCK_CLOEXEC
#else
# define SOCK_FLAGS 0
#endif

	int sock = socket(PF_UNIX, SOCK_STREAM | SOCK_FLAGS, 0);
	if (sock < 0)
		return -1;

#ifndef SOCK_CLOEXEC
# ifdef FD_CLOEXEC
	(void) fcntl(sock, F_SETFD, fcntl(sock, F_GETFD)|FD_CLOEXEC);
# endif
#endif

	/* Bind to file */
	local.sun_family = AF_UNIX;
	g_strlcpy(local.sun_path, path, sizeof(local.sun_path)-1);

	if (-1 == bind(sock, (struct sockaddr *)&local, sizeof(local)))
		goto label_error;

	/* Listen on that socket */
	if (-1 == listen(sock, 65536))
		goto label_error;

	errno = 0;
	return sock;

label_error:
	if (sock >= 0) {
		typeof(errno) errsav;
		errsav = errno;
		close(sock);
		errno = errsav;
	}
	return -1;
}

