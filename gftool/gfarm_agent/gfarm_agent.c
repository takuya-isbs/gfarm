/*
 * $Id$
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include <gfarm/gfarm.h>
#include "gfutil.h"
#include "xxx_proto.h"
#include "io_fd.h"
#include "agent_proto.h"
#include "agent_wrap.h"

#ifdef SOMAXCONN
#define LISTEN_BACKLOG	SOMAXCONN
#else
#define LISTEN_BACKLOG	5
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX	108
#endif

char *program_name = "gfarm_agent";

int debug_mode = 0;

/* this routine should be called before calling exit(). */
static void
cleanup(void)
{
	/* disconnect, do logging */
	gflog_notice("disconnected", NULL);
}

static void
log_proto(char *proto, char *status)
{
	gflog_notice(proto, status);
}

static void
error_proto(char *proto, char *status)
{
	cleanup();
	gflog_error(proto, status);
}

static char *
agent_server_get_request(struct xxx_connection *client, char *diag,
	char *format, ...)
{
	va_list ap;
	char *e;
	int eof;

	va_start(ap, format);
	e = xxx_proto_vrecv(client, 0, &eof, &format, &ap);
	va_end(ap);

	if (e == NULL) {
		if (eof)
			e = "missing RPC argument";
		else if (*format != '\0')
			e = "invalid format character to get request";
	}
	if (e != NULL)
		error_proto(diag, e);

	return (e);
}

static char *
agent_server_put_reply_common(struct xxx_connection *client, char *diag,
	int ecode, char *format, va_list *app)
{
	char *e;

	e = xxx_proto_send(client, "i", (gfarm_int32_t)ecode);
	if (e != NULL) {
		error_proto(diag, e);
		return (e);
	}
	if (ecode == 0) {
		e = xxx_proto_vsend(client, &format, app);
		if (e != NULL)
			error_proto(diag, e);
		if (*format != '\0') {
			e = "invalid format character to put reply";
			error_proto(diag, e);
		}
	}
	return (e);
}

static char *
agent_server_put_reply(struct xxx_connection *client, char *diag,
	char *error, char *format, ...)
{
	va_list ap;
	int eno;
	char *e;

	if (error == NULL)
		eno = 0;
	else
		eno = gfarm_error_to_errno(error);

	va_start(ap, format);
	e = agent_server_put_reply_common(client, diag, eno, format, &ap);
	va_end(ap);
	return (e);
}

static char *
agent_server_path_info_get(struct xxx_connection *client)
{
	char *path, *e, *e_rpc;
	struct gfarm_path_info pi;

	e_rpc = agent_server_get_request(client, "path_info_get", "s", &path);
	if (e_rpc != NULL)
		return (e_rpc); /* protocol error */

	e = gfarm_i_path_info_get(path, &pi);
	free(path);

	e_rpc = agent_server_put_reply(
		client, "path_info_get", e, "siissoiiiiiii",
		pi.pathname,
		pi.status.st_ino, pi.status.st_mode,
		pi.status.st_user, pi.status.st_group,
		pi.status.st_size, pi.status.st_nsections,
		pi.status.st_atimespec.tv_sec, pi.status.st_atimespec.tv_nsec, 
		pi.status.st_mtimespec.tv_sec, pi.status.st_mtimespec.tv_nsec, 
		pi.status.st_ctimespec.tv_sec, pi.status.st_ctimespec.tv_nsec);
	if (e == NULL)
		gfarm_path_info_free(&pi);
	if (e != NULL)
		log_proto("path_info_get", e);
	return (e_rpc);
}

