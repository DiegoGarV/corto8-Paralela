#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_PUESTOS 5
#define PRECIO_MIN 12.0
#define PRECIO_MAX 18.0
#define SIMULACION_TIEMPO 30 

typedef struct {
    int id;
    double precio_actual;
    int ventas_totales;
    int ventas_recientes; 
    double ingresos_totales;
} Puesto;


Puesto puestos[NUM_PUESTOS];
int tiempo_simulacion = 0;
int simulacion_activa = 1;

void inicializar() {
    srand(time(NULL));
    
    
    for (int i = 0; i < NUM_PUESTOS; i++) {
        puestos[i].id = i + 1;
        puestos[i].precio_actual = PRECIO_MIN + ((double)rand() / RAND_MAX) * (PRECIO_MAX - PRECIO_MIN);
        puestos[i].ventas_totales = 0;
        puestos[i].ventas_recientes = 0;
        puestos[i].ingresos_totales = 0.0;
    }
    
    printf("SISTEMA DE VENTAS SHUCOS INICIADO\n");
    printf("================================================\n");
    printf("Puestos inicializados con precios aleatorios:\n");
    for (int i = 0; i < NUM_PUESTOS; i++) {
        printf("Puesto %d: Q%.2f\n", puestos[i].id, puestos[i].precio_actual);
    }
    printf("================================================\n\n");
}


void mostrar_estado() {
    
    system("clear"); 
    
    printf("\nVENTAS EN LOS SHUCOS - TIEMPO: %d segundos\n", tiempo_simulacion);
    printf("================================================================\n");
    printf("| Puesto | Precio  | Ventas Tot | Vent.Rec | Ingresos Tot |\n");
    printf("================================================================\n");
    
    double total_ingresos = 0.0;
    int total_ventas = 0;
    
    for (int i = 0; i < NUM_PUESTOS; i++) {
        printf("| %-6d | Q%-6.2f | %-10d | %-8d | Q%-11.2f |\n",
               puestos[i].id,
               puestos[i].precio_actual,
               puestos[i].ventas_totales,
               puestos[i].ventas_recientes,
               puestos[i].ingresos_totales);
        
        total_ingresos += puestos[i].ingresos_totales;
        total_ventas += puestos[i].ventas_totales;
    }
    
    printf("================================================================\n");
    printf("TOTALES: Ventas: %d | Ingresos: Q%.2f\n", total_ventas, total_ingresos);
    printf("================================================================\n");
    printf("Leyenda: Vent.Rec = Ventas Recientes (últimos 2 seg)\n");
    printf("Ejecutando secuencialmente... Tiempo restante: %d seg\n", SIMULACION_TIEMPO - tiempo_simulacion);
}


void calcular_precios() {
    
    for (int i = 0; i < NUM_PUESTOS; i++) {
        double precio_anterior = puestos[i].precio_actual;
        double factor_ajuste = 1.0;
        
        
        if (puestos[i].ventas_recientes >= 8) {
            
            factor_ajuste = 1.05 + ((double)rand() / RAND_MAX) * 0.10;
        } else if (puestos[i].ventas_recientes >= 4) {
            
            factor_ajuste = 0.95 + ((double)rand() / RAND_MAX) * 0.10;
        } else {
            
            factor_ajuste = 0.90 + ((double)rand() / RAND_MAX) * 0.05;
        }
        
        
        puestos[i].precio_actual *= factor_ajuste;
        
        
        if (puestos[i].precio_actual < PRECIO_MIN) {
            puestos[i].precio_actual = PRECIO_MIN;
        } else if (puestos[i].precio_actual > PRECIO_MAX * 1.5) {
            puestos[i].precio_actual = PRECIO_MAX * 1.5;
        }
        
        
        double cambio_porcentaje = ((puestos[i].precio_actual - precio_anterior) / precio_anterior) * 100;
        if (abs((int)cambio_porcentaje) >= 2) {
            printf("[PRECIO] Puesto %d: Q%.2f → Q%.2f (%.1f%%)\n", 
                   puestos[i].id, precio_anterior, puestos[i].precio_actual, cambio_porcentaje);
        }
        
        
        puestos[i].ventas_recientes = 0;
    }
}


