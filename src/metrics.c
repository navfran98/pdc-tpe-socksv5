#include <stdint.h>
#include "../headers/metrics.h"

long historical_connections = 0;
long concurrent_connections = 0;
unsigned long long total_bytes_transferred = 0;


void add_connection() {
	historical_connections ++;
	concurrent_connections ++;
}

void remove_connection() {
	concurrent_connections --;
}

void add_total_bytes_transferred(unsigned long long bytes) {
	total_bytes_transferred += bytes;
}

long get_historical_connections() {
	return historical_connections;
}

uint16_t get_concurrent_connections() {
	return concurrent_connections;
}

unsigned long long get_total_bytes_transferred() {
	return total_bytes_transferred;
}




