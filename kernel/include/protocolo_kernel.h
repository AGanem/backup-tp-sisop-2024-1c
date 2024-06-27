#ifndef PROTOCOLO_KERNEL_H
#define PROTOCOLO_KERNEL_H
#include <utils/utils.h>
#include <pthread.h>
#include <commons/string.h>
#include "../include/init_kernel.h"
#include "../include/consola.h"
//#include <utils/utils.h>
//#include <pthread.h>
//#include <commons/string.h>
//#include <init_kernel.h>


//void Escuchar_Msj_De_Conexiones();
//void Kernel_atender_cpu_dispatch();
//void Kernel_atender_cpu_interrupt();
//void Kernel_atender_memoria();
void Escuchar_Msj_De_Conexiones();
void Kernel_escuchar_cpu_dispatch();
void Kernel_escuchar_cpu_interrupt();
void Kernel_escuchar_memoria();
void enviar_creacion_de_proceso_a_memoria(t_pcb* pcb, int socket_memoria);
//void enviar_creacion_de_proceso_a_memoria(pcb, socket_memoria);  // Enviar a memoria



#endif
