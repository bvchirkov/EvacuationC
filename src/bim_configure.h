/* Copyright © 2021 bvchirkov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BIMCONF_H
#define BIMCONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum cfg_distr
{
    Distribution_BIM,
    Distribution_UNIFORM
};

enum cfg_transit_width
{
    TransitWidth_BIM,
    TransitWidth_SPECIAL
};

typedef struct
{
    enum cfg_distr type;
    float density;
} _distribution;

typedef struct
{
    enum cfg_transit_width type;
    float doorway_in;
    float doorway_out;
} _transit;

typedef struct
{
    float step;
    float speed_max;
    float density_min;
    float density_max;
} _modeling;

extern _modeling        cfg_modeling;
extern _transit         cfg_transit;
extern _distribution    cfg_distribution;

/**
 * The following is the configurable key/value list.
 * ┌─────────────────────┬──────────────────────────────────────────────────┐
 * │key                  │ value                                            │
 * ├:--------------------┼:-------------------------------------------------┤
 * │distribution         │ BIM or UNIFORM                                   │
 * │distribution.density │ Плотность распределения людей, чел./м^2 (max = 9)│
 * │                     │                                                  │
 * │transit              │ BIM or SPECIAL                                   │
 * │transit.doorway.in   │ Ширина внутренних переходов (двери)              │
 * │transit.doorway.out  │ Ширина выходов из здания                         │
 * │                     │                                                  │
 * │modeling.step        │ Шаг моделирования                                │
 * │modeling.speed.max   │ Максимальная скорость движения людей             │
 * │modeling.density.min │ Минимальное значение плотности                   │
 * │modeling.density.max │ Максимальное значение плотности                  │
 * └─────────────────────┴──────────────────────────────────────────────────┘
 * @param[in] filename The name of the configuration file
 * @return Non-zero value upon success or 0 on error
 */
int bim_configure(const char* filename);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* BIMCONF_H */
