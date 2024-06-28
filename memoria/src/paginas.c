#include "../include/paginas.h"




//Funcion que crea la tabla de paginas a partir de un pid
void crear_proceso(uint32_t proceso_pid){

    printf("Creacion del proceso PID %i", proceso_pid);
    printf("Iniciando estructuras");

    //Guardo en una varia de tipo struct la tabla creada
    t_tabla_de_paginas *tabla_de_paginas = crear_tabla_pagina(proceso_pid);
    //log_info(logger_memoria, "PID: %d - Tamaño: %d", proceso_pid, sizeof(t_tabla_de_paginas));

    list_add(lista_tablas_de_paginas, tabla_de_paginas);
}




//Funcion que a partir de un pid devuelve una tabla de paginas
t_tabla_de_paginas *crear_tabla_pagina(int pid){


    //Inicializo una variable tabla de pagina de tipo struct y le asigno un tamaño
    t_tabla_de_paginas *tabla_de_paginas = malloc(sizeof(t_tabla_de_paginas));

    //Le asigno valores a mi variable creada
    tabla_de_paginas->id = pid;
    tabla_de_paginas->lista_de_paginas = list_create();


    return tabla_de_paginas;
}



//Funcion que a partir del tamaño del proceso devuelve la cantidad de frames
uint32_t calcular_marcos(uint32_t tamanio_proceso){

    //Calculamos la cantidad de marcos
    uint32_t cantidad_de_marcos = tamanio_proceso / cfg_memoria->TAM_PAGINA;

    //Si el resto es distinto a cero la cantidad de marcos la incrementamos en 1
    if (tamanio_proceso % cfg_memoria->TAM_PAGINA != 0)
        cantidad_de_marcos++;
    return cantidad_de_marcos;
}





//Funcion que dado un id de proceso buscamos la tabla de pagina y la retorna
t_tabla_de_paginas *busco_tabla_de_paginas_por_PID(uint32_t proceso_pid){

    log_trace(logger_memoria, "Buscando la Tabla de Paginas por PID");

    t_tabla_de_paginas *tabla_de_paginas;

    //Recorremos la lista que contiene tabla de paginas
    for (int i = 0; i < list_size(lista_tablas_de_paginas); i++){

        //Sacamos una tabla de paginas de la lista
        tabla_de_paginas = list_get(lista_tablas_de_paginas, i);
        //Si el id de la tabla es el mismo que la tabla, la retorna
        if (proceso_pid == tabla_de_paginas->id)
            return tabla_de_paginas;
    }

    log_error(logger_memoria, "PID - %d No se encontro la Tabla de Paginas", proceso_pid);
    abort();
}


//Funcion que dado un marco, busco en la lista de paginas y lo retorna
t_pagina *busco_pagina_por_marco(t_list *lista_de_paginas, uint32_t marco){

    log_trace(logger_memoria, "Buscando Pagina por Marco: %i", marco);

    t_pagina *pagina;

    //Recorremos la lista de paginas perteneciente a la tabla
    for (int i = 0; i < list_size(lista_de_paginas); i++){

        //Sacamos una pagina de la lista
        pagina = list_get(lista_de_paginas, i);
        //Si el marco de la pagina es le mismo que buscaba lo retorna
        if (marco == pagina->marco) {
            log_trace(logger_memoria, "Encontre pagina: %i", i);
            return pagina;
        }
    }
    log_trace(logger_memoria, "No se encontro pagina");
    return NULL;
}



void* escribir_memoria(uint32_t proceso_pid, uint32_t direccion_fisica, char* valor, uint32_t tamanio){

    void* escrito = malloc(tamanio);
    uint32_t espacio_escrito = 0;
    //t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

    while (tamanio > 0){
    
        uint32_t marco = direccion_fisica / cfg_memoria->TAM_PAGINA;
        uint32_t offset = direccion_fisica % cfg_memoria->TAM_PAGINA;
        uint32_t espacio_restante_marco = cfg_memoria->TAM_PAGINA - offset;
        uint32_t espacio_a_escribir;

        if (tamanio < espacio_restante_marco){
            espacio_a_escribir = tamanio;
        }
        else{
            espacio_a_escribir = espacio_restante_marco;
        }

        memcpy(memoria + direccion_fisica, valor, tamanio);

        //t_pagina *pagina = busco_pagina_por_marco(tabla_de_paginas->lista_de_paginas, marco);
        //Actualizamos que fue modificado
        //pagina->modificado = true;

        espacio_escrito = espacio_escrito + espacio_a_escribir;
        tamanio = tamanio - espacio_a_escribir;

        //Si todavia hay espacio que tiene que ser escrito
        if (tamanio > 0){

            t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

            t_pagina *pagina = busco_pagina_por_marco(tabla_de_paginas->lista_de_paginas, marco + 1);
            
            //Actualizamos la df para que empiece desde el sig frame
            direccion_fisica = pagina->marco * cfg_memoria->TAM_PAGINA;
        }
    }
    escrito = "OK";
    return escrito;
}






