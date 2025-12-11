/*
 * main.c
 *
 * UDP Server - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP server
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
#include "protocol.h"
#include <time.h>

void ErrorHandler(const char *errorMessage) {
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}

void init_random(void) {
	srand((unsigned int)time(NULL));
}

static float rand_range(float min, float max) {
	float r = (float)rand() / (float)RAND_MAX;
	return min + r * (max - min);
}

float get_temperature(void) {
	return rand_range(-10.0f, 40.0f);
}

float get_humidity(void) {
	return rand_range(20.0f, 100.0f);
}

float get_wind(void) {
	return rand_range(0.0f, 100.0f);
}

float get_pressure(void) {
	return rand_range(950.0f, 1050.0f);
}

int parse_weather_request(const char *input, weather_request_t *req) {
	if (!input || !req) return 2;

	req->type = input[0];

	if (req->type != 't' && req->type != 'h' && req->type != 'w' && req->type != 'p')
		return 2;

	const char *space = strchr(input, ' ');
	if (!space || *(space + 1) == '\0') return 2;

	strncpy(req->city, space + 1, sizeof(req->city) - 1);
	req->city[sizeof(req->city) - 1] = '\0';

	/* Validate city against small built-in list */
	const char *valid_cities[] = {"Bari", "Roma", "Milano", "Napoli", "Torino",
								  "Palermo", "Genova", "Bologna", "Firenze", "Venezia"};
	const int n = (int)(sizeof(valid_cities) / sizeof(valid_cities[0]));

	int found = 0;
	for (int i = 0; i < n; ++i) {
		/* case-insensitive compare */
		const char *a = req->city;
		const char *b = valid_cities[i];
		int equal = 1;
		while (*a && *b) {
			char ca = *a >= 'A' && *a <= 'Z' ? *a + ('a' - 'A') : *a;
			char cb = *b >= 'A' && *b <= 'Z' ? *b + ('a' - 'A') : *b;
			if (ca != cb) { equal = 0; break; }
			a++; b++;
		}
		if (equal && *a == '\0' && *b == '\0') { found = 1; break; }
	}

	if (!found) return 1; /* city not available */

	return 0;
}

int format_weather_response(const weather_response_t *resp, char *buffer, size_t size) {
	if (!resp || !buffer || size == 0) return 1;
	
	int written = snprintf(buffer, size, "%u %c %.2f", resp->status, resp->type, resp->value);
	if (written < 0 || (size_t)written >= size) return 1;
	
	return 0;
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

	// TODO: Implement server logic

#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif

	int my_socket;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned int cliAddrLen;
	char echoBuffer[ECHOMAX];
	int recvMsgSize;

	//CREAZIONE SOCKET
	if ((my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) ErrorHandler("socket() fallita");

	// COSTRUZIONE DELL'INDIRIZZO DEL SERVER
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET; echoServAddr.sin_port = htons(PORT);
	echoServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// BIND DELLA SOCKET
	if ((bind(my_socket, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr))) < 0)
		ErrorHandler("bind() fallita");

	// RICEZIONE DELLA STRINGA ECHO DAL CLIENT
	while (1) {
	    cliAddrLen = sizeof(echoClntAddr);

	    recvMsgSize = recvfrom(my_socket, echoBuffer, ECHOMAX, 0,
	                           (struct sockaddr*)&echoClntAddr, &cliAddrLen);

	    if (recvMsgSize == -1) {
	        perror("recvfrom error");
	        continue;
	    }
	    // Elaborazione della richiesta e creazione della risposta
	    weather_request_t request;
	    weather_response_t response;

	    // Parse della richiesta
	    if (recvMsgSize > 0) {
	        echoBuffer[recvMsgSize] = '\0';
	        
	        // Parse richiesta
	        int parse_result = parse_weather_request(echoBuffer, &request);
	        
			if (parse_result == 0) {
				response.status = 0; // OK (success)
	            response.type = request.type;
	            
	            // Generare valore meteo in base al tipo richiesto
	            switch(request.type) {
	                case 't': response.value = get_temperature(); break;
	                case 'h': response.value = get_humidity(); break;
	                case 'w': response.value = get_wind(); break;
	                case 'p': response.value = get_pressure(); break;
	                default: response.value = 0.0f;
	            }
			} else if (parse_result == 1) {
				response.status = 1; // city not available
				response.type = '?';
				response.value = 0.0f;
			} else {
				response.status = 2; // invalid request
				response.type = '?';
				response.value = 0.0f;
			}
	    }

	    // Invio response: serializzazione in buffer separato
	    char buffer[sizeof(uint32_t) + sizeof(char) + sizeof(float)];
	    int offset = 0;

	    // Serializza status (con network byte order)
	    uint32_t net_status = htonl(response.status);
	    memcpy(buffer + offset, &net_status, sizeof(uint32_t));
	    offset += sizeof(uint32_t);

	    // Serializza type (1 byte, no conversione)
	    memcpy(buffer + offset, &response.type, sizeof(char));
	    offset += sizeof(char);

	    // Serializza value (float con network byte order)
	    uint32_t temp;
	    memcpy(&temp, &response.value, sizeof(float));
	    temp = htonl(temp);
	    memcpy(buffer + offset, &temp, sizeof(float));
	    offset += sizeof(float);

	    // Invio del buffer
	    if (sendto(my_socket, buffer, offset, 0, (struct sockaddr*)&echoClntAddr, cliAddrLen) < 0) {
	        perror("sendto error");
	    }
	}

	// TODO: Implement UDP datagram reception loop 

	printf("Server terminato.\n");

	closesocket(my_socket);
	clearwinsock();
	return 0;
} // main end
