#include "tryons_network.h"
#include "tryons_common.h"
#include "tryons_text.h"
#include "tryons_audio.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a common socket type
#ifdef _WIN32
typedef SOCKET socket_t;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
typedef int socket_t;
#define INVALID_SOCKET_VALUE (-1)
#endif

// Define THREAD_HANDLE type
#ifdef _WIN32
typedef HANDLE THREAD_HANDLE;
#else
typedef pthread_t THREAD_HANDLE;
#endif

// Global variables
static socket_t server_socket = INVALID_SOCKET_VALUE;
static volatile int running = 0;
#ifdef _WIN32
static WSADATA wsaData;
#endif
static THREAD_HANDLE network_thread;

#ifdef _WIN32
DWORD WINAPI network_thread_func(LPVOID arg)
#else
void *network_thread_func(void *arg)
#endif
{
	(void)arg;
	char buffer[2048];
	while (running) {
		struct sockaddr_in client_addr;
#ifdef _WIN32
		int addr_len = sizeof(client_addr);
#else
		socklen_t addr_len = sizeof(client_addr);
#endif
		socket_t client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
		if (client_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
			continue;
		}
		int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			if (strstr(buffer, "TYPE:text") != NULL) {
				char *data = strstr(buffer, "DATA:");
				if (data) {
					data += 5;
					update_text_source(data);
				}
			} else if (strstr(buffer, "TYPE:audio") != NULL) {
				char *data = strstr(buffer, "DATA:");
				if (data) {
					data += 5;
					play_audio_file(data);
				}
			}
		}
#ifdef _WIN32
		closesocket(client_socket);
#else
		close(client_socket);
#endif
	}
	return 0;
}

bool tryons_network_init(void)
{
	running = 1;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;
#endif
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET_VALUE)
		return false;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(12345);
	if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return false;
	if (listen(server_socket, 5) < 0)
		return false;

#ifdef _WIN32
	network_thread = CreateThread(NULL, 0, network_thread_func, NULL, 0, NULL);
	if (network_thread == NULL)
		return false;
#else
	if (pthread_create(&network_thread, NULL, network_thread_func, NULL) != 0)
		return false;
#endif
	return true;
}

void tryons_network_shutdown(void)
{
	running = 0;
#ifdef _WIN32
	closesocket(server_socket);
	WaitForSingleObject(network_thread, INFINITE);
	WSACleanup();
#else
	close(server_socket);
	pthread_join(network_thread, NULL);
#endif
}