static char *
agent_server_path_info_set(struct xxx_connection *client)
{
	char *pathname, *e, *e_rpc;
	struct gfarm_path_info pi;
	struct gfs_stat *st = &pi.status;

	e_rpc = agent_server_get_request(
		client, "path_info_set", "siissoiiiiiii",
		&pathname,
		&st->st_ino, &st->st_mode,
		&st->st_user, &st->st_group,
		&st->st_size, &st->st_nsections,
		&st->st_atimespec.tv_sec, &st->st_atimespec.tv_nsec, 
		&st->st_mtimespec.tv_sec, &st->st_mtimespec.tv_nsec, 
		&st->st_ctimespec.tv_sec, &st->st_ctimespec.tv_nsec);
	if (e_rpc != NULL)
		return (e_rpc);

	pi.pathname = pathname;
	e = gfarm_i_path_info_set(pathname, &pi);
	/* pathname will be free'ed in gfarm_path_info_free(&pi) */
	gfarm_path_info_free(&pi);

	e_rpc = agent_server_put_reply(client, "path_info_set", e, "");
	if (e != NULL)
		log_proto("path_info_set", e);
	return (e_rpc);
}

static char *
agent_server_path_info_replace(struct xxx_connection *client)
{
	char *pathname, *e, *e_rpc;
	struct gfarm_path_info pi;
	struct gfs_stat *st = &pi.status;

	e_rpc = agent_server_get_request(
		client, "path_info_replace", "siissoiiiiiii",
		&pathname,
		&st->st_ino, &st->st_mode,
		&st->st_user, &st->st_group,
		&st->st_size, &st->st_nsections,
		&st->st_atimespec.tv_sec, &st->st_atimespec.tv_nsec, 
		&st->st_mtimespec.tv_sec, &st->st_mtimespec.tv_nsec, 
		&st->st_ctimespec.tv_sec, &st->st_ctimespec.tv_nsec);
	if (e_rpc != NULL)
		return (e_rpc);

	pi.pathname = pathname;
	e = gfarm_i_path_info_replace(pathname, &pi);
	gfarm_path_info_free(&pi);

	e_rpc = agent_server_put_reply(client, "path_info_replace", e, "");
	if (e != NULL)
		log_proto("path_info_replace", e);
	return (e_rpc);
}

static char *
agent_server_path_info_remove(struct xxx_connection *client)
{
	char *pathname, *e, *e_rpc;

	e_rpc = agent_server_get_request(
		client, "path_info_remove", "s", &pathname);
	if (e_rpc != NULL)
		return (e_rpc);

	e = gfarm_i_path_info_remove(pathname);
	free(pathname);

	e_rpc = agent_server_put_reply(client, "path_info_remove", e, "");
	if (e != NULL)
		log_proto("path_info_remove", e);
	return (e_rpc);
}

static char *
agent_server_realpath_canonical(struct xxx_connection *client)
{
	char *path, *abspath, *e, *e_rpc;

	e_rpc = agent_server_get_request(
		client, "realpath_canonical", "s", &path);
	if (e_rpc != NULL)
		return (e_rpc);

	e = gfs_i_realpath_canonical(path, &abspath);
	free(path);

	e_rpc = agent_server_put_reply(
		client, "realpath_canonical", e, "s", abspath);
	if (e == NULL)
		free(abspath);
	if (e != NULL)
		log_proto("realpath_canonical", e);
	return (e_rpc);
}

static char *
agent_server_get_ino(struct xxx_connection *client)
{
	char *path, *e, *e_rpc;
	long ino;

	e_rpc = agent_server_get_request(client, "get_ino", "s", &path);
	if (e_rpc != NULL)
		return (e_rpc);

	e = gfs_i_get_ino(path, &ino);
	free(path);

	e_rpc = agent_server_put_reply(client, "get_ino", e, "i", ino);
	if (e != NULL)
		log_proto("get_ino", e);
	return (e_rpc);
}

/* pointer table */

#define PTABLE_LEN	1024
static void *ptable[PTABLE_LEN];
static int ptable_free_ptr;

#define PTABLE_OUT_OF_RANGE(p)	((p) < 0 || (p) >= PTABLE_LEN)

static void *
get_pointer(int p)
{
	if (PTABLE_OUT_OF_RANGE(p))
		return (0);

	return (ptable[p]);
}

static int
set_pointer(void *ptr)
{
	int saved_ptr = ptable_free_ptr;

	if (PTABLE_OUT_OF_RANGE(ptable_free_ptr))
		return (-1);
	if (ptable[ptable_free_ptr])
		return (-1);

	ptable[ptable_free_ptr] = ptr;
	while (get_pointer(++ptable_free_ptr));

	return (saved_ptr);
}

