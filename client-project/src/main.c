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

		/* Allow optional command line: ./server-project [-p port]
		   -p port : optional port to listen on (overrides SERVER_PORT)
		*/
		int listen_port = SERVER_PORT;
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "-p") == 0)
			{
				if (i + 1 < argc)
				{
					listen_port = atoi(argv[++i]);
					if (listen_port <= 0 || listen_port > 65535)
					{
						fprintf(stderr, "Invalid port: %s\n", argv[i]);
						return 1;
					}
					continue;
				}
				fprintf(stderr, "Missing value for -p\n");
				return 1;
			}
			/* ignore unknown arguments */
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
	// COSTRUZIONE DELL'INDIRIZZO DEL SERVER
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = PF_INET; echoServAddr.sin_port = htons(PORT);
	echoServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	// TODO: Create UDP socket

	// TODO: Configure server address

	// TODO: Implement UDP communication logic

	// TODO: Close socket
	// closesocket(my_socket);

	printf("Client terminated.\n");

	clearwinsock();
	return 0;
} // main end
