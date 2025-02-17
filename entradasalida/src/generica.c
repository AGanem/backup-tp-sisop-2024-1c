#include "../include/generica.h"


void iniciar_interfaz_generica (int socket_kernel) {
    
    op_code response;
    op_code cop;
 
   

    log_info(logger_entrada_salida, "Interfaz %s de tipo GENERICA iniciada",cfg_entrada_salida->NOMBRE_INTERFAZ);  
    
    while (socket_kernel != -1) {

        if (recv(socket_kernel, &cop, sizeof(int32_t), MSG_WAITALL) != sizeof(int32_t)) {
            log_info(logger_entrada_salida, "DISCONNECT!");

            break;
        }
    switch (cop) {
            
            case HANDSHAKE :

                log_info(logger_entrada_salida, "Handshake realizado con Kernel");
                response = HANDSHAKE_OK;
                if (send(socket_kernel, &response, sizeof(uint32_t), MSG_WAITALL) != sizeof(uint32_t)) {
                    log_error(logger_entrada_salida, "Error al enviar respuesta de handshake a kernel");
                    
                    break;
                }
                break;

            case HANDSHAKE_OK :

                printf("Handshake recibido handshake exitosamente con Kernel");
                
                break;                
            
            case IO_K_GEN_SLEEP :
                
                printf("IO_K_GEN_SLEEP recibida desde Kernel\n");
                t_list* lista_paquete = recibir_paquete(socket_kernel);
                t_io_espera* io_espera = deserializar_espera (lista_paquete);
                esperar(io_espera->pid, io_espera->tiempo_espera);

                response = IO_K_GEN_SLEEP_FIN;
                printf("valor : %d\n",socket_kernel );
                 printf("valor : %d\n", io_espera->pid );
                printf("valor : %d\n", io_espera->pid );
                printf("valor : %d\n",response);
                printf("valor : %s\n",cfg_entrada_salida->NOMBRE_INTERFAZ);
                enviar_respuesta_io (socket_kernel,response, io_espera->pid, cfg_entrada_salida->NOMBRE_INTERFAZ);
                printf("IO_K_GEN_SLEEP RESPUESTA ENVIADAl\n");
                //list_destroy_and_destroy_elements(lista_paquete, free);
                //free(io_espera);
                break;

            default:
                response = OPERACION_INVALIDA;
                if (send(socket_kernel, &response, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
                    log_error(logger_entrada_salida, "Operacion invalida enviada desde kernel");
                    break;
                }
                break;
        }
}

}

 t_io_espera* deserializar_espera (t_list*  lista_paquete ){

       t_io_espera* io_espera = malloc(sizeof(t_io_espera));
       io_espera->pid = *(uint32_t*)list_get(lista_paquete, 0);
       io_espera->tiempo_espera = *(uint32_t*)list_get(lista_paquete, 1);

    return io_espera;    
}

void esperar(int32_t pid, int32_t unidades ){

    log_info(logger_entrada_salida, "PID: %d - Operacion: IO_GEN_SLEEP ",pid);
    sleep (unidades * cfg_entrada_salida->TIEMPO_UNIDAD_TRABAJO / 1000);
    printf("Se completaron  %d unidades de trabajo\n",unidades);
}