static char *
release_pointer(int p)
{
	if (get_pointer(p))
		ptable[p] = 0;
	else
		return ("already released or out of range");

	if (p < ptable_free_ptr)
		ptable_free_ptr = p;

	return (NULL);
}

static char *
agent_server_opendir(struct xxx_connection *client)
{
	char *path, *e, *e_rpc;
	GFS_Dir dir;
	int dir_index = -1;

	e_rpc = agent_server_get_request(client, "opendir", "s", &path);
	if (e_rpc != NULL)
		return (e_rpc);

	e = gfs_i_opendir(path, &dir);
	free(path);
	if (e == NULL) {
		dir_index = set_pointer(dir);
		if (dir_index < 0) {
			e = GFARM_ERR_NO_SPACE;
			(void)gfs_i_closedir(dir);
		}
	}
	e_rpc = agent_server_put_reply(client, "opendir", e, "i", dir_index);
	if (e != NULL)
		log_proto("opendir", e);
	return (e_rpc);
}

static char *
agent_server_readdir(struct xxx_connection *client)
{
	char *e, *e_rpc;
	int dir_index;
	GFS_Dir dir;
	struct gfs_dirent *entry;

	e_rpc = agent_server_get_request(client, "readdir", "i", &dir_index);
	if (e_rpc != NULL)
		return (e_rpc);

	dir = get_pointer(dir_index);
	if (dir)
		e = gfs_i_readdir(dir, &entry);
	else
		e = GFARM_ERR_NO_SUCH_OBJECT; /* XXX - EBADF */

	if (entry)
		e_rpc = agent_server_put_reply(
			client, "readdir", e, "ihccs",
			entry->d_fileno, entry->d_reclen,
			entry->d_type, entry->d_namlen, entry->d_name);
	else
		e_rpc = agent_server_put_reply(
			client, "readdir", e, "ihccs",
			0, 0, 0, 1, "");
	if (e != NULL)
		log_proto("readdir", e);
	return (e_rpc);
}

static char *
agent_server_closedir(struct xxx_connection *client)
{
	char *e, *e_rpc;
	int dir_index;
	GFS_Dir dir;

	e_rpc = agent_server_get_request(client, "closedir", "i", &dir_index);
	if (e_rpc != NULL)
		return (e_rpc);

	dir = get_pointer(dir_index);
	if (dir)
		e = gfs_i_closedir(dir);
	else
		e = GFARM_ERR_NO_SUCH_OBJECT; /* XXX - EBADF */
	if (e == NULL)
		release_pointer(dir_index);

	e_rpc = agent_server_put_reply(client, "closedir", e, "");
	if (e != NULL)
		log_proto("closedir", e);
	return (e_rpc);
}

static char *
agent_server_dirname(struct xxx_connection *client)
{
	char *e_rpc, *name = NULL;
	int dir_index;
	GFS_Dir dir;

	e_rpc = agent_server_get_request(client, "dirname", "i", &dir_index);
	if (e_rpc != NULL)
		return (e_rpc);

	dir = get_pointer(dir_index);
	if (dir)
		name = gfs_i_dirname(dir);

	e_rpc = agent_server_put_reply(client, "dirname", NULL, "s",
			name != NULL ? name : GFARM_ERR_NO_SUCH_OBJECT);
	if (name == NULL)
		log_proto("dirname", GFARM_ERR_NO_SUCH_OBJECT);
	return (e_rpc);
}

static char *
agent_server_uncachedir(struct xxx_connection *client)
{
	char *e_rpc;

	e_rpc = agent_server_get_request(client, "uncachedir", "");
	if (e_rpc != NULL)
		return (e_rpc);

	gfs_i_uncachedir();

	e_rpc = agent_server_put_reply(client, "uncachedir", NULL, "");
	return (e_rpc);
}

static int gfarm_initialized = 0;

