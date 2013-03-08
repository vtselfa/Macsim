#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "macsim.h"
#include "heap.h"
#include "hash-table.h"
#include "debug.h"
#include "random.h"

#define MACSIM_UNKNOWN_STATION 0
#define MACSIM_SUCCESS 1
#define MACSIM_WAITING_STATION 2
#define MACSIM_USING_STATION 3

/* Estructuras */
struct macsim_event_t{
	long long client;
	int kind;
};


struct macsim_station_client_t {
	long long id;             
	long long station_entry_time; //Instante de entrada a la estación
	long long server_entry_time; //Instante de entrada al servidor
	int event_kind; //Evento que causa el encolamiento 
}; 



/* Variables */
static long long current_time; //Instante actual en la simulación en nanosegundos (ns)
static long long last_reset_time; //Instante en que se produjo el último reset en nanosegundos (ns)
static int trace = 1; //Indica si la traza está activada o no
static struct heap_t *event_queue; //Cola de eventos
static struct hash_table_t *stations; //Estaciones
static int current_event; //Último evento sacado de la cola



/* Prototipos */
static void macsim_station_destroy(struct macsim_station_t *station);
void macsim_trace_msg_(int level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));


/* Funciones */
/* Inicialización de la librería */
void macsim_init(){
	event_queue = heap_create(512); //Tamaño inicial
	if(!event_queue)
		fatal("%s: out of memory", __func__);
	stations = hash_table_create(512, 1); //El 1 indica que las claves distinguen mayúsculas y minúsculas. 512 es el tamaño inicial.
	if(!stations)
		fatal("%s: out of memory", __func__);
}


/* Liberación de la memoria usada por la libreria */
void macsim_exit(){
	struct macsim_station_t *station;
	struct macsim_event_t *event;
	char *key;
	
	/* Destruir cola de enventos */
	for(heap_first(event_queue, (void**)&event); heap_next(event_queue, (void**)&event);){
		free(event);
	}
	heap_free(event_queue);
	
	/* Destruir estaciones y clientes */
	HASH_TABLE_FOR_EACH(stations, key, station){
		macsim_station_destroy(station);
	}
	hash_table_free(stations);
}


/* Retorna el instante en que se encuentra la simulación
 * @return Instante actual en ns*/
long long macsim_time_ns(){
	return current_time;
}


/* Retorna el instante en que se encuentra la simulación
 * @return Instante actual en ms*/
double macsim_time(){
	return current_time / 1000000.0;
}


long long macsim_get_last_reset_time(){
	return last_reset_time;
}


/* Insertar un evento planificado para dentro de \ms milisegundos.
 * El evendo insertado será de tipo \kind con id de cliente \client_id.
 * El uso de un double y pasar el tiempo en milisegundos busca evitarle al usuario tener que trabajar en nanosegundos, que es como internamente trabaja la librería. */
void macsim_schedule(int kind, long long client_id, double ms){
	struct macsim_event_t *event = (struct macsim_event_t *) calloc(1, sizeof(struct macsim_event_t));
	if(!event)
		fatal("%s: out of memory", __func__);
	event->client = client_id;
	event->kind = kind;
	heap_insert(event_queue, current_time + (long long) (ms * 1000000), event);
	if(heap_error(event_queue))
		fatal("%s: %s", __func__, heap_error_msg(event_queue));
}


/* Insertar un evento planificado para dentro de \ns nanosegundos
 * El evendo insertado será de tipo \kind con id de cliente \client_id */
void macsim_schedule_ns(int kind, long long client_id, long long ns){
	struct macsim_event_t *event = (struct macsim_event_t *) calloc(1, sizeof(struct macsim_event_t));
	if(!event)
		fatal("%s: out of memory", __func__);
	event->client = client_id;
	event->kind = kind;
	heap_insert(event_queue, current_time + ns, event);
	if(heap_error(event_queue))
		fatal("%s: %s", __func__, heap_error_msg(event_queue));
}


/* Extraer de la cola de eventos */
void macsim_extract(int *kind, long long *client_id){
	struct macsim_event_t *event;
	
	current_time = heap_extract(event_queue, (void**)&event); //Actualizar el instante actual
	if(heap_error(event_queue))
		fatal("%s: %s", __func__, heap_error_msg(event_queue));

	current_event = event->kind; //Actualizar el evento actual
	*kind = event->kind;
	*client_id = event->client;

	free(event);
}


