/*
 * main.c
 *
 * UDP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP client
 * portable across Windows, Linux, and macOS.
 */

#if defined WIN32
#include <winsock.h>
typedef int socklen_t;
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock()
{
#if defined WIN32
	WSACleanup();
#endif
}

/* Print error messages to stderr */
static void errorhandler(const char *errorMessage)
{
	fprintf(stderr, "%s", errorMessage);
}

int parse_weather_response(const char *input, weather_response_t *resp)
{
	if (!input || !resp)
		return 1;

	unsigned int s;
	char t;
	float v;

	sscanf(input, "%u %c %f", &s, &t, &v);

	resp->status = s;
	resp->type = t;
	resp->value = v;

	return 0;
}

void parse_weather_request(const char *input, weather_request_t *req)
{

	// type = primo carattere
	req->type = input[0];

	// trova primo spazio
	const char *space = strchr(input, ' ');
	if (!space || *(space + 1) == '\0')
	{
		return;
	}

	// copia city
	strncpy(req->city, space + 1, sizeof(req->city) - 1);
	req->city[sizeof(req->city) - 1] = '\0';

	return;
}

int resolve_dns(const char *input, char *hostname_out, size_t hostname_size, char *ip_out, size_t ip_size) {
	if (!input || !hostname_out || !ip_out) {
		return -1;
	}

	struct hostent *host = NULL;
	struct in_addr addr;

	addr.s_addr = inet_addr(input);

	if (addr.s_addr == INADDR_NONE) {
		host = gethostbyname(input);
		if (!host) {
			fprintf(stderr, "Errore: impossibile risolvere l'hostname '%s'.\n", input);
			return -1;
		}

		struct in_addr *resolved_addr = (struct in_addr *)host->h_addr_list[0];
		strncpy(ip_out, inet_ntoa(*resolved_addr), ip_size - 1);
		ip_out[ip_size - 1] = '\0';

		strncpy(hostname_out, host->h_name, hostname_size - 1);
		hostname_out[hostname_size - 1] = '\0';

	} else {
		host = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);

		if (!host) {
			strncpy(hostname_out, input, hostname_size - 1);
			hostname_out[hostname_size - 1] = '\0';
		} else {
			strncpy(hostname_out, host->h_name, hostname_size - 1);
			hostname_out[hostname_size - 1] = '\0';
		}

		strncpy(ip_out, input, ip_size - 1);
		ip_out[ip_size - 1] = '\0';
	}

	return 0;
}

int main(int argc, char *argv[])
{
	const char *server_str = SERVER_IP;
	int server_port = SERVER_PORT;
	const char *request_arg = NULL;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-s") == 0)
		{
			if (i + 1 < argc)
			{
				server_str = argv[++i];
				continue;
			}
			fprintf(stderr, "Missing value for -s\n");
			return 1;
		}

		if (strcmp(argv[i], "-p") == 0)
		{
			if (i + 1 < argc)
			{
				server_port = atoi(argv[++i]);
				continue;
			}
			fprintf(stderr, "Missing value for -p\n");
			return 1;
		}

		if (strcmp(argv[i], "-r") == 0)
		{
			if (i + 1 < argc)
			{
				request_arg = argv[++i];
				continue;
			}
			fprintf(stderr, "Missing value for -r\n");
			return 1;
		}

		if (argv[i][0] != '-' && request_arg == NULL)
		{
			request_arg = argv[i];
			continue;
		}
	}

	if (!request_arg)
	{
		fprintf(stderr, "Use: %s [-s server] [-p port] [-r] \"type city\"\n", argv[0]);
		return 1;
	}

	if (server_port <= 0 || server_port > 65535)
	{
		fprintf(stderr, "Invalid port: %d\n", server_port);
		return 1;
	}

#if defined WIN32
	SetConsoleOutputCP(CP_UTF8);
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != NO_ERROR)
	{
		errorhandler("Error at WSAStartup() \n");
		return 0;
	}
#endif

	char server_hostname[256];
	char server_ip[16];

	if (resolve_dns(SERVER_IP, server_hostname, sizeof(server_hostname), server_ip, sizeof(server_ip)) != 0) {
		clearwinsock();
		return 1;
	}

	int my_socket;

	my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); // UDP

	if (my_socket < 0)
	{
		errorhandler("Socket creation failed \n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)server_port);
	server_addr.sin_addr.s_addr = inet_addr(server_ip);

	int msglen = (int)strlen(request_arg) + 1;
	int sent = sendto(my_socket,
					  request_arg,
					  msglen,
					  0,
					  (struct sockaddr *)&server_addr,
					  sizeof(server_addr)); // UDP

	if (sent < 0 || sent != msglen)
	{
		errorhandler("sendto() failed or sent different number of bytes \n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	weather_request_t req;
	parse_weather_request(request_arg, &req);

	int bytes_rcvd;
	char buf[BUFFER_SIZE];
	memset(buf, 0, BUFFER_SIZE);

	struct sockaddr_in from_addr;
	socklen_t from_len = sizeof(from_addr);

	bytes_rcvd = recvfrom(my_socket,
						  buf,
						  BUFFER_SIZE - 1,
						  0,
						  (struct sockaddr *)&from_addr,
						  &from_len); // UDP
	if (bytes_rcvd <= 0)
	{
		errorhandler("recvfrom() failed or no data received \n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}
	buf[bytes_rcvd] = '\0';

	weather_response_t res;
	parse_weather_response(buf, &res);

	printf("Ricevuto risultato dal server %s (ip %s). ", server_hostname,
		   inet_ntoa(from_addr.sin_addr));

	switch (res.status)
	{
	case 0:
		printf("%s: ", req.city);
		switch (res.type)
		{
		case 't':
			printf("Temperatura = %.2f°C \n", res.value);
			break;
		case 'h':
			printf("Umidità = %.2f %% \n", res.value);
			break;
		case 'w':
			printf("Vento = %.2f km/h \n", res.value);
			break;
		case 'p':
			printf("Pressione = %.2f hPa \n", res.value);
			break;
		}
		break;

	case 1:
		errorhandler("Città non disponibile \n");
		break;

	case 2:
		errorhandler("Richiesta non valida \n");
		break;
	}

	closesocket(my_socket);
	puts("Client terminated...");
	clearwinsock();

	return 0;
}
