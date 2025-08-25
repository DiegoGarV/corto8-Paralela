import time
import random
import numpy as np

# Parámetros
NUM_PUESTOS = 610000
PRECIO_MIN = 12.0
PRECIO_MAX = 18.0
SIMULACION_TIEMPO = 1000

# Inicialización de arrays tipo struct
puestos = np.zeros(NUM_PUESTOS, dtype=[('id', np.int32),
                                       ('precio_actual', np.float64),
                                       ('ventas_totales', np.int32),
                                       ('ventas_recientes', np.int32),
                                       ('ingresos_totales', np.float64)])
tiempo_simulacion = 0
simulacion_activa = True

T_total_ini = 0.0
T_total_fin = 0.0
T_mostrar = 0.0
T_precios = 0.0
T_ventas = 0.0

def inicializar():
    rng = np.random.default_rng()
    puestos['id'] = np.arange(1, NUM_PUESTOS+1)
    puestos['precio_actual'] = PRECIO_MIN + rng.random(NUM_PUESTOS) * (PRECIO_MAX - PRECIO_MIN)
    puestos['ventas_totales'].fill(0)
    puestos['ventas_recientes'].fill(0)
    puestos['ingresos_totales'].fill(0.0)

def mostrar_estado():
    global T_mostrar
    t0 = time.perf_counter()
    total_ingresos = puestos['ingresos_totales'].sum()
    total_ventas = puestos['ventas_totales'].sum()
    T_mostrar += time.perf_counter() - t0

def calcular_precios():
    global T_precios
    t0 = time.perf_counter()
    rng = np.random.default_rng()
    
    vr = puestos['ventas_recientes']
    mask_alta = vr >= 8
    mask_media = (vr >= 4) & (vr < 8)
    mask_baja = vr < 4

    f = np.empty(NUM_PUESTOS)
    f[mask_alta] = 1.05 + rng.random(mask_alta.sum()) * 0.10
    f[mask_media] = 0.95 + rng.random(mask_media.sum()) * 0.10
    f[mask_baja] = 0.90 + rng.random(mask_baja.sum()) * 0.05

    puestos['precio_actual'] *= f
    np.clip(puestos['precio_actual'], PRECIO_MIN, PRECIO_MAX*1.5, out=puestos['precio_actual'])
    puestos['ventas_recientes'].fill(0)

    T_precios += time.perf_counter() - t0

def procesar_ventas():
    global T_ventas
    t0 = time.perf_counter()
    rng = np.random.default_rng()
    for i in range(NUM_PUESTOS):
        prob_base = 0.7
        factor_precio = (PRECIO_MAX - puestos['precio_actual'][i]) / PRECIO_MAX
        probabilidad = prob_base * (0.5 + factor_precio)

        clientes_potenciales = rng.integers(1,6)
        for _ in range(clientes_potenciales):
            if rng.random() < probabilidad:
                cantidad = rng.integers(1,4)
                puestos['ventas_totales'][i] += cantidad
                puestos['ventas_recientes'][i] += cantidad
                puestos['ingresos_totales'][i] += cantidad * puestos['precio_actual'][i]

    T_ventas += time.perf_counter() - t0

# Main
print("Inicializando sistema de ventas de shucos...")
inicializar()

print(f"Iniciando simulacion por {SIMULACION_TIEMPO} segundos...")
T_total_ini = time.perf_counter()

while simulacion_activa and tiempo_simulacion < SIMULACION_TIEMPO:
    procesar_ventas()
    if tiempo_simulacion % 2 == 0 and tiempo_simulacion > 0:
        calcular_precios()
    mostrar_estado()
    tiempo_simulacion += 1
    if tiempo_simulacion >= SIMULACION_TIEMPO:
        simulacion_activa = False

T_total_fin = time.perf_counter()

# Resumen final
total_ingresos = puestos['ingresos_totales'].sum()
total_ventas = puestos['ventas_totales'].sum()
precio_promedio = puestos['precio_actual'].mean()
puesto_mas_vendido = np.argmax(puestos['ventas_totales'])

print("=================================")
print(f"Puesto mas exitoso: Puesto {puestos['id'][puesto_mas_vendido]} ({puestos['ventas_totales'][puesto_mas_vendido]} ventas)")
print(f"Ingresos totales: Q{total_ingresos:.2f}")
print(f"Shucos vendidos: {total_ventas}")
print(f"Precio promedio final: Q{precio_promedio:.2f}")
print(f"Ingreso por shuco: Q{(total_ingresos/total_ventas) if total_ventas>0 else 0:.2f}")

T_total = T_total_fin - T_total_ini
print(f"Tiempo total simulacion : {T_total:.6f} s")
print(f" - mostrar_estado       : {T_mostrar:.6f} s")
print(f" - calcular_precios     : {T_precios:.6f} s")
print(f" - procesar_ventas      : {T_ventas:.6f} s")