/* Crear una estación nueva.
 * El nombre se usará como ID de la estación y tiene, por lo tanto, que ser único.
 * La librería se encarga de la gestión de la memória.
 * @return La nueva estación o NULL si ya existe. */
struct macsim_station_t * macsim_station_create(char *name){
	struct macsim_station_t *station = (struct macsim_station_t *) calloc(1, sizeof(struct macsim_station_t));

	if(!station)
		fatal("%s: out of memory", __func__);
	
	station->name = strdup(name); 
	if(!station->name)
		fatal("%s: out of memory", __func__);
	
	/* Crear cola de la estación */
	station->clients = linked_list_create();

	/* Solo insertamos si la estación no existe ya */
	if(!hash_table_get(stations, name)){
		hash_table_insert(stations, name, station);
		return station;
	}
	
	macsim_station_destroy(station);
	return NULL;
}


/* Elimina una estación.
 * La librería se encarga de la gestión de la memória.
 * @return MACSIM_UNKNOWN_STATION si la estación no existe y MACSIM_SUCCESS en caso contrario. */
int macsim_station_delete(char *name){
	struct macsim_station_t *station = (struct macsim_station_t *) hash_table_remove(stations, name);
	
	if(!station) // La estación no existe
		return MACSIM_UNKNOWN_STATION;
	
	macsim_station_destroy(station);
	return MACSIM_SUCCESS;
}


/* Función privada para liberar la memória usada por una estación */
static void macsim_station_destroy(struct macsim_station_t *station){
	struct macsim_station_client_t *client;
	LINKED_LIST_FOR_EACH(station->clients){
		client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		free(client);
	}
	free(station->name);
	free(station);
}


/* Devuelve, si existe, la estación con el nombre indicado
 * @return La estación o NULL si no existe */
struct macsim_station_t * macsim_station_get(char *name){
	return (struct macsim_station_t *) hash_table_get(stations, name);
}


/* Devuelve el nombre de la estación.
 * Este nombre NUNCA debe ser modificado por el usuario de la librería.
 * @return Nombre de la estación */
char * macsim_station_name(struct macsim_station_t *station){
	return station->name;
}

/* Devuelve el número de clientes en la cola de la estación
 * @return Número de clientes en cola */
int macsim_station_queue_length(struct macsim_station_t *station){
	return linked_list_count(station->clients);
}


/* Devuelve el número total de estaciones
 * @return El número de estaciones */
int macsim_num_stations(){
	return hash_table_count(stations);
}


/* El cliente solicita el uso de la estación
 * @return MACSIM_USING_STATION si la estación está vacía y el trabajo ha empezado a ejecutarse y MACSIM_WAITING_STATION si la estación está ocupada y el trabajo ha sido encolado */
int macsim_station_request(struct macsim_station_t *station, long long client_id){
	struct macsim_station_client_t *client;

	/* Estación desconocida */
	if(!station)
		fatal("%s: unknown station", __func__);	

	/* El cliente estaba esperando en la cola y es ya su turno */
	if(station->reschedule){
		linked_list_head(station->clients);
		client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		if(client->id == client_id){ /* El reschedule es para nosotros? */
			client->server_entry_time = current_time; //Estadísticas
			station->reschedule = 0;
			macsim_trace_msg(1, "El cliente %lld entra en la estación \"%s\", en la que estaba encolado", client->id, station->name);
			return MACSIM_USING_STATION;
		}
	}

	/* Crear cliente */
	client = (struct macsim_station_client_t *) calloc(1, sizeof(struct macsim_station_client_t));
	if(!client)
		fatal("%s: out of memory", __func__);	
	client->id = client_id;
	client->event_kind = current_event;

	/* Encolar cliente al final de la cola */
	linked_list_add(station->clients, client);
	if(station->clients->error_code) //Ha habido un error
		fatal("%s: can't add client", __func__);

	client->station_entry_time = current_time; //Estadísticas
	
	/* La estación tiene clientes en la cola */
	if(linked_list_count(station->clients) > 1){
		macsim_trace_msg(1, "El cliente %lld se encola en la estación \"%s\"", client->id, station->name);
		return MACSIM_WAITING_STATION;
	}
	
	/* La estación está vacía así que el cliente entra en el servidor */
	client->server_entry_time = current_time; //Estadísticas
	macsim_trace_msg(1, "El cliente %lld entra en la estación \"%s\"", client->id, station->name);
	return MACSIM_USING_STATION;
}


