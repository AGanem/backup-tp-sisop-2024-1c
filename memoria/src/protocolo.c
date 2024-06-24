#include <../include/protocolo.h>

//Funcion que atiende la peticion de Kernel segun el cod
void memoria_atender_cliente(void* socket_cliente){
    
    
	op_code response;
    t_list* valores =  malloc(sizeof(t_list));

	t_list* valores =  malloc(sizeof(t_list));
    t_io_input* input = malloc(sizeof(t_io_input));
    //t_paquete* paquete;
	op_code response;
	
     //t_proceso_memoria contexto_ejecucion;
    
    //t_paquete* paquete;
	op_code response;
    t_list* valores =  malloc(sizeof(t_list));
    t_proceso_memoria* solicitud_instruccion = malloc(sizeof(t_proceso_memoria));
    t_busqueda_marco* solicitud_marco = malloc(sizeof(t_busqueda_marco));
    t_io_input* peticion_leer = malloc(sizeof(t_io_direcciones_fisicas));
    //t_io_input* peticion_guardar = malloc(sizeof(t_io_input));
    //t_resize* solicitud_resize = malloc(sizeof(t_resize));
    //t_copy* copiar_valor = malloc(sizeof(t_copy));



	while (1) {
        //Se queda esperando a que KErnel le envie algo y extrae el cod de operacion
		int cod_op = recibir_operacion(socket_cliente);
		switch (cod_op) {
		
		case HANDSHAKE:
			log_info(logger_memoria, "Handshake realizado con cliente");
            response = HANDSHAKE_OK;
            if (send(socket_cliente, &response, sizeof(uint32_t), MSG_WAITALL) != sizeof(uint32_t)) {
                log_error(logger_memoria, "Error al enviar respuesta de handshake a cliente");
                   
                break;
            }
            break;
/*---------------------------- KERNEL-------------------------*/
        case CREAR_PROCESO_KERNEL:
            valores = recibir_paquete(socket_cliente);
            t_m_crear_proceso *iniciar_proceso = deserializar_crear_proceso(valores);
            leer_instrucciones(iniciar_proceso->archivo_pseudocodigo);                  
            crear_proceso(iniciar_proceso->pid);

            enviar_respuesta_crear_proceso(iniciar_proceso, socket_cliente);
            break;

		// case FINALIZAR_PROCESO:
		// 	valores = recibir_paquete(socket_cliente);
        //     t_pcb* finalizar_proceso = deserializar_finalizar_proceso(valores);

		// 	finalizar_preceso(finalizar_proceso->pid);
        //     enviar_respuesta_finalizar_proceso(finalizar_proceso, socket_cliente);
        //     break;
/*---------------------------- CPU-------------------------*/
        case PROXIMA_INSTRUCCION:
            valores = recibir_paquete(socket_cliente);
            solicitud_instruccion = deserializar_proxima_instruccion(valores);         
            char* instruccion = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter);
            log_trace(logger_memoria, "Se Encontro la Instruccion: %s", instruccion);
		//     usleep(cfg_memoria->RETARDO_RESPUESTA);
            enviar_respuesta_instruccion(instruccion, socket_cliente);     
            break;

        case PEDIDO_MARCO_A_MEMORIA:
            valores = recibir_paquete(socket_cliente);
            solicitud_marco = deserializar_solicitud_marco(valores);
            int marco = buscar_marco_pagina(solicitud_marco->pid, solicitud_marco->nro_pagina);
            enviar_solicitud_marco(marco, socket_cliente);
            break;


        case SOLICITUD_TAMANIO_PAGINA:
            printf("Envio tamaño de pagina\n");
            enviar_solicitud_tamanio(cfg_memoria->TAM_PAGINA, socket_cliente);
            break;

        case PETICION_VALOR_MEMORIA:
            valores = recibir_paquete(socket_cliente);
            peticion_leer = deserializar_peticion_valor(valores);     
            void* valor = leer_memoria(peticion_leer->direcciones_fisicas, peticion_leer->input_length);          //ver
            log_info(logger_memoria, "PID: %d - Acción: LEER - Direccion fisica: %d", peticion_leer->pid, peticion_leer->direcciones_fisicas);    //ver
            enviar_peticion_valor(valor, socket_cliente);                            
            break;

        // case GUARDAR_EN_DIRECCION_FISICA:
        //     valores = recibir_paquete(socket_cliente);
        //     t_io_input* peticion_guardar = deserializar_input(valores);   //ver
        //     escribir_memoria(peticion_guardar->pid, peticion_guardar->direcciones_fisicas, peticion_guardar->input, peticion_guardar->input_length);
        //     log_info(logger_memoria, "PID: %d - Acción: ESCRIBIR - Direccion fisica: %d", peticion_guardar->pid, peticion_guardar->direccion_fisica);
        //     free(peticion_guardar);
        //     break;

        // case SOLICITUD_RESIZE:
        //     valores = recibir_paquete(socket_cliente);
        //     solicitud_resize = deserializar_solicitud_resize(valores);
        //     administrar_resize(solicitud_resize->pid, solicitud_resize->tamanio);
        //     enviar_respuesta_resize(solicitud_resize, socket_cliente);
        //     break;

        // case ENVIO_COPY_STRING_A_MEMORIA:
        //     valores = recibir_paquete(socket_cliente);
        //     copiar_valor = deserializar_copiar_valor(valores);
        //     copiar_solicitud(copiar_valor->pid, capiar_valor->direccion_fisica, copiar_valor->valor);
        //     break;
		
/*---------------------------- ENTRADASALIDA-------------------------*/  
        case IO_M_STDIN: // lee de teclado y escribe en memoria
            printf("Recibida IO_M_STDIN \n");
            // Llenamos la lista con los datos recibidos de recibir_paquete 
            valores = recibir_paquete(socket_cliente);

            // Deserializamos los valores de la lista 
            input = deserializar_input(valores);

            if (valores == NULL || list_size(valores) == 0) {
                printf("El paquete vino vacío\n");
                break;
            }        

            escribir_memoria(input->pid, input->direcciones_fisicas, input->input, input->input_length);    //ver
            uint32_t response_interfaz = IO_M_STDIN_FIN;
            if (send(socket_cliente, &response_interfaz, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
                perror("send INTERFAZ_RECIBIDA response");
                break;
            }

            printf("Memoria envio IO_M_STDIN_FIN a IO \n");
            list_clean(valores);
            break;           
            
		case IO_M_STDOUT: // Lee de memoria e imprime por pantalla
             printf("Recibida IO_M_STDOUT \n");

            t_io_output* io_output = malloc(sizeof(t_io_input));
            t_io_direcciones_fisicas* io_stdout = malloc(sizeof(t_io_direcciones_fisicas));
                
            valores = recibir_paquete(socket_cliente);
            io_stdout = deserializar_io_df(valores);
        
            if (valores == NULL || list_size(valores) == 0) {
                printf("El paquete vino vacío\n");
                break;
            }

            void* output = leer_memoria(io_stdout->direcciones_fisicas, io_output->output_length);      //ver
            uint32_t tamanio_output = string_length(output)+1;
            io_output->pid = io_stdout->pid;
            io_output->output_length = tamanio_output;
            io_output->output = output;

            printf("Tamanio output %d\n",io_output->output_length);
            enviar_output(io_output ,socket_cliente, IO_M_STDOUT_FIN );
            list_clean(valores);    
            break;
        case IO_FS_WRITE://Lee de memoria y escribe en un archivo
            printf("Recibida IO_FS_WRITE \n");

            t_io_output* io_escritura = malloc(sizeof(t_io_input));
            t_io_direcciones_fisicas* io_fs_write = malloc(sizeof(t_io_direcciones_fisicas));
                
                                
            valores = recibir_paquete(socket_cliente);
            io_stdout = deserializar_io_df(valores);
        
            if (valores == NULL || list_size(valores) == 0) {
                printf("El paquete vino vacío\n");
                break;
            }
            void* output = leer_memoria(io_stdout->direcciones_fisicas, io_output->output_length);      //ver
            char* escritura ="HOLA!"; // Pesa 6 bytes  reemplazar esto por función de memoria que busca el valor
            
            uint32_t tamanio_escritura = string_length(escritura)+1;
            io_escritura->pid = io_fs_write->pid;
            io_escritura->output_length = tamanio_escritura;
            io_escritura->output = escritura;
            printf("Tamanio output %d\n",io_escritura->output_length);
            enviar_output(io_escritura ,socket_cliente, IO_FS_WRITE_M);
            printf("Sevidor Memoria envía IO_M_STDOUT_FIN al cliente\n");
            list_clean(valores);
            break;

        case IO_FS_READ: //Lee de un archivo y escribe en memoria
            printf("Recibida IO_FS_READ \n");
            
            valores = recibir_paquete(socket_cliente);         
            input = deserializar_input(valores);        
             
            escribir_memoria(input->pid, input->direcciones_fisicas, input->input, input->input_length);      
            response_interfaz = IO_FS_READ_M; // termina de escribir
            if (send(socket_cliente, &response_interfaz, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
                perror("send INTERFAZ_RECIBIDA response");
                break;
            }

            printf("Sevidor Memoria envía IO_FS_READ_M al cliente\n");
            list_clean(valores);
            break;

        case -1:
			log_error(logger_memoria, " Cliente se desconectó.");
			//return EXIT_FAILURE;
			break;       
        
        
        default:
			log_warning(logger_memoria,"Operacion desconocida");
			break;  
        
        
        }
    }
}