void* leer_memoria(uint32_t proceso_pid, uint32_t direccion_fisica, uint32_t tamanio){

    //Reservo espacio para la variable que voy a devolver
    void* leido = malloc(tamanio);

    uint32_t espacio_leido = 0;

    t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

    //Mientras haya algo para leer
    while (tamanio > 0){
        
        uint32_t marco = direccion_fisica / cfg_memoria->TAM_PAGINA;
        uint32_t offset = direccion_fisica % cfg_memoria->TAM_PAGINA;
        uint32_t espacio_restante_marco = cfg_memoria->TAM_PAGINA - offset;
        uint32_t espacio_a_leer;

        if (tamanio < espacio_restante_marco){
            espacio_a_leer = tamanio;
        }
        else{
            espacio_a_leer = espacio_restante_marco;
        }

        memcpy(leido, memoria + direccion_fisica, tamanio);

        espacio_leido = espacio_leido + espacio_a_leer;
        tamanio = tamanio - espacio_a_leer;
        direccion_fisica = direccion_fisica + espacio_a_leer;

        //SI todavia hay tamanio por leer
        if (tamanio > 0){

            //Busco la pagina sig
            t_pagina *pagina = busco_pagina_por_marco(tabla_de_paginas->lista_de_paginas, marco + 1);

            //Actualizamos la df para que empiece desde el sig frame
            direccion_fisica = pagina->marco * cfg_memoria->TAM_PAGINA;
        }
        
    }
    
    return leido;
}



//Posible implementacion de copy si solo escribo
// void copiar_solicitud(uint32_t proceso_pid, uint32_t direccion_fisica, char* valor){

//     void* valor_a_copiar = leer_memoria(proceso_pid, direccion_fisica, strlen(valor));
//     escribir_memoria(proceso_pid, direccion_fisica, valor, strlen(valor));
// }



uint32_t buscar_marco_pagina(uint32_t proceso_pid, uint32_t numero_de_pagina){

    t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

    t_pagina *pagina = list_get(tabla_de_paginas->lista_de_paginas, numero_de_pagina);

    log_debug(logger_memoria, "ACCESO TABLA PAGINAS");
    log_info(logger_memoria, "PID: %d - Página: %d - Marco: %d", proceso_pid, numero_de_pagina, pagina->marco);

    if (pagina->presencia){
        return pagina->marco;
    }

    return -1;
}



//Funciones para resize

uint32_t obtener_marco_libre(){
    //Recorro la memoria para marcar los frames libres
    for (uint32_t i = 0; i < cantidad_frames_memoria; i++){
        //Si la posicion del array esta disponible
        if (!bitarray_test_bit(bitmap_frames, i)){
            return i;
        }
    }
    log_trace(logger_memoria, "No se obtuvo un marco libre");
    return -1;
}




op_code administrar_resize(uint32_t proceso_pid, uint32_t tamanio_proceso){

    t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

    int ultima_pagina = list_size(tabla_de_paginas->lista_de_paginas);
    uint32_t marcos_a_reservar = calcular_marcos(tamanio_proceso);

    log_info(logger_memoria, "PID: %d - Tamaño: %d", proceso_pid, marcos_a_reservar);

    if(tamanio_proceso == 0){
        return SOLICITUD_RESIZE_RTA;
    }else{    
        if(tamanio_proceso < espacio_disponible()){
            //Recorro mientras sea menor a la cantidad de frames
            for (int i = 0; i < marcos_a_reservar; i++){
                t_pagina *pagina = malloc(sizeof(t_pagina));

                pagina->marco = obtener_marco_libre();                 
                pagina->posicion = ultima_pagina+i;             
                pagina->presencia = false;          
                pagina->modificado = false;        
            
                list_add(tabla_de_paginas->lista_de_paginas, pagina);
            }
            return SOLICITUD_RESIZE_RTA;
        }else{
            return OUT_OF_MEMORY;
        }
    }  
}


int espacio_disponible(){
    int frames_libres = 0;   

    for (uint32_t i = 0; i < bitarray_get_max_bit(bitmap_frames); i++){
    //Si la posicion del array esta disponible
        if (!bitarray_test_bit(bitmap_frames, i)){
            frames_libres++ ;
        }
    }
    return frames_libres*cfg_memoria->TAM_PAGINA;
}


/*
//Puede que falte detallar mas de cerrar del todo el proceso
//Funcion que en base al id de un proceso finalizamos sus estructuras
void finalizar_proceso(uint32_t proceso_pid){

    log_trace(logger_memoria, "Liberacion del proceso PID %i", proceso_pid);
    t_tabla_de_paginas *tabla_de_paginas = busco_tabla_de_paginas_por_PID(proceso_pid);

    //Recorremos la lista de paginas
    for (int j = 0; j < list_size(tabla_de_paginas->lista_de_paginas); j++){

        //Sacamos la pagina de la lista
        t_pagina *pagina = list_get(tabla_de_paginas->lista_de_paginas, j);

        //Si la pagina se encuentra presente la limpiamos
        if (pagina->presencia){

            pagina->presencia = false;
            bitarray_clean_bit(bitmap_frames, pagina->marco);
        }
    }
    
    log_info(logger_memoria, "PID: %d - Tamaño: %d", proceso_pid, list_size(tabla_de_paginas->lista_de_paginas));
}
*/