/* El cliente solicita el uso de la estación de la que se pasa el nombre.
 * Más lenta que macsim_station_request(...).
 * @return MACSIM_USING_STATION si la estación está vacía y el trabajo ha empezado a ejecutarse y MACSIM_WAITING_STATION si la estación está ocupada y el trabajo ha sido encolado */
int macsim_station_request2(char *name, long long client_id){
	struct macsim_station_t *station = macsim_station_get(name);
	struct macsim_station_client_t *client;

	/* Estación desconocida */
	if(!station)
		fatal("%s: unknown station", __func__);	

	/* El cliente estaba esperando en la cola y es ya su turno */
	if(station->reschedule){
		linked_list_head(station->clients);
		client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		if(client->id == client_id){ /* El reschedule es para nosotros? */
			client->server_entry_time = current_time; //Estadísticas
			station->reschedule = 0;
			macsim_trace_msg(1, "El cliente %lld entra en la estación \"%s\", en la que estaba encolado", client->id, station->name);
			return MACSIM_USING_STATION;
		}
	}

	/* El cliente ya está en la estación */
	LINKED_LIST_FOR_EACH(station->clients){
		client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		if(client->id == client_id)
			fatal("%s: client already in queue", __func__);	
	}


	/* Crear cliente */
	client = (struct macsim_station_client_t *) calloc(1, sizeof(struct macsim_station_client_t));
	if(!client)
		fatal("%s: out of memory", __func__);	
	client->id = client_id;
	client->event_kind = current_event;

	/* Encolar cliente al final de la cola */
	linked_list_add(station->clients, client);
	if(station->clients->error_code) //Ha habido un error
		fatal("%s: can't add client", __func__);

	client->station_entry_time = current_time; //Estadísticas
	
	/* La estación tiene clientes en la cola */
	if(linked_list_count(station->clients) > 1){
		macsim_trace_msg(1, "El cliente %lld se encola en la estación \"%s\"", client->id, station->name);
		return MACSIM_WAITING_STATION;
	}
	
	/* La estación está vacía así que el cliente entra en el servidor */
	client->server_entry_time = current_time; //Estadísticas
	macsim_trace_msg(1, "El cliente %lld entra en la estación \"%s\"", client->id, station->name);
	return MACSIM_USING_STATION;
}


/* El cliente abandona la estación. */
void macsim_station_leave(struct macsim_station_t *station, int client_id){
	struct macsim_station_client_t *client, *next_client;
	int queued_clients;

	if(!station)
		fatal("%s: unknown station", __func__);	

	queued_clients = linked_list_count(station->clients);	
	if(!queued_clients)
		fatal("%s: empty station queue", __func__);
	
	linked_list_head(station->clients);
	client = (struct macsim_station_client_t *) linked_list_get(station->clients);
	linked_list_remove(station->clients);
	
	/* Comprobar que todo va bien */
	if(client->id != client_id)
		fatal("%s: client id missmatch", __func__);

	/* Atender al siguiente cliente, si lo hay */
	queued_clients--;
	if(queued_clients){
		linked_list_head(station->clients);
		next_client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		macsim_schedule_ns(client->event_kind, next_client->id, 0);
		station->reschedule = 1;
	}

	/* Estadísticas */
	station->total_clients++;
	station->total_response_time += current_time - client->station_entry_time;
	station->total_service_time += current_time - client->server_entry_time;

	macsim_trace_msg(1, "El cliente %lld sale de la estación \"%s\" tresp = %f tserv = %f", client->id, station->name, (current_time - client->station_entry_time) / 1000000.0, (current_time - client->server_entry_time) / 1000000.0);

	free(client);
}


/* El cliente abandona la estación.
 * Más lenta que macsim_station_leave. */
