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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "loggerconf.h"

#include "bim_json_object.h"
#include "bim_tools.h"
#include "bim_graph.h"
#include "bim_evac.h"
#include "bim_cli.h"
#include "bim_configure.h"

void applying_scenario_bim_params(bim_t* bim, const bim_cfg_scenario_t* cfg_scenario);

int main (int argc, char** argv)
{
    // Настройки с-logger
    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_INFO);

    const cli_params_t* cli_params = chooser(argc, argv);
    const bim_cfg_scenario_t *bim_cfg_scenario = bim_cfg_load(cli_params->scenario_file);

    for (size_t bim_idx = 0; bim_idx < bim_cfg_scenario->num_of_bim_jsons; bim_idx++)
    {
        // Чтение файла и разворачивание его в структуру
        LOG_INFO("Use module `bim_json_object`. Read the file of bim and create a programming structure");
        const bim_json_object_t * bim_json = bim_json_new(bim_cfg_scenario->bim_jsons[bim_idx].x);
        {
            LOG_INFO("##Building info");
            LOG_INFO("Name: %s", bim_json->name);

            LOG_INFO("##Address info");
            LOG_INFO("City: %s", bim_json->address->city);
            LOG_INFO("Street: %s", bim_json->address->street_address);
            LOG_INFO("Additional: %s", bim_json->address->add_info);
        }

        bim_t *bim = bim_tools_new(bim_json);
        /*{
            LOG_INFO("##Levels info");
            LOG_INFO("Num of levels: %i", bim->numoflevels);

            for (size_t i = 0; i < bim->numoflevels; ++i)
            {
                LOG_INFO("##Level info");
                LOG_INFO("Level name: %s", bim->levels[i].name);
                LOG_INFO("Num of zones on the level: %i", bim->levels[i].numofzones);
                LOG_INFO("Num of transits on the level: %i", bim->levels[i].numoftransits);
                LOG_INFO("Level height over zero mark: %d", bim->levels[i].z_level);

                LOG_INFO("##Zones info");
                for (size_t j = 0; j < bim->levels[i].numofzones; ++j)
                {
                    LOG_INFO("Id: %zu", bim->levels[i].zones[j].id);
                    LOG_INFO("Element name: %s | UUID: %s", bim->levels[i].zones[j].name, bim->levels[i].zones[j].uuid.x);
                    LOG_INFO("Num of outputs: %i", bim->levels[i].zones[j].numofoutputs);
                    LOG_INFO("Area: %f", bim->levels[i].zones[j].area);
                }

                LOG_INFO("##Transits info");
                for (size_t j = 0; j < bim->levels[i].numoftransits; ++j)
                {
                    LOG_INFO("Id: %zu", bim->levels[i].transits[j].id);
                    LOG_INFO("Element name: %s | UUID: %s", bim->levels[i].transits[j].name, bim->levels[i].transits[j].uuid.x);
                    LOG_INFO("Num of outputs: %i", bim->levels[i].transits[j].numofoutputs);
                    LOG_INFO("Width: %f", bim->levels[i].transits[j].width);
                }
            }

            LOG_INFO("Outside id: %u", ((bim_zone_t *)bim->zones->data[bim->zones->length-1])->id);
            LOG_INFO("Outside uuid: %s", ((bim_zone_t *)bim->zones->data[bim->zones->length-1])->uuid.x);
        }*/

        applying_scenario_bim_params(bim, bim_cfg_scenario);

        bim_graph_t *graph = bim_graph_new(bim);
        //bim_graph_print(graph);

        ArrayList * transits = bim->transits;
        ArrayList * zones    = bim->zones;
        evac_def_modeling_step(bim);
        evac_time_reset();

        double remainder = 0.0; // Количество человек, которое может остаться в зд. для остановки цикла
        while(true)
        {
            evac_moving_step(graph, zones, transits);
            evac_time_inc();

            double num_of_people = 0;
            for (size_t i = 0; i < zones->length; i++)
            {
                bim_zone_t *zone = zones->data[i];
                if (zone->is_visited)
                {
                   num_of_people += zone->numofpeople;
                }
            }

            if (num_of_people <= remainder) break;
        }

        LOG_INFO("---------------------------------------");
        LOG_INFO("Количество человек в здании: %.2f чел.", bim_tools_get_numofpeople(bim));
        LOG_INFO("Количество человек в безопасной зоне: %.2f чел.", ((bim_zone_t*)zones->data[OUTSIDE_IDX(bim)])->numofpeople);
        LOG_INFO("Длительность эвакуации: %.2f с., %.2f мин.", evac_get_time_s(), evac_get_time_m());
        LOG_INFO("---------------------------------------");

        bim_graph_free(graph);
        bim_tools_free(bim);
    }
}

