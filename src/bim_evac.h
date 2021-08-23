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

#pragma once

#include "math.h"
#include "bim_graph.h"

void    evac_def_modeling_step (const bim_t *bim, uint64_t bim_element_count);
void    evac_bim_ext_init      (const ArrayList *zones, const ArrayList *transits);
void    evac_moving_step       (const bim_graph *graph, const ArrayList *zones, const ArrayList *transits);

double evac_time_m            (void);
double evac_time_s            (void);
void   evac_time_inc          (void);
void   evac_time_reset        (void);
