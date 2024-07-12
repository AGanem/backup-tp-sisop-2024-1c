#include "../include/planificador.h"
// ver si tengo que incluir la libreria donde esta el pcb
//#include "../include/servidorCpu.c"
//#include "../include/servidorCpu.h"

sem_t sem_contexto_ejecucion_recibido;
sem_t sem_confirmacion_memoria;
t_temporal* cronometro;

// Devuelve un t_algoritmo a partir de la config cargada
t_algoritmo_planificacion obtener_algoritmo_planificador(char* algoritmo_planificacion) {
    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        return FIFO;
    } else if (strcmp(algoritmo_planificacion, "ROUND ROBIN") == 0) {
        return ROUND_ROBIN;
    } else if (strcmp(algoritmo_planificacion, "VIRTUAL ROUND ROBIN") == 0) {
        return VIRTUAL_ROUND_ROBIN;
    } else {
        // Manejo de error para tipos de algoritmos desconocidos
        return -1;
    }
}

// detiene tanto el planificador de corto plazo como el de largo plazo
void detener_planificacion(t_planificador* planificador) {
    planificador->planificacion_detenida = true;
}

//Inicializa un nuevo planificador

t_planificador* inicializar_planificador(t_algoritmo_planificacion algoritmo, int quantum,int grado_multiprogramacion) {

    t_planificador* planificador = malloc(sizeof(t_planificador));
    planificador->cola_new = list_create();
    planificador->cola_ready = list_create();
    planificador->cola_exec = list_create();
    planificador->cola_blocked = dictionary_create();
    planificador->cola_exit = list_create();
    planificador->algoritmo = algoritmo;
    planificador->quantum = quantum;
    planificador->grado_multiprogramacion = grado_multiprogramacion;
    planificador->grado_multiprogramacion_actual = 0;
    planificador->planificacion_detenida = false; // Inicializar planificación como no detenida
    return planificador;
}

// Destruye el planificador y libera la memoria
void destruir_planificador(t_planificador* planificador) {
    list_destroy_and_destroy_elements(planificador->cola_new, free);
    list_destroy_and_destroy_elements(planificador->cola_ready, free);
    list_destroy_and_destroy_elements(planificador->cola_exec, free);
    list_destroy_and_destroy_elements(planificador->cola_blocked, free);
    list_destroy_and_destroy_elements(planificador->cola_exit, free);
    free(planificador);
}

// Agrega un nuevo proceso al planificador
bool agregar_proceso(t_planificador* planificador, t_pcb* proceso) {
    list_add(planificador->cola_new, proceso);
    if (planificador->grado_multiprogramacion_actual < planificador->grado_multiprogramacion) {
        t_pcb* proceso_nuevo = list_remove(planificador->cola_new, 0);
        list_add(planificador->cola_ready, proceso_nuevo);
        planificador->grado_multiprogramacion_actual++;
    }
    return true;
}

// Obtiene el próximo proceso a ejecutar
t_pcb* obtener_proximo_proceso(t_planificador* planificador) {
    t_pcb* proceso;
    if (planificador->algoritmo == FIFO) {
        proceso = list_remove(planificador->cola_ready, 0);
    } else if (planificador->algoritmo == ROUND_ROBIN) {
        proceso = list_get(planificador->cola_ready, 0);
        list_remove(planificador->cola_ready, 0);
        list_add(planificador->cola_ready, proceso);
    } else { // Virtual Round Robin
        proceso = list_remove(planificador->cola_ready, 0);
        if (proceso->tiempo_ejecucion >= planificador->quantum) {
            proceso->tiempo_ejecucion -= planificador->quantum;
            list_add(planificador->cola_ready, proceso);
        }
    }
    //TODO:ENVIAR ANTES A CPU?
    list_add(planificador->cola_exec, proceso);
    return proceso;
}

// Desaloja un proceso de la cola de ejecución y lo pone en la cola de listos
void desalojar_proceso(t_planificador* planificador, t_pcb* proceso) {
    list_remove(planificador->cola_exec, proceso);
    list_add(planificador->cola_ready, proceso);
}

//Bloquea un proceso y lo mueve a la cola de bloqueados
void bloquear_proceso(t_planificador* planificador, t_pcb* proceso, char* nombre_lista) {
    list_remove(planificador->cola_exec, proceso);
    dictionary_put(planificador->cola_blocked,nombre_lista,proceso);
}

