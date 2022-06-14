#ifndef PROTOS2022A_METRICS
#define PROTOS2022A_METRICS

void add_connection();
void remove_connection();
void add_total_bytes_transferred(unsigned long long bytes);

long get_historical_connections();
uint16_t get_concurrent_connections();
unsigned long long get_total_bytes_transferred();

#endif
