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
#include "bim_graph.h"
#include "bim_evac.h"
#include "logger.h"
#include "loggerconf.h"
#include "bim_configure.h"

void print_info(const double time, const ArrayList * zones, const double numofpeople)
{
    printf("%-6.2f", time);
    for (size_t i = 0; i < zones->length; i++)
    {
        bim_zone_t *zone = zones->data[i];
        printf("%8.2f", zone->num_of_people);
    }
    printf("%8.2f\n", numofpeople);
}

static void output_head(FILE *fp, bim_t *bim);
static void output_body(FILE *fp, bim_t *bim);
static void output_footer(FILE *fp, bim_t *bim);

int main (int argc, char** argv)
{
    // Обработка аргументов командной строки
    char *input_file = NULL;
    char *output_file = NULL;
    char *logger_config_file = NULL;
    char *bim_config_file = NULL;
    int c;
    while ((c = getopt (argc, argv, "c:l:o:f:")) != -1)
    {
        switch (c)
        {
        case 'c':
            bim_config_file = optarg;
            break;
        case 'l':
            logger_config_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'f':
            input_file = optarg;
            break;
        default:
            abort ();
        }
    }

    // Настройки с-logger
    logger_initConsoleLogger(stdout);
#ifndef NDEBUG
    logger_setLevel(LogLevel_DEBUG);
#else
    logger_setLevel(LogLevel_INFO);
#endif
    if (logger_config_file) logger_configure(logger_config_file);

    // Настроки bim
    if (bim_config_file) bim_configure(bim_config_file);

    // Создание структуры здания
    bim_t *bim = bim_tools_new(input_file);

    ArrayList * zones = bim->zones;
    if (cfg_distribution.type == Distribution_UNIFORM)
        for (size_t i = 0; i < zones->length; i++)
        {
            bim_zone_t *zone = zones->data[i];
            if (zone->base->sign != OUTSIDE)
                bim_tools_set_people_to_zone(zone, (zone->area * cfg_distribution.density));
        }

    ArrayList * transits = bim->transits;
    if (cfg_transit.type == TransitWidth_SPECIAL)
        for (size_t i = 0; i < transits->length; i++)
        {
            bim_transit_t *transit = transits->data[i];
            if (transit->base->sign == DOOR_WAY_INT && cfg_transit.doorway_in  > 0) transit->width = cfg_transit.doorway_in;
            if (transit->base->sign == DOOR_WAY_OUT && cfg_transit.doorway_out > 0) transit->width = cfg_transit.doorway_out;
        }

    LOG_TRACE("Файл описания объекта: %s", input_file);
    if (bim_config_file) LOG_TRACE("Файл конфигурации сценария: %s", bim_config_file);
    LOG_TRACE("Файл с детальной информацией: %s", output_file);
    LOG_TRACE("Название объекта: %s", bim->object->name);
    LOG_TRACE("Площадь здания: %.2f m^2", bim_tools_get_area_bim(bim));
    LOG_TRACE("Количество этажей: %i", bim->object->levels_count);
    LOG_TRACE("Количество помещений: %i", zones->length);
    LOG_TRACE("Количество дверей: %i", transits->length);
    LOG_TRACE("Количество человек в здании: %.2f чел.", bim_tools_get_numofpeople(bim));

    bim_graph *graph = bim_graph_new(bim);
    //bim_graph_print(graph);

    if (cfg_modeling.step > 0) evac_set_modeling_step(cfg_modeling.step);
    else evac_def_modeling_step(bim, zones->length);
    if (cfg_modeling.speed_max > 0) evac_set_speed_max(cfg_modeling.speed_max);
    if (cfg_modeling.density_max > 0) evac_set_density_max(cfg_modeling.density_max);
    if (cfg_modeling.density_min > 0) evac_set_density_min(cfg_modeling.density_min);

    evac_time_reset();

    // Файл с результатами
    FILE *fp = fopen(output_file, "w+");
    output_head(fp, bim);
    output_body(fp, bim);

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
               num_of_people += zone->num_of_people;
            }
        }
        output_body(fp, bim);

        if (num_of_people <= remainder) break;
    }

    LOG_INFO("---------------------------------------");
    LOG_INFO("Количество человек в здании: %.2f чел.", bim_tools_get_numofpeople(bim));
    LOG_INFO("Количество человек в безопасной зоне: %.2f чел.", ((bim_zone_t*)zones->data[zones->length-1])->num_of_people);
    LOG_INFO("Длительность эвакуации: %.2f с., %.2f мин.", evac_get_time_s(), evac_get_time_m());
    LOG_INFO("---------------------------------------");

    output_footer(fp, bim);
    bim_graph_free(graph);
    bim_tools_free(bim);
    return 0;
}

static void output_head(FILE *fp, bim_t *bim)
{
    fprintf(fp, "t;");
    for (size_t i = 0; i < bim->zones->length; i++)
    {
        bim_zone_t *zone = bim->zones->data[i];
        fprintf(fp, "%s;%.2f;;;", zone->base->name, zone->area);
    }
    for (size_t i = 0; i < bim->transits->length; i++)
    {
        bim_transit_t *transit = bim->transits->data[i];
        fprintf(fp, "%s;%.2f;;", transit->base->name, transit->width);
    }
    fprintf(fp, "\n");
    fprintf(fp, ";");
    for (size_t i = 0; i < bim->zones->length; i++)
    {
        fprintf(fp, "is_blocked;is_visited;num_of_people;potential;");
    }
    for (size_t i = 0; i < bim->transits->length; i++)
    {
        fprintf(fp, "is_blocked;is_visited;num_of_people;");
    }
    fprintf(fp, "\n");
}

static void output_body(FILE *fp, bim_t *bim)
{
    fprintf(fp, "%.2f;", evac_get_time_s());
    for (size_t i = 0; i < bim->zones->length; i++)
    {
        bim_zone_t *zone = bim->zones->data[i];
        fprintf(fp, "%u;%u;%.2f;%.2f;", zone->is_blocked, zone->is_visited, zone->num_of_people, zone->potential);
    }
    for (size_t i = 0; i < bim->transits->length; i++)
    {
        bim_transit_t *transit = bim->transits->data[i];
        fprintf(fp, "%u;%u;%.2f;", transit->is_blocked, transit->is_visited, transit->num_of_people);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

static void output_footer(FILE *fp, bim_t *bim __attribute__((unused)))
{
    fclose(fp);
}
