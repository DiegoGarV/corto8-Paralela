import argparse, time, math, os, sys, random
from multiprocessing import Process, Lock, Value
from multiprocessing import shared_memory
import numpy as np

# Parámetros
NUM_PUESTOS_DEFAULT = 650000
PRECIO_MIN = 12.0
PRECIO_MAX = 18.0
SIMULACION_TIEMPO_DEFAULT = 1000

# Utilidades de arrays compartidos (numpy + shared_memory)
def make_shared_array(name_hint, shape, dtype):
    nbytes = np.prod(shape) * np.dtype(dtype).itemsize
    shm = shared_memory.SharedMemory(create=True, size=int(nbytes), name=None)
    arr = np.ndarray(shape, dtype=dtype, buffer=shm.buf)
    arr_name = shm.name
    return shm, arr, arr_name

def attach_shared_array(name, shape, dtype):
    shm = shared_memory.SharedMemory(name=name)
    arr = np.ndarray(shape, dtype=dtype, buffer=shm.buf)
    return shm, arr

# Procesos
def proc_mostrar(precio_name, tot_name, rec_name, ing_name, N,
                 lock_display, simulacion_activa, print_each=0):

    shm_p, precio = attach_shared_array(precio_name, (N,), np.float64)
    shm_t, ventas_totales = attach_shared_array(tot_name, (N,), np.int32)
    shm_r, ventas_recientes = attach_shared_array(rec_name, (N,), np.int32)
    shm_i, ingresos_totales = attach_shared_array(ing_name, (N,), np.float64)

    t0 = time.perf_counter()
    while simulacion_activa.value:
        with lock_display:
            total_ingresos = float(ingresos_totales.sum())
            total_ventas = int(ventas_totales.sum())
            # if print_each and (time.time() % print_each < 0.05):
            #     print(f"[display] ventas={total_ventas} ingresos={total_ingresos:.2f}")
        
    t1 = time.perf_counter()
    # print(f"[mostrar] {t1 - t0:.3f}s")

    shm_p.close(); shm_t.close(); shm_r.close(); shm_i.close()

def proc_precios(precio_name, rec_name, N, lock_precios, simulacion_activa):
    shm_p, precio = attach_shared_array(precio_name, (N,), np.float64)
    shm_r, ventas_recientes = attach_shared_array(rec_name, (N,), np.int32)

    rng = random.Random(12345)
    t0 = time.perf_counter()
    while simulacion_activa.value:
        with lock_precios:
            # ajusta precios en bloque
            vr = ventas_recientes
            mask_alta = (vr >= 8)
            mask_media = (~mask_alta) & (vr >= 4)
            mask_baja = ~mask_alta & ~mask_media

            # factores aleatorios
            f = np.empty_like(precio)
            f[mask_alta]  = 1.05 + np.random.random(mask_alta.sum()) * 0.10
            f[mask_media] = 0.95 + np.random.random(mask_media.sum()) * 0.10
            f[mask_baja]  = 0.90 + np.random.random(mask_baja.sum())  * 0.05

            np.multiply(precio, f, out=precio)
            np.clip(precio, PRECIO_MIN, PRECIO_MAX * 1.5, out=precio)

            ventas_recientes.fill(0)
    t1 = time.perf_counter()
    # print(f"[precios] {t1 - t0:.3f}s")

    shm_p.close(); shm_r.close()