//  Desbloquea un proceso y lo mueve a la cola de listos
void desbloquear_proceso(t_planificador* planificador, t_pcb* proceso, char* nombre_lista) {
    t_list* lista_a_desbloquear = malloc(sizeof(t_list));
    lista_a_desbloquear = dictionary_remove(planificador->cola_blocked,nombre_lista);
    uint32_t indice_a_desbloquear = malloc(sizeof(uint32_t));
    indice_a_desbloquear = encontrar_indice_proceso_pid(lista_a_desbloquear,proceso);
    list_remove_and_destroy_element(lista_a_desbloquear,indice_a_desbloquear,list_destroy);
    dictionary_put(planificador->cola_blocked,nombre_lista,lista_a_desbloquear);
    if (!planificador->planificacion_detenida) {
        list_add(planificador->cola_ready, proceso);
    }
}

// Finaliza un proceso y libera su memoria
void finalizar_proceso(t_planificador* planificador, t_pcb* proceso) {
    //uint32_t indice_proceso_a_finalizar = malloc(sizeof(uint32_t));
    //indice_proceso_a_finalizar = encontrar_indice_proceso_pid(planificador->cola_exec,proceso);
    //list_remove(planificador->cola_exec, indice_proceso_a_finalizar);
    //list_add(planificador->cola_exit, proceso);
    //Liberar los recursos del proceso
    char* pid_string = malloc(sizeof(proceso->pid));
    pid_string = sprintf(pid_string, "%u", proceso->pid);
    t_proceso_recurso_diccionario* proceso_recurso = malloc(sizeof(t_proceso_recurso_diccionario));
    proceso_recurso = dictionary_get(procesos_recursos,pid_string);
    for (size_t i = 0; i < proceso_recurso->nombres_recursos->elements_count; i++)
    {
        //buscar en que indice de la tabla general de recursos esta
        uint32_t indice_recurso_buscado = buscar_indice_recurso(cfg_kernel->RECURSOS,list_get(proceso_recurso->nombres_recursos,i)); 
        
        //sumo cant instancias correspondientes a lista de instancias global del recurso correspondiente
        uint32_t instancias_generales_actuales = malloc(sizeof(uint32_t));
        uint32_t instancias_proceso = malloc(sizeof(uint32_t));
        instancias_generales_actuales = list_get(cfg_kernel->INSTANCIAS_RECURSOS,indice_recurso_buscado);
        instancias_proceso = list_get(proceso_recurso->instancias_recursos,i);
        list_replace(cfg_kernel->INSTANCIAS_RECURSOS,indice_recurso_buscado,instancias_generales_actuales + instancias_proceso);
    }
    
    //borrar valor del diccionario para pid correspondiente
    dictionary_remove_and_destroy(procesos_recursos,pid_string,free);//TODO: reemplazar free por funcion que borre la esttructura y las listas que lo componen
    free(proceso);
     planificador->grado_multiprogramacion_actual--;
    if (!list_is_empty(planificador->cola_new) && !planificador->planificacion_detenida) {
        t_pcb* proceso_nuevo = list_remove(planificador->cola_new, 0);
        list_add(planificador->cola_ready, proceso_nuevo);
        planificador->grado_multiprogramacion_actual++;
    }
}

// Crea un nuevo proceso
void crear_proceso(t_planificador* planificador, char* path_pseudocodigo) {
    t_pcb* nuevo_proceso = crear_pcb(path_pseudocodigo);
    agregar_proceso(planificador, nuevo_proceso);
    // Notificar a la memoria para crear el proceso
}

void eliminar_proceso(t_planificador* planificador, t_pcb* proceso) {
    //ESTA INCLUIDA LO DE LISTAS DE LAS COMMONS?
    if (list_contains(planificador->cola_exec, proceso->pid)) {
        enviar_interrupcion_a_cpu(proceso,ELIMINAR_PROCESO,conexion_cpu_interrupt);
        
        // Esperar a que la CPU retorne el Contexto de Ejecución
        sem_wait(&sem_contexto_ejecucion_recibido);

        // Obtener el contexto de ejecución actualizado
        //t_pcb* pcb_actualizado = recibir_pcb(conexion_cpu_dispatch);

        // Actualizar el proceso con el contexto de ejecución recibido
        //actualizar_proceso(proceso, pcb_actualizado);
        proceso = pcb_actualizado_interrupcion;

        // Liberar memoria
        
        //eliminar_paquete(paquete);
        //free(buffer->stream);
        //free(buffer);
        //free(pcb_actualizado);
    }

   liberar_proceso_memoria(proceso->pid);

    // Esperar confirmación de la memoria
    sem_wait(&sem_confirmacion_memoria);

    // Liberar memoria
   // eliminar_paquete(paquete_memoria);
    //free(buffer_memoria->stream);
   // free(buffer_memoria);

    // Finalizar el proceso en el planificador
    finalizar_proceso(planificador, proceso);
}