void applying_scenario_bim_params(bim_t* bim, const bim_cfg_scenario_t* cfg_scenario)
{
    ArrayList *transits = bim->transits;
    for (size_t i = 0; i < transits->length; ++i)
    {
        bim_transit_t *transit = transits->data[i];

        if (cfg_scenario->transits.type == transits_width_users)
        {
            if (transit->sign == DOOR_WAY_INT) transit->width = cfg_scenario->transits.doorwayin;
            if (transit->sign == DOOR_WAY_OUT) transit->width = cfg_scenario->transits.doorwayout;
        }

        // A special setting up the transit width of item of bim
        for (size_t s = 0; s < cfg_scenario->transits.num_of_special_blocks; s++)
        {
            const special_t special = cfg_scenario->transits.special[s];
            for (size_t u = 0; u < special.num_of_uuids; u++)
            {
                if (uuideq(transit->uuid.x, special.uuid[u].x))
                {
                    transit->width = special.value;
                }
            }
        }
    }

    ArrayList * zones = bim->zones;
    for (size_t i = 0; i < zones->length; ++i)
    {
        bim_zone_t *zone = zones->data[i];
        if (zone->sign == OUTSIDE) continue;

        if (cfg_scenario->distribution.type == distribution_uniform)
        {
            bim_tools_set_people_to_zone(zone, (zone->area * cfg_scenario->distribution.density));
        }

        // A special setting up the density of item of bim
        for (size_t s = 0; s < cfg_scenario->distribution.num_of_special_blocks; s++)
        {
            const special_t special = cfg_scenario->distribution.special[s];
            for (size_t u = 0; u < special.num_of_uuids; u++)
            {
                if (uuideq(zone->uuid.x, special.uuid[u].x))
                {
                    bim_tools_set_people_to_zone(zone, (zone->area * special.value));
                }
            }
        }
    }

    evac_set_modeling_step(cfg_scenario->modeling.step);
    evac_set_speed_max(cfg_scenario->modeling.speed_max);
    evac_set_density_max(cfg_scenario->modeling.density_max);
    evac_set_density_min(cfg_scenario->modeling.density_min);
}