def proc_ventas(precio_name, tot_name, rec_name, ing_name, N,
                lock_ventas, simulacion_activa, tiempo_simulacion, sim_time_limit):
    shm_p, precio = attach_shared_array(precio_name, (N,), np.float64)
    shm_t, ventas_totales = attach_shared_array(tot_name, (N,), np.int32)
    shm_r, ventas_recientes = attach_shared_array(rec_name, (N,), np.int32)
    shm_i, ingresos_totales = attach_shared_array(ing_name, (N,), np.float64)

    rng = np.random.default_rng(777)
    prob_base = 0.7
    t0 = time.perf_counter()
    while simulacion_activa.value:
        with tiempo_simulacion.get_lock():
            tiempo_simulacion.value += 1
            ts = tiempo_simulacion.value
            if ts >= sim_time_limit:
                simulacion_activa.value = 0

        # calcula prob por puesto
        factor_precio = (PRECIO_MAX - precio) / PRECIO_MAX
        prob = prob_base * (0.5 + factor_precio)
        clientes = rng.integers(1, 6, size=N, endpoint=False)

        ventas_exitosas = rng.binomial(clientes, np.clip(prob, 0.0, 1.0))
        cantidades = rng.integers(1, 4, size=N, endpoint=False)

        delta = (ventas_exitosas * cantidades).astype(np.int32)
        ingreso_delta = delta.astype(np.float64) * precio

        # protege la escritura conjunta (como un lock en C)
        with lock_ventas:
            np.add(ventas_totales,  delta, out=ventas_totales, casting="unsafe")
            np.add(ventas_recientes,delta, out=ventas_recientes, casting="unsafe")
            np.add(ingresos_totales,ingreso_delta, out=ingresos_totales, casting="unsafe")
    t1 = time.perf_counter()
    # print(f"[ventas] {t1 - t0:.3f}s")

    shm_p.close(); shm_t.close(); shm_r.close(); shm_i.close()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--puestos", type=int, default=NUM_PUESTOS_DEFAULT)
    ap.add_argument("--tiempo",  type=int, default=SIMULACION_TIEMPO_DEFAULT)
    ap.add_argument("--imprimir-cada", type=float, default=0.0,
                    help="Segundos entre impresiones en display (0 = sin imprimir)")
    args = ap.parse_args()

    N = int(args.puestos)
    sim_limit = int(args.tiempo)

    # crea shared arrays (padre)
    shm_p, precio, precio_name = make_shared_array("precio", (N,), np.float64)
    shm_t, ventas_totales, tot_name = make_shared_array("ventas_totales", (N,), np.int32)
    shm_r, ventas_recientes, rec_name = make_shared_array("ventas_recientes", (N,), np.int32)
    shm_i, ingresos_totales, ing_name = make_shared_array("ingresos_totales", (N,), np.float64)

    # inicializa
    rng = np.random.default_rng(42)
    precio[:] = PRECIO_MIN + rng.random(N) * (PRECIO_MAX - PRECIO_MIN)
    ventas_totales.fill(0)
    ventas_recientes.fill(0)
    ingresos_totales.fill(0)

    # locks y banderas
    lock_ventas  = Lock()
    lock_precios = Lock()
    lock_display = Lock()
    simulacion_activa = Value('i', 1)
    tiempo_simulacion = Value('i', 0)

    # procesos
    p_display = Process(target=proc_mostrar,
                        args=(precio_name, tot_name, rec_name, ing_name, N,
                              lock_display, simulacion_activa, args.imprimir_cada))
    p_precios = Process(target=proc_precios,
                        args=(precio_name, rec_name, N, lock_precios, simulacion_activa))
    p_ventas = Process(target=proc_ventas,
                       args=(precio_name, tot_name, rec_name, ing_name, N,
                             lock_ventas, simulacion_activa, tiempo_simulacion, sim_limit))

    t0 = time.perf_counter()
    p_display.start(); p_precios.start(); p_ventas.start()
    p_display.join();  p_precios.join();  p_ventas.join()
    t1 = time.perf_counter()

    # resumen (padre)
    total_ingresos = float(ingresos_totales.sum())
    total_ventas = int(ventas_totales.sum())
    precio_prom = float(precio.mean())
    puesto_mas = int(np.argmax(ventas_totales))
    # print resultados (pocos)
    print("=================================")
    print(f"RESUMEN FINAL DESPUES DE {sim_limit} SEGUNDOS:")
    print("=================================")
    print(f"Puesto mas exitoso: Puesto {puesto_mas+1} ({ventas_totales[puesto_mas]} ventas)")
    print(f"Ingresos totales: Q{total_ingresos:.2f}")
    print(f"Shucos vendidos: {total_ventas}")
    print(f"Precio promedio final: Q{precio_prom:.2f}")
    print(f"Ingreso por shuco: Q{(total_ingresos/total_ventas) if total_ventas>0 else 0:.2f}")
    print(f"Tiempo total simulacion : {t1 - t0:.6f} s")

    # cleanup shared memory
    shm_p.close(); shm_t.close(); shm_r.close(); shm_i.close()
    shm_p.unlink(); shm_t.unlink(); shm_r.unlink(); shm_i.unlink()

if __name__ == "__main__":
    main()
