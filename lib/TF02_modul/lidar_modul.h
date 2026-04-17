#pragma once
#include <Arduino.h>

// ── Offentlig API ─────────────────────────────────────────────────────────────

/**
 * Initialiser TF02-Pro LiDAR-sensor.
 * Setter opp seriell kommunikasjon, millimeter-modus og maks frekvens.
 * Kall én gang i setup().
 */
void lidar_init();

/**
 * Les avstand fra sensoren.
 * 
 * @param avstand_ut  Settes til målt avstand i mm hvis målingen er gyldig.
 * @param styrke_ut   Settes til signalstyrke hvis målingen er gyldig.
 * @return            true hvis gyldig måling ble lest, false ellers.
 */
bool lidar_les(int &avstand_ut, int &styrke_ut);