void procesar_ventas() {
    
    for (int i = 0; i < NUM_PUESTOS; i++) {
        
        double prob_base = 0.7; 
        double factor_precio = (PRECIO_MAX - puestos[i].precio_actual) / PRECIO_MAX;
        double probabilidad = prob_base * (0.5 + factor_precio);
        
        
        int clientes_potenciales = 1 + rand() % 5;
        int ventas_este_segundo = 0;
        
        for (int j = 0; j < clientes_potenciales; j++) {
            if ((double)rand() / RAND_MAX < probabilidad) {
                
                int cantidad = 1 + rand() % 3;
                
                puestos[i].ventas_totales += cantidad;
                puestos[i].ventas_recientes += cantidad;
                puestos[i].ingresos_totales += cantidad * puestos[i].precio_actual;
                ventas_este_segundo += cantidad;
            }
        }
        
        
        if (ventas_este_segundo >= 3) {
            printf("[VENTA] Puesto %d vendió %d shucos (Total: %d)\n", 
                   puestos[i].id, ventas_este_segundo, puestos[i].ventas_totales);
        }
    }
}

int main() {
    inicializar();
    
    printf("Iniciando simulación por %d segundos...\n", SIMULACION_TIEMPO);
    sleep(2);
    
    
    while (simulacion_activa && tiempo_simulacion < SIMULACION_TIEMPO) {
        
        procesar_ventas();
        
        
        if (tiempo_simulacion % 2 == 0 && tiempo_simulacion > 0) {
            calcular_precios();
        }
        
        
        mostrar_estado();
        
        
        tiempo_simulacion++;
        sleep(1);
        
        
        if (tiempo_simulacion >= SIMULACION_TIEMPO) {
            simulacion_activa = 0;
        }
    }
    
    
    system("clear");
    printf("=================================\n");
    printf("RESUMEN FINAL DESPUÉS DE %d SEGUNDOS:\n", SIMULACION_TIEMPO);
    printf("=================================\n");
    
    double total_ingresos = 0.0;
    int total_ventas = 0;
    int puesto_mas_vendido = 0;
    double precio_promedio = 0.0;
    
    for (int i = 0; i < NUM_PUESTOS; i++) {
        printf("Puesto %d:\n", puestos[i].id);
        printf("  - Precio final: Q%.2f\n", puestos[i].precio_actual);
        printf("  - Ventas totales: %d shucos\n", puestos[i].ventas_totales);
        printf("  - Ingresos: Q%.2f\n", puestos[i].ingresos_totales);
        printf("  - Precio promedio por shuco: Q%.2f\n", 
               puestos[i].ventas_totales > 0 ? puestos[i].ingresos_totales / puestos[i].ventas_totales : 0);
        printf("\n");
        
        total_ingresos += puestos[i].ingresos_totales;
        total_ventas += puestos[i].ventas_totales;
        precio_promedio += puestos[i].precio_actual;
        
        if (puestos[i].ventas_totales > puestos[puesto_mas_vendido].ventas_totales) {
            puesto_mas_vendido = i;
        }
    }
    
    precio_promedio /= NUM_PUESTOS;
    
    printf("Puesto más exitoso: Puesto %d (%d ventas)\n", 
           puestos[puesto_mas_vendido].id, puestos[puesto_mas_vendido].ventas_totales);
    printf("Ingresos totales: Q%.2f\n", total_ingresos);
    printf("Shucos vendidos: %d\n", total_ventas);
    printf("Precio promedio final: Q%.2f\n", precio_promedio);
    printf("Ingreso por shuco: Q%.2f\n", total_ventas > 0 ? total_ingresos / total_ventas : 0);
    
    return 0;
}