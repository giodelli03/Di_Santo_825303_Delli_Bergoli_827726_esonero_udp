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
#include <ctype.h>
#include <time.h>
#include "protocol.h"

void ErrorHandler(const char *errorMessage) {
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}

#define NO_ERROR 0

#define ECHOMAX 255
#define PORT 48000

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}


int main(int argc, char *argv[]) {

	// TODO: Implement client logic

#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif


	/* Print error messages to stderr */
	static void errorhandler(const char *errorMessage)
	{
		fprintf(stderr, "%s", errorMessage);
	}

	/* Case-insensitive exact string comparison. Returns 1 if equal. */
	static int stricmp_ci(const char *a, const char *b)
	{
		while (*a && *b)
		{
			if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
				return 0;
			a++;
			b++;
		}
		return *a == '\0' && *b == '\0';
	}

	/* Check whether city is in a small built-in list */
	static int is_valid_city(const char *city)
	{
		const char *valid_cities[] = {"Bari", "Roma", "Milano", "Napoli", "Torino",
									  "Palermo", "Genova", "Bologna", "Firenze", "Venezia"};
		const int n = (int)(sizeof(valid_cities) / sizeof(valid_cities[0]));

		for (int i = 0; i < n; i++)
		{
			if (stricmp_ci(city, valid_cities[i]))
				return 1;
		}
		return 0;
	}

	/* Return canonical capitalization for a known city (or input if not found) */
	static const char *get_canonical_city(const char *city)
	{
		if (!city) return city;
		const char *valid_cities[] = {"Bari", "Roma", "Milano", "Napoli", "Torino",
									  "Palermo", "Genova", "Bologna", "Firenze", "Venezia"};
		const int n = (int)(sizeof(valid_cities) / sizeof(valid_cities[0]));
		for (int i = 0; i < n; ++i) {
			const char *a = city;
			const char *b = valid_cities[i];
			int equal = 1;
			while (*a && *b) {
				char ca = *a >= 'A' && *a <= 'Z' ? *a + ('a' - 'A') : *a;
				char cb = *b >= 'A' && *b <= 'Z' ? *b + ('a' - 'A') : *b;
				if (ca != cb) { equal = 0; break; }
				a++; b++;
			}
			if (equal && *a == '\0' && *b == '\0') return valid_cities[i];
		}
		return city;
	}

	/*
	 * Parse a request string like: "t Bari".
	 * return codes: 0 OK, 1 city not available, 2 invalid request
	 */
	int parse_weather_request(const char *input, weather_request_t *req)
	{
		if (!input || !req)
			return 2;

		req->type = input[0];

		/* Validate type */
		if (req->type != 't' && req->type != 'h' && req->type != 'w' && req->type != 'p')
			return 2;

		const char *space = strchr(input, ' ');
		if (!space || *(space + 1) == '\0')
			return 2;

		strncpy(req->city, space + 1, sizeof(req->city) - 1);
		req->city[sizeof(req->city) - 1] = '\0';

		if (!is_valid_city(req->city))
			return 1;

		return 0;
	}

	/// @brief Converte la struct in una stringa: "<status> <type> <value>"
	/// @param resp struct con i dati
	/// @param buffer stringa di output
	/// @param size dimensione del buffer
	/// @return 0 se ok, 1 se errore
	/* Format a response into `buffer`. Returns 0 on success. */
	int format_weather_response(const weather_response_t *resp, char *buffer, size_t size)
	{
		if (!resp || !buffer || size == 0)
			return 1;

		int written = snprintf(buffer, size, "%u %c %.2f", resp->status, resp->type, resp->value);
		if (written < 0 || (size_t)written >= size)
			return 1; /* buffer too small */

		return 0;
	}

	/* Initialize RNG once at startup */
	void init_random(void)
	{
		srand((unsigned int)time(NULL));
	}

	/* Generate random float in [min,max] */
	static float rand_range(float min, float max)
	{
		float r = (float)rand() / (float)RAND_MAX; /* 0.0 -> 1.0 */
		return min + r * (max - min);
	}

	float get_temperature(void)
	{
		return rand_range(-10.0f, 40.0f);
	}

	float get_humidity(void)
	{
		return rand_range(20.0f, 100.0f);
	}

	float get_wind(void)
	{
		return rand_range(0.0f, 100.0f);
	}

	float get_pressure(void)
	{
		return rand_range(950.0f, 1050.0f);
	}

	int main(int argc, char *argv[])
	{

		/* Parse command line options:
		   -s <server> : server hostname or IP (optional, default "localhost")
		   -r "request": request string like "t Bari" (optional - if present send once and exit)
		*/
		char server_spec[128];
		strncpy(server_spec, "localhost", sizeof(server_spec));
		server_spec[sizeof(server_spec)-1] = '\0';
		char cli_request[256] = "";
		int have_cli_request = 0;

		for (int i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-s") == 0) {
				if (i + 1 < argc) {
					strncpy(server_spec, argv[++i], sizeof(server_spec)-1);
					server_spec[sizeof(server_spec)-1] = '\0';
					continue;
				}
				fprintf(stderr, "Missing value for -s\n");
				return 1;
			}
			if (strcmp(argv[i], "-r") == 0) {
				if (i + 1 < argc) {
					strncpy(cli_request, argv[++i], sizeof(cli_request)-1);
					cli_request[sizeof(cli_request)-1] = '\0';
					have_cli_request = 1;
					continue;
				}
				fprintf(stderr, "Missing value for -r\n");
				return 1;
			}
		}


	#if defined WIN32
		SetConsoleOutputCP(CP_UTF8);
		// Initialize Winsock
		WSADATA wsa_data;
		int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
		if (result != NO_ERROR)
		{
			errorhandler("Error at WSAStartup() \n");
			return 0;
		}
	#endif

	int my_socket;

	// CREAZIONE DELLA SOCKET
	if ((my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		ErrorHandler("socket() failed");

	// Risolvi server_spec -> indirizzo IPv4 (forward lookup)
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; /* IPv4 only */
	hints.ai_socktype = SOCK_DGRAM;

	int gai = getaddrinfo(server_spec, NULL, &hints, &res);
	if (gai != 0 || res == NULL) {
		fprintf(stderr, "Errore risoluzione server '%s': %s\n", server_spec, gai_strerror(gai));
		closesocket(my_socket);
		return 1;
	}

	struct sockaddr_in serv_sockaddr;
	memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
	serv_sockaddr.sin_family = AF_INET;
	serv_sockaddr.sin_port = htons(PORT);
	serv_sockaddr.sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr; /* copy IPv4 address */

	/* Obtain textual IP */
	char ipstr[INET_ADDRSTRLEN];
#if defined WIN32
	strcpy(ipstr, inet_ntoa(serv_sockaddr.sin_addr));
#else
	inet_ntop(AF_INET, &serv_sockaddr.sin_addr, ipstr, sizeof(ipstr));
#endif

	/* Reverse lookup: get canonical name from IP */
	char hostbuf[NI_MAXHOST];
	int rv = getnameinfo((struct sockaddr *)&serv_sockaddr, sizeof(serv_sockaddr), hostbuf, sizeof(hostbuf), NULL, 0, 0);
	char resolved_name[NI_MAXHOST];
	if (rv == 0) {
		strncpy(resolved_name, hostbuf, sizeof(resolved_name)-1);
		resolved_name[sizeof(resolved_name)-1] = '\0';
	} else {
		/* fallback: use server_spec as name */
		strncpy(resolved_name, server_spec, sizeof(resolved_name)-1);
		resolved_name[sizeof(resolved_name)-1] = '\0';
	}

	freeaddrinfo(res);

	// Buffers and server address length
	char request_str[256];
	char response_buffer[sizeof(uint32_t) + sizeof(char) + sizeof(float)];
	int response_size;
	struct sockaddr_in server_addr;
	unsigned int server_addr_len = sizeof(server_addr);

	// If CLI provided request (-r), send once and exit after printing
	if (have_cli_request) {
		strncpy(request_str, cli_request, sizeof(request_str)-1);
		request_str[sizeof(request_str)-1] = '\0';

		if (sendto(my_socket, request_str, (int)strlen(request_str), 0,
				   (struct sockaddr *)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
			perror("sendto error");
			closesocket(my_socket);
			return 1;
		}

		response_size = recvfrom(my_socket, response_buffer, sizeof(response_buffer), 0,
								 (struct sockaddr *)&server_addr, &server_addr_len);
		if (response_size < 0) {
			perror("recvfrom error");
			closesocket(my_socket);
			return 1;
		}

		// Deserialize
		if (response_size >= (int)(sizeof(uint32_t)+sizeof(char)+sizeof(float))) {
			int offset = 0;
			uint32_t net_status; memcpy(&net_status, response_buffer + offset, sizeof(uint32_t));
			uint32_t status = ntohl(net_status); offset += sizeof(uint32_t);
			char type; memcpy(&type, response_buffer + offset, sizeof(char)); offset += sizeof(char);
			uint32_t tmp; memcpy(&tmp, response_buffer + offset, sizeof(float)); tmp = ntohl(tmp);
			float value; memcpy(&value, &tmp, sizeof(float));

			weather_request_t req;
			parse_weather_request(request_str, &req); // to get city (may return error but req.city filled)
			const char *cityname = get_canonical_city(req.city);

			// Print according to status and type with exact format
			if (status == 0) {
				switch (type) {
					case 't':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Temperatura = %.1f°C\n", resolved_name, ipstr, cityname, value);
						break;
					case 'h':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Umidità = %.1f%%\n", resolved_name, ipstr, cityname, value);
						break;
					case 'w':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Vento = %.1f km/h\n", resolved_name, ipstr, cityname, value);
						break;
					case 'p':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Pressione = %.1f hPa\n", resolved_name, ipstr, cityname, value);
						break;
					default:
						printf("Ricevuto risultato dal server %s (ip %s). %s: Richiesta non valida\n", resolved_name, ipstr, cityname);
				}
			} else if (status == 1) {
				printf("Ricevuto risultato dal server %s (ip %s). Città non disponibile\n", resolved_name, ipstr);
			} else if (status == 2) {
				printf("Ricevuto risultato dal server %s (ip %s). Richiesta non valida\n", resolved_name, ipstr);
			} else {
				printf("Ricevuto risultato dal server %s (ip %s). Richiesta non valida\n", resolved_name, ipstr);
			}
		} else {
			fprintf(stderr, "Risposta di dimensione errata: %d bytes\n", response_size);
		}

		closesocket(my_socket);
		clearwinsock();
		return 0;
	}

	// Interactive loop (same behavior as previous, but now with resolved server name/ip)
	while (1) {
		printf("Inserisci richiesta meteo (es. 't Roma') o 'quit' per uscire: ");
		if (fgets(request_str, sizeof(request_str), stdin) == NULL) break;

		// Rimuovi newline
		size_t len = strlen(request_str);
		if (len > 0 && request_str[len-1] == '\n') request_str[len-1] = '\0';
		if (strcmp(request_str, "quit") == 0) break;

		if (sendto(my_socket, request_str, (int)strlen(request_str), 0,
				   (struct sockaddr *)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
			perror("sendto error");
			continue;
		}

		response_size = recvfrom(my_socket, response_buffer, sizeof(response_buffer), 0,
								 (struct sockaddr *)&server_addr, &server_addr_len);
		if (response_size < 0) {
			perror("recvfrom error");
			continue;
		}

		if (response_size >= (int)(sizeof(uint32_t)+sizeof(char)+sizeof(float))) {
			int offset = 0;
			uint32_t net_status; memcpy(&net_status, response_buffer + offset, sizeof(uint32_t));
			uint32_t status = ntohl(net_status); offset += sizeof(uint32_t);
			char type; memcpy(&type, response_buffer + offset, sizeof(char)); offset += sizeof(char);
			uint32_t tmp; memcpy(&tmp, response_buffer + offset, sizeof(float)); tmp = ntohl(tmp);
			float value; memcpy(&value, &tmp, sizeof(float));

			weather_request_t req;
			parse_weather_request(request_str, &req);
			const char *cityname = get_canonical_city(req.city);

			if (status == 0) {
				switch (type) {
					case 't':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Temperatura = %.1f°C\n", resolved_name, ipstr, cityname, value);
						break;
					case 'h':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Umidità = %.1f%%\n", resolved_name, ipstr, cityname, value);
						break;
					case 'w':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Vento = %.1f km/h\n", resolved_name, ipstr, cityname, value);
						break;
					case 'p':
						printf("Ricevuto risultato dal server %s (ip %s). %s: Pressione = %.1f hPa\n", resolved_name, ipstr, cityname, value);
						break;
					default:
						printf("Ricevuto risultato dal server %s (ip %s). %s: Richiesta non valida\n", resolved_name, ipstr, cityname);
				}
			} else if (status == 1) {
				printf("Ricevuto risultato dal server %s (ip %s). Città non disponibile\n", resolved_name, ipstr);
			} else if (status == 2) {
				printf("Ricevuto risultato dal server %s (ip %s). Richiesta non valida\n", resolved_name, ipstr);
			} else {
				printf("Ricevuto risultato dal server %s (ip %s). Richiesta non valida\n", resolved_name, ipstr);
			}
		} else {
			fprintf(stderr, "Risposta di dimensione errata: %d bytes\n", response_size);
		}
	}

	// Chiusura socket
	closesocket(my_socket);

	printf("Client terminated.\n");

	clearwinsock();
	return 0;
} // main end
