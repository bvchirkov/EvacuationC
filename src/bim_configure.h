/* Copyright © 2022 bvchirkov
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

#include <stdint.h>
#include <string.h>
#include "json.h"        ///< https://github.com/rbtylee/tutorial-jsonc/blob/master/tutorial/index.md
#include "bim_uuid.h"

#define UUID_SIZE 36 + 1

enum distribution_type
{
    distribution_from_bim,
    distribution_uniform
};

enum transits_width_type
{
    transits_width_from_bim,
    transits_width_users
};

typedef struct
{
    uuid_t  *uuid;
    uint8_t num_of_uuids;
    float   value;
} special_t;

typedef struct
{
    enum distribution_type  type;
    float                   density;
    special_t               *special;
    uint8_t                 num_of_special_blocks;
} bim_cfg_distribution_t;

typedef struct
{
    enum transits_width_type    type;
    float                       doorwayin;
    float                       doorwayout;
    special_t                   *special;
    uint8_t                     num_of_special_blocks;
} bim_cfg_transitions_width_t;

typedef struct
{
    float step;
    float speed_max;
    float density_min;
    float density_max;
} bim_cfg_modeling_t;

typedef struct
{
    const char x[256];
} bim_cfg_file_name_t;

typedef struct
{
    bim_cfg_file_name_t         *bim_jsons;
    bim_cfg_file_name_t         logger_configure;
    uint8_t                     num_of_bim_jsons;
    bim_cfg_distribution_t      distribution;
    bim_cfg_transitions_width_t transits;
    bim_cfg_modeling_t          modeling;
} bim_cfg_scenario_t;


const bim_cfg_scenario_t*   bim_cfg_load    (const char *filename);
void                        bim_cfg_unload  (bim_cfg_scenario_t* bim_cfg_scenario);

#endif /* BIMCONF_H */
