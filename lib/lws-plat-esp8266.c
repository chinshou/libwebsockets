#include "private-libwebsockets.h"

/*
 * included from libwebsockets.c for esp8266 builds
 */

unsigned long long time_in_microseconds(void)
{
	return 123456;
}

LWS_VISIBLE int
lws_get_random(struct lws_context *context, void *buf, int len)
{
//	return read(context->fd_random, (char *)buf, len);
	return 0;
}

LWS_VISIBLE int
lws_send_pipe_choked(struct lws *wsi)
{
#if 0
	struct lws_pollfd fds;

	/* treat the fact we got a truncated send pending as if we're choked */
	if (wsi->trunc_len)
		return 1;

	fds.fd = wsi->sock;
	fds.events = POLLOUT;
	fds.revents = 0;

	if (poll(&fds, 1, 0) != 1)
		return 1;

	if ((fds.revents & POLLOUT) == 0)
		return 1;

	/* okay to send another packet without blocking */
#endif
	return 0;
}

LWS_VISIBLE int
lws_poll_listen_fd(struct lws_pollfd *fd)
{
#if 0
	return poll(fd, 1, 0);
#endif
}

LWS_VISIBLE void
lws_cancel_service_pt(struct lws *wsi)
{
#if 0
	struct lws_context_per_thread *pt = &wsi->context->pt[(int)wsi->tsi];
	char buf = 0;

	if (write(pt->dummy_pipe_fds[1], &buf, sizeof(buf)) != 1)
		lwsl_err("Cannot write to dummy pipe");
#endif
}

LWS_VISIBLE void
lws_cancel_service(struct lws_context *context)
{
#if 0
	struct lws_context_per_thread *pt = &context->pt[0];
	char buf = 0, m = context->count_threads;

	while (m--) {
		if (write(pt->dummy_pipe_fds[1], &buf, sizeof(buf)) != 1)
			lwsl_err("Cannot write to dummy pipe");
		pt++;
	}
#endif
}

LWS_VISIBLE void lwsl_emit_syslog(int level, const char *line)
{
	NODE_DBG("%d: %s", level, line);
}

LWS_VISIBLE int
lws_plat_service_tsi(struct lws_context *context, int timeout_ms, int tsi)
{
#if 0
	struct lws_context_per_thread *pt = &context->pt[tsi];
	int n = -1, m, c;
	char buf;

	/* stay dead once we are dead */

	if (!context || !context->vhost_list)
		return 1;

	if (timeout_ms < 0)
		goto faked_service;

	lws_libev_run(context, tsi);
	lws_libuv_run(context, tsi);

	if (!context->service_tid_detected) {
		struct lws _lws;

		memset(&_lws, 0, sizeof(_lws));
		_lws.context = context;

		context->service_tid_detected =
			context->vhost_list->protocols[0].callback(
			&_lws, LWS_CALLBACK_GET_THREAD_ID, NULL, NULL, 0);
	}
	context->service_tid = context->service_tid_detected;

	timeout_ms = lws_service_adjust_timeout(context, timeout_ms, tsi);

	n = poll(pt->fds, pt->fds_count, timeout_ms);

#ifdef LWS_OPENSSL_SUPPORT
	if (!pt->rx_draining_ext_list &&
	    !lws_ssl_anybody_has_buffered_read_tsi(context, tsi) && !n) {
#else
	if (!pt->rx_draining_ext_list && !n) /* poll timeout */ {
#endif
		lws_service_fd_tsi(context, NULL, tsi);
		return 0;
	}

faked_service:
	m = lws_service_flag_pending(context, tsi);
	if (m)
		c = -1; /* unknown limit */
	else
		if (n < 0) {
			if (LWS_ERRNO != LWS_EINTR)
				return -1;
			return 0;
		} else
			c = n;

	/* any socket with events to service? */
	for (n = 0; n < pt->fds_count && c; n++) {
		if (!pt->fds[n].revents)
			continue;

		c--;

		if (pt->fds[n].fd == pt->dummy_pipe_fds[0]) {
			if (read(pt->fds[n].fd, &buf, 1) != 1)
				lwsl_err("Cannot read from dummy pipe.");
			continue;
		}

		m = lws_service_fd_tsi(context, &pt->fds[n], tsi);
		if (m < 0)
			return -1;
		/* if something closed, retry this slot */
		if (m)
			n--;
	}
#endif
	return 0;
}

LWS_VISIBLE int
lws_plat_check_connection_error(struct lws *wsi)
{
	return 0;
}

LWS_VISIBLE int
lws_plat_service(struct lws_context *context, int timeout_ms)
{
//	return lws_plat_service_tsi(context, timeout_ms, 0);
	return 0;
}

LWS_VISIBLE int
lws_plat_set_socket_options(struct lws_vhost *vhost, lws_sockfd_type fd)
{
#if 0
	int optval = 1;
	socklen_t optlen = sizeof(optval);

#if defined(__APPLE__) || \
    defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__NetBSD__) || \
    defined(__OpenBSD__)
	struct protoent *tcp_proto;
#endif

	if (vhost->ka_time) {
		/* enable keepalive on this socket */
		optval = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
			       (const void *)&optval, optlen) < 0)
			return 1;

