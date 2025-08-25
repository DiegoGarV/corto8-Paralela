#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PUESTOS 610000
#define PRECIO_MIN 12.0
#define PRECIO_MAX 18.0
#define SIMULACION_TIEMPO 1000 

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

static double T_total_ini, T_total_fin;
static double T_mostrar = 0.0, T_precios = 0.0, T_ventas = 0.0;

void inicializar() {
    srand(time(NULL));
    for (int i = 0; i < NUM_PUESTOS; i++) {
        puestos[i].id = i + 1;
        puestos[i].precio_actual = PRECIO_MIN + ((double)rand() / RAND_MAX) * (PRECIO_MAX - PRECIO_MIN);
        puestos[i].ventas_totales = 0;
        puestos[i].ventas_recientes = 0;
        puestos[i].ingresos_totales = 0.0;
    }
}

void mostrar_estado() {
    double t0 = (double)clock() / CLOCKS_PER_SEC;

    double total_ingresos = 0.0;
    int total_ventas = 0;

    for (int i = 0; i < NUM_PUESTOS; i++) {
        total_ingresos += puestos[i].ingresos_totales;
        total_ventas += puestos[i].ventas_totales;
    }

    T_mostrar += ((double)clock() / CLOCKS_PER_SEC) - t0;
}

void calcular_precios() {
    double t0 = (double)clock() / CLOCKS_PER_SEC;

    for (int i = 0; i < NUM_PUESTOS; i++) {
        double factor_ajuste = 1.0;

        if (puestos[i].ventas_recientes >= 8) {
            factor_ajuste = 1.05 + ((double)rand() / RAND_MAX) * 0.10;
        } else if (puestos[i].ventas_recientes >= 4) {
            factor_ajuste = 0.95 + ((double)rand() / RAND_MAX) * 0.10;
        } else {
            factor_ajuste = 0.90 + ((double)rand() / RAND_MAX) * 0.05;
        }

        puestos[i].precio_actual *= factor_ajuste;

        if (puestos[i].precio_actual < PRECIO_MIN) puestos[i].precio_actual = PRECIO_MIN;
        else if (puestos[i].precio_actual > PRECIO_MAX * 1.5) puestos[i].precio_actual = PRECIO_MAX * 1.5;

        puestos[i].ventas_recientes = 0;
    }

    T_precios += ((double)clock() / CLOCKS_PER_SEC) - t0;
}

void procesar_ventas() {
    double t0 = (double)clock() / CLOCKS_PER_SEC;

    for (int i = 0; i < NUM_PUESTOS; i++) {
        double prob_base = 0.7;
        double factor_precio = (PRECIO_MAX - puestos[i].precio_actual) / PRECIO_MAX;
        double probabilidad = prob_base * (0.5 + factor_precio);

        int clientes_potenciales = 1 + rand() % 5;

        for (int j = 0; j < clientes_potenciales; j++) {
            if ((double)rand() / RAND_MAX < probabilidad) {
                int cantidad = 1 + rand() % 3;
                puestos[i].ventas_totales += cantidad;
                puestos[i].ventas_recientes += cantidad;
                puestos[i].ingresos_totales += cantidad * puestos[i].precio_actual;
            }
        }
    }

    T_ventas += ((double)clock() / CLOCKS_PER_SEC) - t0;
}

int main() {
    printf("Inicializando sistema de ventas de shucos...\n");
    inicializar();

    printf("Iniciando simulacion por %d segundos...\n", SIMULACION_TIEMPO);
    T_total_ini = (double)clock() / CLOCKS_PER_SEC;

    while (simulacion_activa && tiempo_simulacion < SIMULACION_TIEMPO) {
        procesar_ventas();

        if (tiempo_simulacion % 2 == 0 && tiempo_simulacion > 0) {
            calcular_precios();
        }

        mostrar_estado();

        tiempo_simulacion++;

        if (tiempo_simulacion >= SIMULACION_TIEMPO) simulacion_activa = 0;
    }

    T_total_fin = (double)clock() / CLOCKS_PER_SEC;

    // Resumen final
    double total_ingresos = 0.0;
    int total_ventas = 0;
    int puesto_mas_vendido = 0;
    double precio_promedio = 0.0;

    for (int i = 0; i < NUM_PUESTOS; i++) {
        total_ingresos += puestos[i].ingresos_totales;
        total_ventas += puestos[i].ventas_totales;
        precio_promedio += puestos[i].precio_actual;

        if (puestos[i].ventas_totales > puestos[puesto_mas_vendido].ventas_totales) {
            puesto_mas_vendido = i;
        }
    }

    precio_promedio /= NUM_PUESTOS;

    printf("Puesto mas exitoso: Puesto %d (%d ventas)\n",
           puestos[puesto_mas_vendido].id, puestos[puesto_mas_vendido].ventas_totales);
    printf("Ingresos totales: Q%.2f\n", total_ingresos);
    printf("Shucos vendidos: %d\n", total_ventas);
    printf("Precio promedio final: Q%.2f\n", precio_promedio);
    printf("Ingreso por shuco: Q%.2f\n", total_ventas > 0 ? total_ingresos / total_ventas : 0);

    double T_total = T_total_fin - T_total_ini;
    printf("Tiempo total simulacion : %.6f s\n", T_total);
    printf(" - mostrar_estado       : %.6f s\n", T_mostrar);
    printf(" - calcular_precios     : %.6f s\n", T_precios);
    printf(" - procesar_ventas      : %.6f s\n", T_ventas);

    return 0;
}