void
server(int client_fd)
{
	char *e;
	struct xxx_connection *client;
	int eof;
	gfarm_int32_t request;
	char buffer[GFARM_INT32STRLEN];

	if (!gfarm_initialized) {
		e = gfarm_initialize(NULL, NULL);
		if (e == NULL)
			gfarm_initialized = 1;
		else {
			(void)gfarm_terminate();
			close(client_fd);
			error_proto("gfarm_initialize", e);
			return;
		}
	}

	e = gfarm_metadb_check();
	if (e != NULL) {
		log_proto("gfarm_metadb_check", e);
		e = gfarm_metadb_initialize();
		if (e != NULL) {
			close(client_fd);
			error_proto("gfarm_metadb_initialize", e);
			return;
		}
	}

	e = xxx_fd_connection_new(client_fd, &client);
	if (e != NULL) {
		close(client_fd);
		error_proto("xxx_connection_new", e);
		return;
	}

	for (;;) {
		char *cmd;
		e = xxx_proto_recv(client, 0, &eof, "i", &request);
		if (e != NULL) {
			error_proto("request number", e);
			goto exit_free_conn;
		}
		if (eof) {
			cleanup();
			goto exit_free_conn;
		}
		switch (request) {
		case AGENT_PROTO_PATH_INFO_GET:
			cmd = "path_info_get";
			e = agent_server_path_info_get(client);
			break;
		case AGENT_PROTO_PATH_INFO_SET:
			cmd = "path_info_set";
			e = agent_server_path_info_set(client);
			break;
		case AGENT_PROTO_PATH_INFO_REPLACE:
			cmd = "path_info_replace";
			e = agent_server_path_info_replace(client);
			break;
		case AGENT_PROTO_PATH_INFO_REMOVE:
			cmd = "path_info_remove";
			e = agent_server_path_info_remove(client);
			break;
		case AGENT_PROTO_REALPATH_CANONICAL:
			cmd = "realpath_canonical";
			e = agent_server_realpath_canonical(client);
			break;
		case AGENT_PROTO_GET_INO:
			cmd = "get_ino";
			e = agent_server_get_ino(client);
			break;
		case AGENT_PROTO_OPENDIR:
			cmd = "opendir";
			e = agent_server_opendir(client);
			break;
		case AGENT_PROTO_READDIR:
			cmd = "readdir";
			e = agent_server_readdir(client);
			break;
		case AGENT_PROTO_CLOSEDIR:
			cmd = "closedir";
			e = agent_server_closedir(client);
			break;
		case AGENT_PROTO_DIRNAME:
			cmd = "dirname";
			e = agent_server_dirname(client);
			break;
		case AGENT_PROTO_UNCACHEDIR:
			cmd = "uncachedir";
			e = agent_server_uncachedir(client);
			break;
		default:
			sprintf(buffer, "%d", (int)request);
			gflog_warning("unknown request", buffer);
			cleanup();
			goto exit_free_conn;
		}
		if (e != NULL) {
			/* protocol error */
			error_proto(cmd, e);
			goto exit_free_conn;
		}
	}
 exit_free_conn:
	/* 'client_fd' will be free'ed in xxx_connection_free() */
	e = xxx_connection_free(client);
	if (e != NULL)
		error_proto("xxx_connection_free", e);
}

static char *sock_path;

void
display_env(int fd)
{
	FILE *f = fdopen(fd, "w");
	pid_t pid = getpid();

	if (f == NULL)
		return;
	fprintf(f, "GFARM_AGENT_SOCK=%s; export GFARM_AGENT_SOCK;\n",
		sock_path);
	fprintf(f, "GFARM_AGENT_PID=%d; export GFARM_AGENT_PID;\n", pid);
	fprintf(f, "echo Agent pid %d;\n", pid);
	fclose(f);
}

#define AGENT_SOCK_TEMPLATE	"/tmp/.gfarm-XXXXXX"

