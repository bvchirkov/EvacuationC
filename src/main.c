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

#include <stdio.h>
#include <string.h>
#include "bim/bim_json_object.h"
#include "bim_tools.h"
#include "bim_graph.h"
#include "bim_evac.h"

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

int main (int argc, char** argv)
{
    if (argc == 1)
    {
        printf("Укажите путь к файлу с описанием здания\n");
        return 1;
    } else if (argc > 2)
    {
        printf("Слишком много параметров\n");
        return 1;
    }

    bim_object_t *bim = bim_tools_new(argv[1]);

    ArrayList *zones = bim_tools_get_zones_list();
    ArrayList *transits = bim_tools_get_transits_list();
    for (size_t i = 0; i < zones->length; i++)
    {
        bim_zone_t *zone = zones->data[i];
        if (zone->base->sign != OUTSIDE)
            bim_tools_set_people_to_zone(zone, (zone->area * 0.2));
        //printf("Element id::name: %lu[%lu]::%-32s::%.2f\n",  zone->base->id, i, zone->base->name, zone->num_of_people);
    }
    printf("Файл описания объекта: %s\n", argv[1]);
    printf("Название объекта: %s\n", bim->name);
    printf("Площадь здания: %.2f m^2\n", bim_tools_get_area_bim(bim));
    printf("Количество этажей: %i\n", bim->levels_count);
    printf("Количество помещений: %i\n", zones->length);
    printf("Количество дверей: %i\n", transits->length);
    printf("Количество человек в здании: %.2f чел.\n", bim_tools_get_numofpeople(bim));

    bim_graph *graph = bim_graph_new(zones, transits);
    //bim_graph_print(graph);

    evac_def_modeling_step(bim, zones->length);
    evac_time_reset();
//    bim_transit_t *out1 = transits->data[2];
//    out1->is_blocked = false;

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

        if (num_of_people <= remainder) break;
    }

    printf("---------------------------------------\n");
    printf("Количество человек в здании: %.2f чел.\n", bim_tools_get_numofpeople(bim));
    printf("Количество человек в безопасной зоне: %.2f чел.\n", ((bim_zone_t*)zones->data[zones->length-1])->num_of_people);
    printf("Длительность эвакуации: %.2f с., %.2f мин.\n", evac_time_s(), evac_time_m());
    printf("---------------------------------------\n");

    arraylist_free(zones);
    arraylist_free(transits);
    bim_graph_free(graph);
    bim_tools_free(bim);

    return 0;
}