uint32_t encontrar_indice_proceso_pid(t_list * lista_procesos , t_pcb* pcb) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_pcb* proceso = list_get(lista_procesos, i);
        if (proceso->pid == pcb->pid) {
            return i;
        }
    }
    return NULL;
}

void enviar_interrupcion_a_cpu(t_pcb* proceso,motivo_interrupcion motivo_interrupcion, int conexion){
    // Enviar señal de interrupción a la CPU
        t_paquete* paquete = crear_paquete(INTERRUPCION_KERNEL);

        // Serializar el proceso interrumpido
        //buffer = proceso_interrumpido_serializar(proceso_interrumpido);
        agregar_a_paquete(paquete, &(proceso->pid), sizeof(uint32_t));
        agregar_a_paquete(paquete, &motivo_interrupcion, sizeof(uint32_t));

        enviar_paquete(paquete, conexion); 

        free(paquete); 

}

void liberar_proceso_memoria(uint32_t pid){
     // Notificar a la memoria para liberar las estructuras del proceso
    t_paquete* paquete_memoria = crear_paquete(FINALIZAR_PROCESO);
    //t_buffer* buffer_memoria = crear_buffer();

    // Serializar el PID del proceso a liberar
    //buffer_write_uint32(buffer_memoria, proceso->pid);
    agregar_a_paquete(paquete_memoria, &pid, sizeof(uint32_t));
   // agregar_a_paquete(paquete_memoria, buffer_memoria->stream, buffer_memoria->size);

    // Enviar el paquete a la memoria
    enviar_paquete(paquete_memoria, conexion_memoria);
}

bool list_contains(t_list* lista_de_procesos, uint32_t pid){

    t_pcb* pcb_a_recorrer = malloc(sizeof(t_pcb));

    for (size_t i = 0; i < lista_de_procesos->elements_count; i++)
    {
        pcb_a_recorrer = list_get(lista_de_procesos,i);
        if(pcb_a_recorrer->pid == pid){
            return true;
        }
    }
    
    return false;
}

void poner_en_cola_exit(t_pcb* proceso){
    uint32_t indice_proceso_a_finalizar = malloc(sizeof(uint32_t));
    indice_proceso_a_finalizar = encontrar_indice_proceso_pid(planificador->cola_exec,proceso);
    list_remove(planificador->cola_exec, indice_proceso_a_finalizar);
    list_add(planificador->cola_exit, proceso);
}

void enviar_proceso_a_cpu(t_pcb* pcb, int conexion){

   t_paquete* paquete_archivo_nuevo = malloc(sizeof(t_paquete));
    
    paquete_archivo_nuevo = crear_paquete(NUEVO_PROCESO);
    
    agregar_a_paquete(paquete_archivo_nuevo, &(pcb->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &(pcb->program_counter), sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &(pcb->path_length), sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, (pcb->path), pcb->path_length);
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.PC, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.AX, sizeof(uint32_t)); //VER TAMANIO
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.BX, sizeof(uint32_t)); //VER TAMANIO
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.CX, sizeof(uint32_t)); //VER TAMANIO
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.DX, sizeof(uint32_t)); //VER TAMANIO
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.EAX, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.EBX, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.ECX, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.EDX, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.SI, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->registros_cpu.DI, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->estado, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->tiempo_ejecucion, sizeof(uint32_t));
    agregar_a_paquete(paquete_archivo_nuevo, &pcb->quantum, sizeof(uint32_t));

    enviar_paquete(paquete_archivo_nuevo, conexion); 

    free(paquete_archivo_nuevo);

}

void replanificar_y_ejecutar(t_pcb* proceso_ejecutando){
   t_pcb* siguiente_proceso = malloc(sizeof(t_pcb));
     
   if (planificador->algoritmo = FIFO) { // en este caso obtiene el siguiente proceso en la lista y lo manda a ejecutar.
		siguiente_proceso = obtener_proximo_proceso(planificador);
        enviar_proceso_a_cpu(siguiente_proceso,conexion_cpu_dispatch);
        free(siguiente_proceso); 
	}else {
		 if (temporal_gettime(cronometro) > 0) { // verifico si hay un cronometro andando
            
            desalojar_proceso_vrr(proceso_ejecutando); // finalizo la ejecucion del proceso actual y lo mando a alguna cola ready segun corresponda
            siguiente_proceso = obtener_proximo_proceso(planificador);
            ejecutar_modo_round_robin(siguiente_proceso);
        }else {
            siguiente_proceso = obtener_proximo_proceso(planificador);
            ejecutar_modo_round_robin(siguiente_proceso);
         }
    }

}
 
