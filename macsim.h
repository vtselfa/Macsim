#ifndef MACSIM_H
#define MACSIM_H

#include "linked-list.h"

#define MACSIM_VERBOSE

#ifdef MACSIM_VERBOSE
#  define macsim_trace_msg(...); macsim_trace_msg_(__VA_ARGS__);
#  define macsim_print(...); macsim_print_(__VA_ARGS__);
#else
#  define macsim_trace_msg(...); 
#  define macsim_print(...); 
#endif

#define MACSIM_UNKNOWN_STATION 0
#define MACSIM_SUCCESS 1
#define MACSIM_WAITING_STATION 2
#define MACSIM_USING_STATION 3

/* Estructuras */
struct macsim_station_t{
	char *name; //Nombre de la estación
	int reschedule : 1; //Marca de replanificación
	struct linked_list_t *clients; //Lista de clientes en la estación
	long long total_service_time; //Suma de los tiempos de servicio
	long long total_response_time; //Suma de los tiempos de respuesta
	long long total_clients; //Núm. clientes que han pasado por la estación
};

/* Prototipos */
void macsim_init();
void macsim_exit();
long long macsim_time_ns();
double macsim_time();
long long macsim_get_last_reset_time();
void macsim_schedule(int kind, long long client_id, double ms);
void macsim_schedule_ns(int kind, long long client_id, long long ns);
void macsim_extract(int *kind, long long *client_id);
struct macsim_station_t * macsim_station_create(char *name);
int macsim_station_delete(char *name);
struct macsim_station_t * macsim_station_get(char *name);
char * macsim_station_name(struct macsim_station_t *station);
int macsim_stations_count();
int macsim_station_queue_length(struct macsim_station_t *station);
int macsim_station_request(struct macsim_station_t *station, long long client_id);
int macsim_station_request2(char *name, long long client_id);
void macsim_station_leave(struct macsim_station_t *station, int client_id);
void macsim_station_leave2(char* name, int client_id);
double macsim_exponential(double mean);
double macsim_uniform(double a, double b); 
void macsim_reset_statistics();
void macsim_report();
void macsim_trace(int value);
void macsim_station_print(char* name);
void macsim_print_(int level, const char *fmt, ...);
void macsim_trace_msg_(int level, const char *fmt, ...);
   
#endif /* MACSIM_H */