void macsim_station_leave2(char* name, int client_id){
	struct macsim_station_t *station = macsim_station_get(name);
	struct macsim_station_client_t *client, *next_client;
	int queued_clients;

	if(!station)
		fatal("%s: unknown station", __func__);	

	queued_clients = linked_list_count(station->clients);	
	if(!queued_clients)
		fatal("%s: empty station queue", __func__);
	
	linked_list_head(station->clients);
	client = (struct macsim_station_client_t *) linked_list_get(station->clients);
	linked_list_remove(station->clients);
	
	/* Comprobar que todo va bien */
	if(client->id != client_id)
		fatal("%s: client id missmatch", __func__);

	/* Atender al siguiente cliente, si lo hay */
	queued_clients--;
	if(queued_clients){
		linked_list_head(station->clients);
		next_client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		macsim_schedule_ns(client->event_kind, next_client->id, 0);
		station->reschedule = 1;
	}

	/* Estadísticas */
	station->total_clients++;
	station->total_response_time += current_time - client->station_entry_time;
	station->total_service_time += current_time - client->server_entry_time;

	macsim_trace_msg(1, "El cliente %lld sale de la estación \"%s\" tresp = %f tserv = %f", client->id, station->name, (current_time - client->station_entry_time) / 1000000.0, (current_time - client->server_entry_time) / 1000000.0);

	free(client);
}


/* Genera un número siguiendo una dist. exponencial con la media pasada como parámetro
 * @return Número generado siguiendo una dist. exponencial */
double macsim_exponential(double mean){
	return(-mean * log(macsim_random(0)));
}


/* Genera un número aletorio entre a y b
 * @return Número aleatorio entre a y b */
double macsim_uniform(double a, double b){ 
	double c; 
	if (a>b){
		c = a;
		a = b;
		b = c;
	}
	return( a + (b-a) * macsim_random(0));
}


/* Resetea las estadísticas de las estaciones.
 * Útil para eliminar el transitorio. */
void macsim_reset_statistics(){
	char *key;
	struct macsim_station_t *station;
	
	HASH_TABLE_FOR_EACH(stations, key, station){
		station->total_clients = 0;
		station->total_response_time = 0;
		station->total_service_time = 0;
	}

	last_reset_time = current_time;
}


/* Imprime estadísticas por la salida estandar */
void macsim_report(){
	char *key;
	double serv, resp, queue, thro, util=0;
	struct macsim_station_t *station;

	printf("\n");
	printf("RESULTADOS DE LA SIMULACIÓN\n");
	HASH_TABLE_FOR_EACH(stations, key, station){
		serv = station->total_service_time / station->total_clients;
		resp = station->total_response_time / station->total_clients;
		queue = resp - serv;
		thro = station->total_clients / (double) (current_time - last_reset_time) * 1000000;
		util = thro / (1000000.0/serv);
		printf("\n");
		printf("ESTACION: %s\n", station->name);
		printf("Tiempo de servicio    Tiempo de respuesta   Tiempo en cola        Total clientes        Productividad         Utilización\n");
		printf("%-20.4f  %-20.4f  %-20.4f  %-20lld  %-20.4f  %-20.4f\n", serv/1000000.0, resp/1000000.0, queue/1000000.0, station->total_clients, thro, util);
		printf("\n");
	}
}


void macsim_trace_msg_(int level, const char *fmt, ...){
	if(!trace) return; /* Traza desactivada */
	/* Se imprimen los mensajes que tienen un nivel MAYOR O IGUAL que nivel de traza
	 * traza == 1 en principio se reserva para los mensajes de la librería 
	 * traza > 1 puede usarse a discreción del usuario */
	if(level >= trace){
		va_list va;
		va_start(va, fmt);
		fprintf(stderr, "%f ", macsim_time());
		vfprintf(stderr, fmt, va);
		fprintf(stderr, "\n");
		fflush(NULL);
	}
}


void macsim_print_(int level, const char *fmt, ...){
	if(!trace) return; /* Traza desactivada */
	/* Se imprimen los mensajes que tienen un nivel MAYOR O IGUAL que nivel de traza
	 * traza == 1 en principio se reserva para los mensajes de la librería 
	 * traza > 1 puede usarse a discreción del usuario */
	if(level >= trace){
		va_list va;
		va_start(va, fmt);
		vfprintf(stderr, fmt, va);
		fflush(NULL);
	}
}


/* Activa o desactiva la traza */
void macsim_trace(int value){
	trace = value;
}


void macsim_station_print(char* name){
	struct macsim_station_t *station = macsim_station_get(name);
	struct macsim_station_client_t *client;
	LINKED_LIST_FOR_EACH(station->clients){
		client = (struct macsim_station_client_t *) linked_list_get(station->clients);
		printf("%lld ", client->id);
	}
	printf("\n");
}