void none(void)
{
//static void output_head(FILE *fp, bim_t *bim);
//static void output_body(FILE *fp, bim_t *bim);
//static void output_footer(FILE *fp, bim_t *bim);

//int main (int argc, char** argv)
//{
//    // Настройки с-logger
//    logger_initConsoleLogger(stdout);
//#ifndef NDEBUG
//    logger_setLevel(LogLevel_DEBUG);
//#else
//    logger_setLevel(LogLevel_INFO);
//#endif
//    if (logger_config_file) logger_configure(logger_config_file);

//    // Настроки bim
//    if (bim_config_file) bim_configure(bim_config_file);

//    // Создание структуры здания
//    bim_t *bim = bim_tools_new(input_file);

//    ArrayList * zones = bim->zones;
//    if (cfg_distribution.type == Distribution_UNIFORM)
//        for (size_t i = 0; i < zones->length; i++)
//        {
//            bim_zone_t *zone = zones->data[i];
//            if (zone->base->sign != OUTSIDE)
//                bim_tools_set_people_to_zone(zone, (zone->area * cfg_distribution.density));
//        }

//    ArrayList * transits = bim->transits;
//    if (cfg_transit.type == TransitWidth_SPECIAL)
//        for (size_t i = 0; i < transits->length; i++)
//        {
//            bim_transit_t *transit = transits->data[i];
//            if (transit->base->sign == DOOR_WAY_INT && cfg_transit.doorway_in  > 0) transit->width = cfg_transit.doorway_in;
//            if (transit->base->sign == DOOR_WAY_OUT && cfg_transit.doorway_out > 0) transit->width = cfg_transit.doorway_out;
//        }

//    LOG_TRACE("Файл описания объекта: %s", input_file);
//    if (bim_config_file) LOG_TRACE("Файл конфигурации сценария: %s", bim_config_file);
//    LOG_TRACE("Файл с детальной информацией: %s", output_file);
//    LOG_TRACE("Название объекта: %s", bim->object->name);
//    LOG_TRACE("Площадь здания: %.2f m^2", bim_tools_get_area_bim(bim));
//    LOG_TRACE("Количество этажей: %i", bim->object->levels_count);
//    LOG_TRACE("Количество помещений: %i", zones->length);
//    LOG_TRACE("Количество дверей: %i", transits->length);
//    LOG_TRACE("Количество человек в здании: %.2f чел.", bim_tools_get_numofpeople(bim));

//    bim_graph_t *graph = bim_graph_new(bim);
//    //bim_graph_print(graph);

//    if (cfg_modeling.step > 0) evac_set_modeling_step(cfg_modeling.step);
//    else evac_def_modeling_step(bim, zones->length);
//    if (cfg_modeling.speed_max > 0) evac_set_speed_max(cfg_modeling.speed_max);
//    if (cfg_modeling.density_max > 0) evac_set_density_max(cfg_modeling.density_max);
//    if (cfg_modeling.density_min > 0) evac_set_density_min(cfg_modeling.density_min);

//    evac_time_reset();

//    // Файл с результатами
//    FILE *fp = fopen(output_file, "w+");
//    output_head(fp, bim);
//    output_body(fp, bim);

//    double remainder = 0.0; // Количество человек, которое может остаться в зд. для остановки цикла
//    while(true)
//    {
//        evac_moving_step(graph, zones, transits);
//        evac_time_inc();

//        double num_of_people = 0;
//        for (size_t i = 0; i < zones->length; i++)
//        {
//            bim_zone_t *zone = zones->data[i];
//            if (zone->is_visited)
//            {
//               num_of_people += zone->num_of_people;
//            }
//        }
//        output_body(fp, bim);

//        if (num_of_people <= remainder) break;
//    }

//    LOG_INFO("---------------------------------------");
//    LOG_INFO("Количество человек в здании: %.2f чел.", bim_tools_get_numofpeople(bim));
//    LOG_INFO("Количество человек в безопасной зоне: %.2f чел.", ((bim_zone_t*)zones->data[zones->length-1])->num_of_people);
//    LOG_INFO("Длительность эвакуации: %.2f с., %.2f мин.", evac_get_time_s(), evac_get_time_m());
//    LOG_INFO("---------------------------------------");

//    output_footer(fp, bim);
//    bim_graph_free(graph);
//    bim_tools_free(bim);
//    return 0;
//}

//static void output_head(FILE *fp, bim_t *bim)
//{
//    fprintf(fp, "t;");
//    for (size_t i = 0; i < bim->zones->length; i++)
//    {
//        bim_zone_t *zone = bim->zones->data[i];
//        fprintf(fp, "%s;%.2f;;;", zone->base->name, zone->area);
//    }
//    for (size_t i = 0; i < bim->transits->length; i++)
//    {
//        bim_transit_t *transit = bim->transits->data[i];
//        fprintf(fp, "%s;%.2f;;", transit->base->name, transit->width);
//    }
//    fprintf(fp, "\n");
//    fprintf(fp, ";");
//    for (size_t i = 0; i < bim->zones->length; i++)
//    {
//        fprintf(fp, "is_blocked;is_visited;num_of_people;potential;");
//    }
//    for (size_t i = 0; i < bim->transits->length; i++)
//    {
//        fprintf(fp, "is_blocked;is_visited;num_of_people;");
//    }
//    fprintf(fp, "\n");
//}

//static void output_body(FILE *fp, bim_t *bim)
//{
//    fprintf(fp, "%.2f;", evac_get_time_s());
//    for (size_t i = 0; i < bim->zones->length; i++)
//    {
//        bim_zone_t *zone = bim->zones->data[i];
//        fprintf(fp, "%u;%u;%.2f;%.2f;", zone->is_blocked, zone->is_visited, zone->num_of_people, zone->potential);
//    }
//    for (size_t i = 0; i < bim->transits->length; i++)
//    {
//        bim_transit_t *transit = bim->transits->data[i];
//        fprintf(fp, "%u;%u;%.2f;", transit->is_blocked, transit->is_visited, transit->num_of_people);
//    }
//    fprintf(fp, "\n");
//    fflush(fp);
//}

//static void output_footer(FILE *fp, bim_t *bim __attribute__((unused)))
//{
//    fclose(fp);
//}
}
