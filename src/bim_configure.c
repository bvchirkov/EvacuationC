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

#include "bim_configure.h"
#define streq(str1, str2) strcmp(str1, str2) == 0

const bim_cfg_scenario_t*   bim_cfg_load    (const char *filename)
{
    json_object *root = NULL;
    bim_cfg_scenario_t *cfg_scenarion = NULL;

    root = json_object_from_file(filename);
    if (!root) {
//        LOG_ERROR("Не удалось прочитать файл. Проверьте правильность имени файла и пути: %s", filename);
        return NULL;
    }

    cfg_scenarion = (bim_cfg_scenario_t*)malloc(sizeof (bim_cfg_scenario_t));
    if (!cfg_scenarion) {
//        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
        free(root);
        return NULL;
    }

    json_object *j_bim            = NULL;
    json_object *j_distribution   = NULL;
    json_object *j_transits       = NULL;
    json_object *j_modeling       = NULL;
    json_object_object_get_ex(root, "bim",          &j_bim);
    json_object_object_get_ex(root, "distribution", &j_distribution);
    json_object_object_get_ex(root, "transits",     &j_transits);
    json_object_object_get_ex(root, "modeling",     &j_modeling);

    uint8_t num_of_bims = json_object_array_length(j_bim);
    bim_cfg_file_name_t *bims = (bim_cfg_file_name_t*) malloc(sizeof (bim_cfg_file_name_t) * num_of_bims);
    if (!bims) {
//        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
        free(cfg_scenarion);
        free(root);
        return NULL;
    }
    for (size_t i = 0; i < num_of_bims; i++)
    {
        json_object *j_bim_item = json_object_array_get_idx(j_bim, i);
        strcpy(bims[i].x, json_object_get_string(j_bim_item));
    }
    cfg_scenarion->bim_jsons = bims;
    cfg_scenarion->num_of_bim_jsons = num_of_bims;

    json_object *j_distribution_type      = NULL;
    json_object *j_distribution_density   = NULL;
    json_object *j_distribution_special   = NULL;
    json_object_object_get_ex(j_distribution, "type",     &j_distribution_type);
    json_object_object_get_ex(j_distribution, "density",  &j_distribution_density);
    json_object_object_get_ex(j_distribution, "special",  &j_distribution_special);

    const char *distribution_type = json_object_get_string(j_distribution_type);
    if (streq(distribution_type, "from_bim"))     { cfg_scenarion->distribution.type = distribution_from_bim; }
    else if (streq(distribution_type, "uniform")) { cfg_scenarion->distribution.type = distribution_uniform;  }
    cfg_scenarion->distribution.density = json_object_get_double(j_distribution_density);

    uint8_t num_of_special_blocks = json_object_array_length(j_distribution_special);
    special_t *distribution_special = (special_t*) malloc(sizeof (special_t) * num_of_special_blocks);
    if (!distribution_special) {
//        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
        free(bims);
        free(cfg_scenarion);
        free(root);
        return NULL;
    }
    for (size_t i = 0; i < num_of_special_blocks; i++)
    {
        json_object *j_distribution_special_item = json_object_array_get_idx(j_distribution_special, i);

        json_object *j_distribution_special_uuids   = NULL;
        json_object *j_distribution_special_value   = NULL;
        json_object_object_get_ex(j_distribution_special_item, "uuid",     &j_distribution_special_uuids);
        json_object_object_get_ex(j_distribution_special_item, "density",  &j_distribution_special_value);

        uint8_t num_of_uuids = json_object_array_length(j_distribution_special_uuids);
        uuid_t *uuids = (uuid_t*) malloc(sizeof (uuid_t) * num_of_uuids);
        if (!uuids) {
    //        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
            free(distribution_special);
            free(bims);
            free(cfg_scenarion);
            free(root);
            return NULL;
        }
        for (size_t j = 0; j < num_of_uuids; j++)
        {
            json_object *j_uuids_item = json_object_array_get_idx(j_distribution_special_uuids, i);
            strcpy((void *)uuids[i].x, json_object_get_string(j_uuids_item));
        }
        distribution_special->num_of_uuids = num_of_uuids;
        distribution_special->value = json_object_get_double(j_distribution_special_value);
        distribution_special->uuid  = uuids;
        //cfg_scenarion->distribution.special.num_of_uuids = num_of_uuids;
    }
    cfg_scenarion->distribution.num_of_special_blocks = num_of_special_blocks;
    cfg_scenarion->distribution.special = distribution_special;

    json_object *j_transits_type       = NULL;
    json_object *j_transits_doorwayin  = NULL;
    json_object *j_transits_doorwayout = NULL;
    json_object *j_transits_special    = NULL;
    json_object_object_get_ex(j_transits, "type",       &j_transits_type);
    json_object_object_get_ex(j_transits, "doorwayin",  &j_transits_doorwayin);
    json_object_object_get_ex(j_transits, "doorwayout", &j_transits_doorwayout);
    json_object_object_get_ex(j_transits, "special",    &j_transits_special);

    const char *transits_type = json_object_get_string(j_transits_type);
    if (streq(transits_type, "from_bim"))   { cfg_scenarion->transits.type = transits_width_from_bim; }
    else if (streq(transits_type, "users")) { cfg_scenarion->transits.type = transits_width_users;    }
    cfg_scenarion->transits.doorwayin = json_object_get_double(j_transits_doorwayin);
    cfg_scenarion->transits.doorwayout = json_object_get_double(j_transits_doorwayout);

    num_of_special_blocks = json_object_array_length(j_transits_special);
    special_t *transits_special = (special_t*) malloc(sizeof (special_t) * num_of_special_blocks);
    if (!transits_special) {
//        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
        free(distribution_special);
        free(bims);
        free(cfg_scenarion);
        free(root);
        return NULL;
    }
    for (size_t i = 0; i < num_of_special_blocks; i++)
    {
        json_object *j_transits_special_item = json_object_array_get_idx(j_transits_special, i);

        json_object *j_transits_special_uuids   = NULL;
        json_object *j_transits_special_value   = NULL;
        json_object_object_get_ex(j_transits_special_item, "uuid",   &j_transits_special_uuids);
        json_object_object_get_ex(j_transits_special_item, "width",  &j_transits_special_value);

        uint8_t num_of_uuids = json_object_array_length(j_transits_special_uuids);
        uuid_t *uuids = (uuid_t*) malloc(sizeof (uuid_t) * num_of_uuids);
        if (!uuids) {
    //        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
            free(transits_special);
            free(distribution_special);
            free(bims);
            free(cfg_scenarion);
            free(root);
            return NULL;
        }
        for (size_t j = 0; j < num_of_uuids; j++)
        {
            json_object *j_uuids_item = json_object_array_get_idx(j_transits_special_uuids, i);
            strcpy((void *)uuids[i].x, json_object_get_string(j_uuids_item));
        }
        transits_special->num_of_uuids = num_of_uuids;
        transits_special->value = json_object_get_double(j_transits_special_value);
        transits_special->uuid  = uuids;
        //cfg_scenarion->distribution.special.num_of_uuids = num_of_uuids;
    }
    cfg_scenarion->transits.num_of_special_blocks = num_of_special_blocks;
    cfg_scenarion->transits.special = transits_special;

    json_object *j_modeling_step        = NULL;
    json_object *j_modeling_speed_max   = NULL;
    json_object *j_modeling_density_min = NULL;
    json_object *j_modeling_density_max = NULL;
    json_object_object_get_ex(j_modeling, "step",         &j_modeling_step);
    json_object_object_get_ex(j_modeling, "speed_max",    &j_modeling_speed_max);
    json_object_object_get_ex(j_modeling, "density_min",  &j_modeling_density_min);
    json_object_object_get_ex(j_modeling, "density_max",  &j_modeling_density_max);

    cfg_scenarion->modeling.step = json_object_get_double(j_modeling_step);
    cfg_scenarion->modeling.speed_max = json_object_get_double(j_modeling_speed_max);
    cfg_scenarion->modeling.density_min = json_object_get_double(j_modeling_density_min);
    cfg_scenarion->modeling.density_max = json_object_get_double(j_modeling_density_max);

    json_object_put(root);
    return cfg_scenarion;
}

void                        bim_cfg_unload  (bim_cfg_scenario_t* bim_cfg_scenario)
{
    free(bim_cfg_scenario->bim_jsons);
    free(bim_cfg_scenario);
}
