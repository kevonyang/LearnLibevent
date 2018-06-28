#include <WinSock2.h>

#include "event2\event.h"
#include "event2\buffer.h"
#include "event2\bufferevent.h"
#include "event2\util.h"

int tcp_connect_server(const char *ip, int port)
{
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	evutil_inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr);
	//server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = port;

	evutil_socket_t server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd == -1)
		return -1;

	int status = connect(server_fd, (const sockaddr *)&server_addr, sizeof(server_addr));
	if (status == -1)
	{
		evutil_closesocket(server_fd);
	}

	evutil_make_socket_nonblocking(server_fd);
	return server_fd;
}

void read_callback(struct bufferevent *bev, void *ctx)
{
}

void write_callback(struct bufferevent *bev, void *ctx)
{
}

void event_callback(struct bufferevent *bev, short what, void *ctx)
{
}

void cmd_callback(evutil_socket_t, short, void *arg)
{
	printf("timeout callback\n");
	char cmd[1024];
	scanf("%s", cmd);
	printf("%s", cmd);
	struct bufferevent *bev = (bufferevent *)arg;
	if (!bev) return;
	bufferevent_write(bev, cmd, 1024);
}

int main(int argc, char *argv[])
{
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0)
	{
		printf("WSAStartup fail,%d!\n", result);
		return -1;
	}

	evutil_socket_t server_fd = tcp_connect_server("127.0.0.1", 9999);
	if (server_fd == -1)
	{
		return -1;
	}

	struct event_base *evt_base = event_base_new();
	struct bufferevent *bevt = bufferevent_socket_new(evt_base, server_fd, BEV_OPT_CLOSE_ON_FREE);

	struct event *evt_timer = event_new(evt_base, -1, EV_PERSIST, cmd_callback, bevt);
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	evtimer_add(evt_timer, &tv);

	bufferevent_setcb(bevt, read_callback, write_callback, event_callback, evt_timer);
	bufferevent_enable(bevt, EV_READ | EV_PERSIST);
	event_base_dispatch(evt_base);

	WSACleanup();

	return 0;
}