#if defined(__APPLE__) || \
    defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__NetBSD__) || \
        defined(__CYGWIN__) || defined(__OpenBSD__)

		/*
		 * didn't find a way to set these per-socket, need to
		 * tune kernel systemwide values
		 */
#else
		/* set the keepalive conditions we want on it too */
		optval = vhost->ka_time;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,
			       (const void *)&optval, optlen) < 0)
			return 1;

		optval = vhost->ka_interval;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,
			       (const void *)&optval, optlen) < 0)
			return 1;

		optval = vhost->ka_probes;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,
			       (const void *)&optval, optlen) < 0)
			return 1;
#endif
	}

	/* Disable Nagle */
	optval = 1;
#if !defined(__APPLE__) && \
    !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) && \
    !defined(__NetBSD__) && \
    !defined(__OpenBSD__)
	if (setsockopt(fd, SOL_TCP, TCP_NODELAY, (const void *)&optval, optlen) < 0)
		return 1;
#else
	tcp_proto = getprotobyname("TCP");
	if (setsockopt(fd, tcp_proto->p_proto, TCP_NODELAY, &optval, optlen) < 0)
		return 1;
#endif

	/* We are nonblocking... */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		return 1;
#endif
	return 0;
}

LWS_VISIBLE void
lws_plat_drop_app_privileges(struct lws_context_creation_info *info)
{
}

LWS_VISIBLE int
lws_plat_context_early_init(void)
{
//	signal(SIGPIPE, SIG_IGN);

	return 0;
}

LWS_VISIBLE void
lws_plat_context_early_destroy(struct lws_context *context)
{
}

LWS_VISIBLE void
lws_plat_context_late_destroy(struct lws_context *context)
{
#if 0
	struct lws_context_per_thread *pt = &context->pt[0];
	int m = context->count_threads;

	if (context->lws_lookup)
		lws_free(context->lws_lookup);

	while (m--) {
		close(pt->dummy_pipe_fds[0]);
		close(pt->dummy_pipe_fds[1]);
		pt++;
	}
#endif
//	close(context->fd_random);
}

/* cast a struct sockaddr_in6 * into addr for ipv6 */

LWS_VISIBLE int
lws_interface_to_sa(int ipv6, const char *ifname, struct sockaddr_in *addr,
		    size_t addrlen)
{
	return 0;
}

LWS_VISIBLE void
lws_plat_insert_socket_into_fds(struct lws_context *context, struct lws *wsi)
{
#if 0
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	lws_libev_io(wsi, LWS_EV_START | LWS_EV_READ);
	lws_libuv_io(wsi, LWS_EV_START | LWS_EV_READ);

	pt->fds[pt->fds_count++].revents = 0;
#endif
}

LWS_VISIBLE void
lws_plat_delete_socket_from_fds(struct lws_context *context,
						struct lws *wsi, int m)
{
#if 0
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	lws_libev_io(wsi, LWS_EV_STOP | LWS_EV_READ | LWS_EV_WRITE);
	lws_libuv_io(wsi, LWS_EV_STOP | LWS_EV_READ | LWS_EV_WRITE);

	pt->fds_count--;
#endif
}

