#include <WinSock2.h>

#include "event2\event.h"
#include "event2\buffer.h"
#include "event2\bufferevent.h"
#include "event2\util.h"

evutil_socket_t tcp_server_init(int port, int listen_num)
{
	evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == -1)
	{
		printf("tcp server init socket fail!\n");
		return -1;
	}
	evutil_make_listen_socket_reuseable(listener);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	const char *ip = "127.0.0.1";
	evutil_inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr);
	//server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = port;

	if (bind(listener, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		goto error;

	if (listen(listener, listen_num) < 0)
		goto error;

	evutil_make_socket_nonblocking(listener);
	return listener;
error:
	printf("tcp server init fail!\n");
	evutil_closesocket(listener);
	return -1;
}

void read_callback(bufferevent *bev_client, void *arg)
{
	printf("read callback\n");
	struct event_base *evt_base = (event_base*)arg;
	char cmd[1024];
	bufferevent_read(bev_client, cmd, 1024);
	printf("%s", cmd);
}

void write_callback(bufferevent *bev_client, void *arg)
{
	printf("write callback\n");
}

void event_callback(bufferevent *bev_client, short events, void *arg)
{
	printf("event callback\n");
	bufferevent_free(bev_client);
}

void accept_callback(evutil_socket_t listener, short events, void *arg)
{
	evutil_socket_t client_fd;
	struct sockaddr_in client_addr;
	ev_socklen_t client_addr_len = sizeof(client_addr);
	client_fd = accept(listener, (sockaddr *)&client_addr, &client_addr_len);
	if (client_fd < 0)
	{
		printf("client accept fail!");
		return;
	}
	evutil_make_socket_nonblocking(client_fd);

	struct event_base *evt_base = (event_base*)arg;

	bufferevent *bevt_client = bufferevent_socket_new(evt_base, client_fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bevt_client, read_callback, write_callback, event_callback, arg);
	bufferevent_enable(bevt_client, EV_READ | EV_WRITE | EV_PERSIST);
	printf("client accept!\n");
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

	evutil_socket_t listener = tcp_server_init(9999, 10000);
	if (listener < 0)
		return -1;

	struct event_base *evt_base = event_base_new();
	struct event *evt_listen = event_new(evt_base, listener, EV_READ | EV_PERSIST, accept_callback, evt_base);
	event_add(evt_listen, NULL);
	event_base_dispatch(evt_base);
	event_base_free(evt_base);
	
	WSACleanup();

	return 0;
}