void  ejecutar_modo_round_robin( t_pcb* proceso){
    int quatum_restante;
    pthread_t hilo_cronometro;
    
    if (planificador->algoritmo == ROUND_ROBIN) {
		 quatum_restante = cfg_kernel->QUANTUM;
	}else {
		quatum_restante = proceso->quantum; // si ejecuta sin interrupciones entonces  proceso->quantum = cfg_kernel->QUANTUM
	}
	enviar_proceso_a_cpu(proceso,conexion_cpu_dispatch);
	cronometro = temporal_create();	
    //creamos hilo para no tener espera activa del cronometro      
    if (pthread_create(&hilo_cronometro, NULL, lanzar_interrupcion_fin_quantum, (int*)quatum_restante ) != 0) {
        perror("pthread_create_hilo_cronometro");
        free(hilo_cronometro);       
        
    }
     pthread_detach(hilo_cronometro);
	
	
}

void lanzar_interrupcion_fin_quantum (int quantum){
    uint32_t motivo  = FIN_QUANTUM;
    sleep(quantum);
    send(conexion_cpu_interrupt, &motivo, sizeof(uint32_t), NULL);
    log_info(logger_kernel, "Enviando interrupcion FIN de QUANTUM\n");
}

void desalojar_proceso_vrr(t_pcb* proceso){ // recibo contexto actualizado desde cpu
    
    temporal_stop(cronometro);
    
    list_remove(planificador->cola_exec, 0); // debería ser el único ejecutando}
    
    proceso->tiempo_ejecucion = get_time_temporal(cronometro);
	
	proceso->quantum = cfg_kernel->QUANTUM - proceso->tiempo_ejecucion; // actualizo el nuevo quantum restante

	if  (proceso->tiempo_ejecucion !=cfg_kernel->QUANTUM) {
		 list_add(planificador->cola_ready_prioridad, proceso); // como todavia le queda por ejecutar se asigna a la cola de prioridad
	}else {
		list_add(planificador->cola_ready, proceso);
	}	 

}



/* void replanificar_y_ejecutar() {
    t_pcb* proceso_actual = NULL;
    t_pcb* proximo_proceso = NULL;
    int motivo_desalojo;

    while (1) {
        // Obtener el próximo proceso a ejecutar
        proximo_proceso = obtener_proximo_proceso(planificador);

        if (proximo_proceso == NULL) {
            // No hay procesos para ejecutar, esperar un tiempo y volver a intentar
            usleep(100000); // equivale a 100ms
            continue;
        }

        // cambio de estado al estado EXEC
        cambiar_estado(proximo_proceso, ESTADO_RUNNING);

        // Enviar el Contexto de Ejecución al CPU
        enviar_pcb_a_cpu_por_dispatch(proximo_proceso);

        // Esperar la respuesta del CPU
        t_paquete* paquete_respuesta = recibir_paquete(conexion_cpu_dispatch);
        t_pcb* pcb_actualizado = deserializar_pcb(paquete_respuesta->buffer);
        motivo_desalojo = paquete_respuesta->codigo_operacion;

        // Actualizar el PCB con la información recibida
        actualizar_pcb(proximo_proceso, pcb_actualizado);

        // motivos de desalojo
        switch (motivo_desalojo) {
            case DESALOJO_QUANTUM:
                if (planificador->algoritmo == ROUND_ROBIN || planificador->algoritmo == VIRTUAL_ROUND_ROBIN) {
                    // Enviar interrupción para forzar el desalojo
                    enviar_interrupcion_a_cpu(proximo_proceso, conexion_cpu_interrupt);
                    cambiar_estado(proximo_proceso, ESTADO_READY);
                    list_add(planificador->cola_ready, proximo_proceso);
                }
                break;
            case DESALOJO_IO:
                cambiar_estado(proximo_proceso, ESTADO_BLOCKED);
                // Agregar a la cola de bloqueados correspondiente
                // Esto dependerá de cómo manejes los recursos de I/O
                break;
            case DESALOJO_EXIT:
                cambiar_estado(proximo_proceso, ESTADO_EXIT);
                finalizar_proceso(planificador, proximo_proceso);
                break;
            // ver si hay mas casos de desalojo
        }

        // Liberar memoria
        eliminar_paquete(paquete_respuesta);
        free(pcb_actualizado);

        // Si el motivo de desalojo implica replanificar, continuamos con el ciclo
        if (motivo_desalojo == DESALOJO_QUANTUM || motivo_desalojo == DESALOJO_EXIT) {
            continue;
        }

        // Si se llega aca, el proceso actual está bloqueado o ha finalizado
        // Esperamos un corto tiempo antes de intentar planificar el siguiente proceso
        usleep(10000); // 10ms
    }
}
*/