LWS_VISIBLE void
lws_plat_service_periodic(struct lws_context *context)
{
#if 0
	/* if our parent went down, don't linger around */
	if (context->started_with_parent &&
	    kill(context->started_with_parent, 0) < 0)
		kill(getpid(), SIGTERM);
#endif
}

LWS_VISIBLE int
lws_plat_change_pollfd(struct lws_context *context,
		      struct lws *wsi, struct lws_pollfd *pfd)
{
	return 0;
}

LWS_VISIBLE const char *
lws_plat_inet_ntop(int af, const void *src, char *dst, int cnt)
{
//	return inet_ntop(af, src, dst, cnt);
	return 0;
}

static lws_filefd_type
_lws_plat_file_open(struct lws *wsi, const char *filename,
		    unsigned long *filelen, int flags)
{
#if 0
	struct stat stat_buf;
	int ret = open(filename, flags, 0664);

	if (ret < 0)
		return LWS_INVALID_FILE;

	if (fstat(ret, &stat_buf) < 0) {
		close(ret);
		return LWS_INVALID_FILE;
	}
	*filelen = stat_buf.st_size;
	return ret;
#endif

	return LWS_INVALID_FILE;
}

static int
_lws_plat_file_close(struct lws *wsi, lws_filefd_type fd)
{
	return 0; // close(fd);
}

unsigned long
_lws_plat_file_seek_cur(struct lws *wsi, lws_filefd_type fd, long offset)
{
	return 0; //lseek(fd, offset, SEEK_CUR);
}

static int
_lws_plat_file_read(struct lws *wsi, lws_filefd_type fd, unsigned long *amount,
		    unsigned char *buf, unsigned long len)
{
#if 0
	long n;

	n = read((int)fd, buf, len);
	if (n == -1) {
		*amount = 0;
		return -1;
	}

	*amount = n;
#endif
	return 0;
}

static int
_lws_plat_file_write(struct lws *wsi, lws_filefd_type fd, unsigned long *amount,
		     unsigned char *buf, unsigned long len)
{
#if 0
	long n;

	n = write((int)fd, buf, len);
	if (n == -1) {
		*amount = 0;
		return -1;
	}

	*amount = n;
#endif
	return 0;
}


LWS_VISIBLE int
lws_plat_init(struct lws_context *context,
	      struct lws_context_creation_info *info)
{
	struct lws_context_per_thread *pt = &context->pt[0];
	int n = context->count_threads, fd;

	/* master context has the global fd lookup array */
	context->lws_lookup = lws_zalloc(sizeof(struct lws *) *
					 context->max_fds);
	if (context->lws_lookup == NULL) {
		lwsl_err("OOM on lws_lookup array for %d connections\n",
			 context->max_fds);
		return 1;
	}

	lwsl_notice(" mem: platform fd map: %5u bytes\n",
		    sizeof(struct lws *) * context->max_fds);
//	fd = open(SYSTEM_RANDOM_FILEPATH, O_RDONLY);

//	context->fd_random = fd;
//	if (context->fd_random < 0) {
//		lwsl_err("Unable to open random device %s %d\n",
//			 SYSTEM_RANDOM_FILEPATH, context->fd_random);
//		return 1;
//	}

	if (!lws_libev_init_fd_table(context) &&
	    !lws_libuv_init_fd_table(context)) {
		/* otherwise libev handled it instead */
#if 0
		while (n--) {
			if (pipe(pt->dummy_pipe_fds)) {
				lwsl_err("Unable to create pipe\n");
				return 1;
			}

			/* use the read end of pipe as first item */
			pt->fds[0].fd = pt->dummy_pipe_fds[0];
			pt->fds[0].events = LWS_POLLIN;
			pt->fds[0].revents = 0;
			pt->fds_count = 1;
			pt++;
		}
#endif
	}

	context->fops.open	= _lws_plat_file_open;
	context->fops.close	= _lws_plat_file_close;
	context->fops.seek_cur	= _lws_plat_file_seek_cur;
	context->fops.read	= _lws_plat_file_read;
	context->fops.write	= _lws_plat_file_write;

#ifdef LWS_WITH_PLUGINS
	if (info->plugin_dirs)
		lws_plat_plugins_init(context, info->plugin_dirs);
#endif

	return 0;
}