int
open_accepting_socket(void)
{
	struct sockaddr_un self_addr;
	socklen_t self_addr_size;
	int sock, sockopt;
	char *sdir, sockdir[] = AGENT_SOCK_TEMPLATE;
	char tmpsoc[7 + GFARM_INT32STRLEN];

	sdir = mkdtemp(sockdir);
	if (sdir == NULL)
		gflog_fatal_errno("mkdtemp");
	sprintf(tmpsoc, "/agent.%ld", (long)getpid());

	memset(&self_addr, 0, sizeof(self_addr));
	self_addr.sun_family = AF_UNIX;
	strcpy(self_addr.sun_path, sdir);
	strcat(self_addr.sun_path, tmpsoc);
	self_addr_size = sizeof(self_addr);
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		(void)rmdir(sdir);
		gflog_fatal_errno("accepting socket");
	}
	sockopt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
	    &sockopt, sizeof(sockopt)) == -1)
		gflog_warning_errno("SO_REUSEADDR");
	if (bind(sock, (struct sockaddr *)&self_addr, self_addr_size) < 0) {
		(void)unlink(self_addr.sun_path);
		(void)rmdir(sdir);
		gflog_fatal_errno("bind accepting socket");
	}
	if (listen(sock, LISTEN_BACKLOG) < 0) {
		(void)unlink(self_addr.sun_path);
		(void)rmdir(sdir);
		gflog_fatal_errno("listen");
	}
	sock_path = strdup(self_addr.sun_path);
	return (sock);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s [option]\n", program_name);
	fprintf(stderr, "option:\n");
	fprintf(stderr, "\t-P <pid-file>\n");
	fprintf(stderr, "\t-f <gfarm-configuration-file>\n");
	fprintf(stderr, "\t-s <syslog-facility>\n");
	fprintf(stderr, "\t-v\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	struct sockaddr_un client_addr;
	socklen_t client_addr_size;
	char *e, *config_file = NULL, *pid_file = NULL;
	FILE *pid_fp = NULL;
	int syslog_facility = GFARM_DEFAULT_FACILITY;
	int ch, accepting_sock, client, stdout_fd;

	if (argc >= 1)
		program_name = basename(argv[0]);
	gflog_set_identifier(program_name);

	while ((ch = getopt(argc, argv, "P:df:s:v")) != -1) {
		switch (ch) {
		case 'P':
			pid_file = optarg;
			break;
		case 'd':
			debug_mode = 1;
			break;
		case 'f':
			config_file = optarg;
			break;
		case 's':
			syslog_facility =
			    gflog_syslog_name_to_facility(optarg);
			if (syslog_facility == -1)
				gflog_fatal(optarg, "unknown syslog facility");
			break;
		case 'v':
			gflog_auth_set_verbose(1);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (config_file != NULL)
		gfarm_config_set_filename(config_file);

	/*
	 * Gfarm_initialize() may fail when the metadata server is not
	 * running at start-up time.  In this case, retry at connection time.
	 */
	gfarm_agent_disable();
	e = gfarm_initialize(NULL, NULL);
	if (e == NULL)
		gfarm_initialized = 1;
	else
		(void)gfarm_terminate();

	accepting_sock = open_accepting_socket();
	stdout_fd = dup(1);

	if (pid_file != NULL) {
		/*
		 * We do this before calling gfarm_daemon()
		 * to print the error message to stderr.
		 */
		pid_fp = fopen(pid_file, "w");
		if (pid_fp == NULL)
			gflog_fatal_errno(pid_file);
	}
	if (!debug_mode) {
		gflog_syslog_open(LOG_PID, syslog_facility);
		gfarm_daemon(0, 0);
	}
	if (pid_file != NULL) {
		/*
		 * We do this after calling gfarm_daemon(),
		 * because it changes pid.
		 */
		fprintf(pid_fp, "%ld\n", (long)getpid());
		fclose(pid_fp);
	}
	display_env(stdout_fd);

	for (;;) {
		client_addr_size = sizeof(client_addr);
		client = accept(accepting_sock,
			(struct sockaddr *)&client_addr, &client_addr_size);
		if (client < 0) {
			if (errno == EINTR)
				continue;
			gflog_fatal_errno("accept");
		}
		server(client);
	}
	/*NOTREACHED*/
	return (0); /* to shut up warning */
}
