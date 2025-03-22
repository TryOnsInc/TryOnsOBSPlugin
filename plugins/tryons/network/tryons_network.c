#include "tryons_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
#include "../tryons_common.h"

static int network_socket = -1;
static volatile int network_running = 0;
#ifdef _WIN32
static WSADATA wsaData;
#endif

// This is a very basic example using blocking sockets.
static void *network_thread_func(void *arg)
{
	(void)arg;
	char buffer[1024];
	while (network_running) {
		// Accept a connection and receive data…
		// (Your robust protocol code would go here.)
		// For demonstration, assume we receive a message like: "TYPE:text;DATA:Hello, world!"
		int bytes_received = recv(network_socket, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			// Simple parsing (in practice use proper parsing!)
			if (strstr(buffer, "TYPE:text") != NULL) {
				// Extract data after "DATA:" – assume fixed format.
				char *data = strstr(buffer, "DATA:");
				if (data) {
					data += 5;
					tryons_text_source_network_update(data);
				}
			} else if (strstr(buffer, "TYPE:audio") != NULL) {
				char *data = strstr(buffer, "DATA:");
				if (data) {
					data += 5;
					tryons_audio_source_network_update(data);
				}
			}
		}
		// Sleep or yield briefly
#ifdef _WIN32
		Sleep(100);
#else
		usleep(100000);
#endif
	}
	return NULL;
}

bool tryons_network_init(void)
{
	network_running = 1;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;
#endif
	network_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (network_socket < 0)
		return false;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(12345); // Listen on port 12345

	if (bind(network_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return false;
	if (listen(network_socket, 5) < 0)
		return false;

// Create the network thread (platform‐dependent; here using pthreads)
#ifdef _WIN32
	HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)network_thread_func, NULL, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, network_thread_func, NULL);
#endif

	return true;
}

void tryons_network_poll(void)
{
	// If using a polling model, process events here.
}

void tryons_network_shutdown(void)
{
	network_running = 0;
#ifdef _WIN32
	closesocket(network_socket);
	WSACleanup();
#else
	close(network_socket);
#endif
}
