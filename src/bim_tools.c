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

#include "bim_tools.h"

void        _list_sort      (ArrayList *list, ArrayListCompareFunc compare_func);
int32_t     _zone_id_cmp    (const ArrayListValue value1, const ArrayListValue value2);
int32_t     _transit_id_cmp (const ArrayListValue value1, const ArrayListValue value2);
bim_zone_t* _outside_init   (const bim_json_object_t *bim_json);
int         _calculate_transits_width(ArrayList *zones, ArrayList *transits);

bim_t* bim_tools_new(const bim_json_object_t *const bim_json)
{
    const bim_json_object_t* const jbim = bim_json;

    bim_t *bim = NULL;
    bim = (bim_t *)calloc(1, sizeof(bim_t));
    if (!bim) {
        return NULL;
    }

    ArrayList *zones_list = arraylist_new(0);
    if (!zones_list) {
        free(bim);
        return NULL;
    }
    ArrayList *transits_list = arraylist_new(0);
    if (!transits_list) {
        arraylist_free(zones_list);
        free(bim);
        return NULL;
    }

    bim->transits = transits_list;
    bim->zones    = zones_list;

    bim->name        = strdup(jbim->name);
    bim->numoflevels = jbim->numoflevels;

    bim_level_t *levels = (bim_level_t *) calloc(bim->numoflevels, sizeof (bim_level_t));
    if (!levels) {
        arraylist_free(zones_list);
        arraylist_free(transits_list);
        free(bim->name);
        free(bim);
        return NULL;
    }
    bim->levels = levels;

    // Обход уровней
    bim_json_level_t *jlevels = jbim->levels;
    for(size_t i = 0; i < bim->numoflevels; i++)
    {
        bim_json_level_t jlevel = jlevels[i];

        bim_level_t   *level    = NULL;
        bim_zone_t    *zones    = NULL;
        bim_transit_t *transits = NULL;

        zones = (bim_zone_t *) calloc(jlevel.numofelements, sizeof (bim_zone_t));
        if (!zones) {
            arraylist_free(zones_list);
            arraylist_free(transits_list);
            free(levels);
            free(bim);
            return NULL;
        }
        transits = (bim_transit_t *) calloc(jlevel.numofelements, sizeof (bim_transit_t));
        if (!transits) {
            arraylist_free(zones_list);
            arraylist_free(transits_list);
            free(levels);
            free(zones);
            free(bim);
            return NULL;
        }

        size_t numofzones = 0;
        size_t numoftransits = 0;
        // Обход элементов уровня
        for(size_t j = 0; j < jlevel.numofelements; j++)
        {
            bim_json_element_t jelement = jlevel.elements[j];
            if (jelement.sign == ROOM || jelement.sign == STAIRCASE)
            {
                bim_zone_t *zone = &zones[numofzones];
                zone->id = jelement.id;
                strcpy((void(*))zone->uuid.x, jelement.uuid.x);
                zone->name    = strdup(jelement.name);
                zone->size_z  = jelement.size_z;
                zone->z_level = jelement.z_level;
                zone->sign    = jelement.sign;
                zone->numofpeople = jelement.numofpeople;

                zone->numofoutputs = jelement.numofoutputs;
                zone->outputs = (uuid_t*)NULL;
                zone->outputs = (uuid_t*)calloc(zone->numofoutputs, sizeof(uuid_t));
                if (!zone->outputs) {
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                for (int idx_out = 0; idx_out < zone->numofoutputs; ++idx_out)
                {
                     strcpy((void *)zone->outputs[idx_out].x, jelement.outputs[idx_out].x);
                }

                zone->polygon = (polygon_t*)NULL;
                zone->polygon = (polygon_t*)calloc(1, sizeof (polygon_t));
                if (!zone->polygon) {
                    free(zone->outputs);
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                zone->polygon->numofpoints = jelement.polygon->numofpoints;
                zone->polygon->points = (point_t*)NULL;
                zone->polygon->points = (point_t*)calloc(zone->polygon->numofpoints, sizeof (point_t));
                if (!zone->polygon->points) {
                    free(zone->polygon);
                    free(zone->outputs);
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                for (size_t idx_p = 0; idx_p < zone->polygon->numofpoints; ++idx_p)
                {
                    point_t *pt = NULL;
                    pt = &zone->polygon->points[idx_p];
                    pt->x = jelement.polygon->points[idx_p].x;
                    pt->y = jelement.polygon->points[idx_p].y;
                }

                zone->area = geom_tools_area_polygon(zone->polygon);
                zone->is_blocked = false;
                zone->is_visited = false;
                zone->is_safe    = false;
                zone->potential  = __FLT_MAX__;
                zone->hazard_level = 0;
                arraylist_append(zones_list, zone);
                numofzones++;
            }
            else if (jelement.sign == DOOR_WAY || jelement.sign == DOOR_WAY_OUT || jelement.sign == DOOR_WAY_INT)
            {
                bim_transit_t *transit = &transits[numoftransits];
                transit->id = jelement.id;
                transit->name = strdup(jelement.name);
                strcpy((void(*))transit->uuid.x, jelement.uuid.x);
                transit->size_z = jelement.size_z;
                transit->z_level = jelement.z_level;
                transit->sign  = jelement.sign;

                transit->numofoutputs = jelement.numofoutputs;
                transit->outputs = (uuid_t*)NULL;
                transit->outputs = (uuid_t*)calloc(transit->numofoutputs, sizeof (uuid_t));
                if (!transit->outputs) {
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                for (int idx_out = 0; idx_out < transit->numofoutputs; ++idx_out)
                {
                     strcpy((void *)transit->outputs[idx_out].x, jelement.outputs[idx_out].x);
                }

                transit->polygon = (polygon_t*)NULL;
                transit->polygon = (polygon_t*)calloc(1, sizeof (polygon_t));
                if (!transit->polygon) {
                    free(transit->outputs);
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                transit->polygon->numofpoints = jelement.polygon->numofpoints;
                transit->polygon->points = (point_t*)NULL;
                transit->polygon->points = calloc(transit->polygon->numofpoints, sizeof (point_t));
                if (!transit->polygon->points) {
                    free(transit->polygon);
                    free(transit->outputs);
                    free(zones);
                    free(transits);
                    free(levels);
                    arraylist_free(zones_list);
                    arraylist_free(transits_list);
                    free(bim);
                    return NULL;
                }

                for (size_t idx_p = 0; idx_p < transit->polygon->numofpoints; ++idx_p)
                {
                    point_t *pt = NULL;
                    pt = &transit->polygon->points[idx_p];
                    pt->x = jelement.polygon->points[idx_p].x;
                    pt->y = jelement.polygon->points[idx_p].y;
                }

                transit->is_blocked = false;
                transit->is_visited = false;
                transit->nop_proceeding = 0;
                transit->width = -1; //Calculated below
                arraylist_append(transits_list, transit);
                numoftransits++;
            }
        }

        level = &levels[i];
        level->name = strdup(jlevel.name);
        level->z_level = jlevel.z_level;
        level->zones = zones;
        level->transits = transits;
        level->numofzones = numofzones;
        level->numoftransits = numoftransits;

        if (level->numofzones == 0 || level->numoftransits == 0)
            fprintf(stderr, "[func: %s() | line: %u] :: zone_count (%u) or transit_count (%u) is zero\n", __func__, __LINE__, level->numofzones, level->numoftransits);
        else
        {
            level->zones    = (bim_zone_t*)   realloc(level->zones,    level->numofzones    * sizeof (bim_zone_t));
            level->transits = (bim_transit_t*)realloc(level->transits, level->numoftransits * sizeof (bim_transit_t));
        }
    }

    bim_zone_t *outside = _outside_init(bim_json);
    arraylist_append(zones_list, outside);

    arraylist_sort(zones_list,    _zone_id_cmp);
    arraylist_sort(transits_list, _transit_id_cmp);

    _calculate_transits_width(zones_list, transits_list);

    return bim;
}

int _find_zone_callback(ArrayListValue value1, ArrayListValue value2)
{
    const bim_zone_t *zone = (bim_zone_t *)value1;
    uuid_t *uuid = (uuid_t *)value2;

    for (size_t i = 0; i < UUID_SIZE; ++i)
    {
        if (zone->uuid.x[i] != uuid->x[i])
        {
            return 0;
        }
    }

    return 1;
}

line_t* _intersected_edge(const polygon_t *aPolygonElement, const line_t *aLine)
{
    line_t *line = (line_t *)NULL;
    line = (line_t *)calloc(1, sizeof (line_t));

    line->p1 = (point_t *)NULL;
    line->p2 = (point_t *)NULL;
    line->p1 = (point_t *)calloc(1, sizeof (point_t));
    line->p2 = (point_t *)calloc(1, sizeof (point_t));

    uint8_t numOfIntersect = 0;
    for (size_t i = 1; i < aPolygonElement->numofpoints; ++i)
    {
        point_t *pointElementA = &aPolygonElement->points[i-1];
        point_t *pointElementB = &aPolygonElement->points[i];
        line_t line_tmp = {pointElementA, pointElementB};
        bool isIntersect = geom_tools_is_intersect_line(aLine, &line_tmp);
        if (isIntersect)
        {
            line->p1 = pointElementA;
            line->p2 = pointElementB;
            numOfIntersect++;
        }
    }

    if (numOfIntersect != 1) {
        fprintf(stderr, "[func: %s() | line: %u] :: Ошибка геометрии. Проверьте правильность ввода дверей и вирутальных проемов.\n", __func__, __LINE__);
        free(line->p1);
        free(line->p2);
        free(line);
        return (line_t *)NULL;
    }

    return line;
}

double _width_door_way(const polygon_t *zone1, const polygon_t *zone2, const line_t *edge1, const line_t *edge2)
{
    /*
     * Возможные варианты стыковки помещений, которые соединены проемом
     * Код ниже определяет область их пересечения
       +----+  +----+     +----+
            |  |               | +----+
            |  |               | |
            |  |               | |
       +----+  +----+          | |
                               | +----+
       +----+             +----+
            |  +----+
            |  |          +----+ +----+
            |  |               | |
       +----+  |               | |
               +----+          | +----+
                          +----+
     *************************************************************************
     * 1. Определить грани помещения, которые пересекает короткая сторона проема
     * 2. Вычислить среднее проекций граней друг на друга
     */

    point_t *l1p1 = edge1->p1;
    point_t *l1p2 = edge2->p2;
    double length1 = geom_tools_length_side(l1p1, l1p2);

    point_t *l2p1 = edge1->p1;
    point_t *l2p2 = edge2->p2;
    double length2 = geom_tools_length_side(l2p1, l2p2);

    // Короткая линия проема, которая пересекает оба помещения
    line_t dline = {NULL, NULL};
    if (length1 >= length2)
    {
        dline.p1 = l2p1;
        dline.p2 = l2p2;
    } else
    {
        dline.p1 = l1p1;
        dline.p2 = l1p2;
    }

    // Линии, которые находятся друг напротив друга и связаны проемом
    line_t *edgeElementA = NULL;
    line_t *edgeElementB = NULL;
    edgeElementA = _intersected_edge(zone1, &dline);
    if (!edgeElementA) {
        return -1;
    }
    edgeElementB = _intersected_edge(zone2, &dline);
    if (!edgeElementB) {
        free(edgeElementA);
        return -1;
    }
    // Поиск точек, которые являются ближайшими к отрезку edgeElement
    // Расстояние между этими точками и является шириной проема
    point_t *pt1 = geom_tools_nearest_point(edgeElementA->p1, edgeElementB);
    point_t *pt2 = geom_tools_nearest_point(edgeElementA->p2, edgeElementB);
    double d12 = geom_tools_length_side(pt1, pt2);

    point_t *pt3 = geom_tools_nearest_point(edgeElementB->p1, edgeElementA);
    point_t *pt4 = geom_tools_nearest_point(edgeElementB->p2, edgeElementA);
    double d34 = geom_tools_length_side(pt3, pt4);

    free(pt1); free(pt2); free(pt3); free(pt4);
    free(edgeElementA); free(edgeElementB);

    return (d12 + d34) / 2;
}

// Вычисление ширины проема по данным из модели здания
int _calculate_transits_width(ArrayList *zones,    // Список всех зон
                              ArrayList *transits) // Список всех переходов
{
    for (size_t i = 0; i < transits->length; i++)
    {
        bim_transit_t *transit = NULL;
        transit = transits->data[i];
        uint8_t stair_sing_counter = 0; // Если stair_sing_counter = 2, то проем межэтажный (между лестницами)
        int zuuid = -1; // Идентификатор зоны
        bim_zone_t *t_realted_zones[2] = {(bim_zone_t*)NULL, (bim_zone_t*)NULL};

        for (size_t j = 0; j < transit->numofoutputs; j++)
        {
            zuuid = arraylist_index_of(zones, _find_zone_callback, (void *)transit->outputs[j].x);
            t_realted_zones[j] = (bim_zone_t *)zones->data[zuuid];
            if (t_realted_zones[j]->sign == STAIRCASE) stair_sing_counter++;
        }

        if (zuuid == -1)
        {
            LOG_ERROR("Не найден элемент, соединенный с переходом: id=%lu, name=%s [%s]",
                      transit->id, transit->uuid, transit->name);
            return -1;
        }

        if (stair_sing_counter == 2) // => Межэтажный проем
        {
            transit->width = sqrt((t_realted_zones[0]->area + t_realted_zones[1]->area)/2);
            continue;
        }

        line_t edge1 = {NULL, NULL};
        line_t edge2 = {NULL, NULL};
        size_t numofpoints_edge1 = 2;
        size_t numofpoints_edge2 = 2;

        const polygon_t *tpolygon = transit->polygon;
        for(size_t j = 0; j < tpolygon->numofpoints; ++j)
        {
            const point_t *tpoint = &tpolygon->points[j];
            const polygon_t *zpolygon = t_realted_zones[0]->polygon;
            size_t tpoint_in_zpolygon = geom_tools_is_point_in_polygon(tpoint, zpolygon);
            if (tpoint_in_zpolygon)
            {
                if (numofpoints_edge1 == 2)
                {
                    edge1.p1 = (point_t*)tpoint;
                }
                else if (numofpoints_edge1 == 1)
                {
                    edge1.p2 = (point_t*)tpoint;
                }
                else continue;
                --numofpoints_edge1;
            }
            else
            {
                if (numofpoints_edge2 == 2)
                {
                    edge2.p1 = (point_t*)tpoint;
                }
                else if (numofpoints_edge2 == 1)
                {
                    edge2.p2 = (point_t*)tpoint;
                }
                else continue;
                --numofpoints_edge2;
            }
        }

        double width = -1;
        if (numofpoints_edge1 || numofpoints_edge1)
        {
            LOG_ERROR("Невозможно вычислить ширину двери: id=%lu, uuid=%s [name=%s]",
                      transit->id, transit->uuid.x, transit->name);
            return -1;
        }

        if (transit->sign == DOOR_WAY_INT || transit->sign == DOOR_WAY_OUT)
        {
            point_t *l1p1 = edge1.p1;
            point_t *l1p2 = edge1.p2;
            double width1 = geom_tools_length_side(l1p1, l1p2);

            point_t *l2p1 = edge2.p1;
            point_t *l2p2 = edge2.p2;
            double width2 = geom_tools_length_side(l2p1, l2p2);

            width = (width1 + width2) / 2;
        } else if (transit->sign == DOOR_WAY)
        {
            width = _width_door_way(t_realted_zones[0]->polygon, t_realted_zones[1]->polygon, &edge1, &edge2);
        }

        transit->width = width;

        if (transit->width < 0)
        {
            LOG_ERROR("Ширина проема не определена: id=%lu, name=%s [%s], width=%f",
                     transit->id, transit->name, transit->uuid.x, transit->width);
            fprintf(stderr, "[func: %s() | line: %u] :: Ширина проема не определена: id=%lu, name=%s [%s], width=%f\n",
                    __func__, __LINE__, transit->id, transit->name, transit->uuid.x, transit->width);
        }
        else if (transit->width < 0.5)
        {
            LOG_WARN("Ширина проема меньше 0.5 м: id=%lu, name=%s [%s], width=%f",
                     transit->id, transit->name, transit->uuid.x, transit->width);
        }
    }

    return 0;
}

bim_zone_t* _outside_init(const bim_json_object_t * bim_json)
{
    bim_zone_t * outside = (bim_zone_t*)malloc(sizeof (bim_zone_t));
    if (!outside) {
        return NULL;
    }
    outside->id = 0;
    outside->name = strdup("Outside");
    outside->sign = OUTSIDE;
    outside->polygon = NULL;
    strcpy((void *)outside->uuid.x, "outside0-safe-zone-0000-000000000000");
    outside->z_level = 0;
    outside->size_z = __FLT_MAX__;
    outside->numofpeople = 0;
    outside->hazard_level = 0;
    outside->is_safe = true;

    size_t numofoutputs = 0;
    uuid_t *outputs = (uuid_t*)malloc(sizeof (uuid_t) * 100);

    for(size_t i = 0; i < bim_json->numoflevels; i++)
    {
        for(size_t j = 0; j < bim_json->levels[i].numofelements; j++)
        {
            bim_json_element_t *element = &bim_json->levels[i].elements[j];
            if (element->sign == DOOR_WAY_OUT)
            {
                uuid_t output = element->uuid;
                strcpy((void *)outputs[numofoutputs].x, output.x);
                numofoutputs++;
            }
            else if (element->sign == ROOM || element->sign == STAIRCASE)
                outside->id++;
        }
    }

    if (!numofoutputs) {
        free(outputs);
        free(outside);
        return (bim_zone_t*)NULL;
    }

    outside->numofoutputs = numofoutputs;
    outside->outputs = (uuid_t*)realloc(outputs, outside->numofoutputs * sizeof (uuid_t));
    outside->is_blocked = false;
    outside->is_visited = false;
    outside->potential = 0;
    outside->area = __FLT_MAX__;
    outside->numofpeople = 0;

    return outside;
}

bim_t* bim_tools_copy    (const bim_t * const bim)
{
    return (bim_t *)bim;
}

void bim_tools_free (bim_t* bim)
{
    bim_level_t *lvl_ptr = bim->levels;
    for(size_t i = 0; i < bim->numoflevels; i++, lvl_ptr++)
    {
        free(lvl_ptr->zones);
        free(lvl_ptr->transits);
    }
    free(bim->levels);
    bim_zone_t *outside = (bim_zone_t *)bim->zones->data[0];
//    free(outside->name);
//    free(outside->outputs);
//    free(outside);

    free(bim->name);

    arraylist_free(bim->zones);
    arraylist_free(bim->transits);
    free(bim);
}

void bim_tools_set_people_to_zone(bim_zone_t* zone, float num_of_people)
{
    zone->numofpeople = num_of_people;
}

double  bim_tools_get_numofpeople(const bim_t *const bim)
{
    double numofpeople = 0;
    for(size_t i = 0; i < bim->numoflevels; i++)
    {
        for (size_t j = 0; j < bim->levels[i].numofzones; j++)
        {
            const bim_zone_t *zone = &bim->levels[i].zones[j];
            numofpeople += zone->numofpeople;
        }
    }
    return numofpeople;
}

double bim_tools_get_area_bim(const bim_t * const bim)
{
    static double area = -1;
    if (area < 0)
    {
        area = 0;
        for (size_t i = 0; i < bim->numoflevels; i++)
        {
            for (size_t j = 0; j < bim->levels[i].numofzones; j++)
            {
                bim_zone_t zone = bim->levels[i].zones[j];
                if (zone.sign == ROOM || zone.sign == STAIRCASE)
                    area += bim->levels[i].zones[j].area;
            }
        }
    }
    return area;
}

void bim_tools_print_element(const bim_zone_t *zone)
{
    printf("Zone 'base' info: %p\n", zone);
    printf("\t%s: %lu\n", "ID", zone->id);
    printf("\t%s: %s\n", "Name", zone->name);
    printf("\t%s: %u\n", "Sign", zone->sign);
    printf("\t%s: %f\n", "Level", zone->z_level);
    printf("Zone 'add' info: %p\n", zone);
    printf("\t%s: %f\n", "Area", zone->area);
    printf("\t%s: %u\n", "Is visited", zone->is_visited);
    printf("\t%s: %u\n", "Is blocked", zone->is_blocked);
}

// -------------------------------------------------------
// *******************************************************
// -------------------------------------------------------

int32_t _zone_id_cmp (const ArrayListValue value1, const ArrayListValue value2)
{
    const bim_zone_t *e1 = (bim_zone_t *)value1;
    const bim_zone_t *e2 = (bim_zone_t *)value2;
    if (e1->id > e2->id) return 1;
    else if (e1->id < e2->id) return -1;
    else return 0;
}

int32_t _transit_id_cmp (const ArrayListValue value1, const ArrayListValue value2)
{
    const bim_transit_t *e1 = (bim_transit_t *)value1;
    const bim_transit_t *e2 = (bim_transit_t *)value2;
    if (e1->id > e2->id) return 1;
    else if (e1->id < e2->id) return -1;
    else return 0;
}
