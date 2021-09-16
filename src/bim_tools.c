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
int32_t     _id_cmp         (const ArrayListValue value1, const ArrayListValue value2);
bim_zone_t* _outside_init   (const bim_json_object_t *bim_json);
int         _calculate_transits_width(ArrayList *zones, ArrayList *transits);

bim_t* bim_tools_new(const char* file)
{
    bim_json_object_t *bim_json = bim_json_new(file);
    if (!bim_json)
    {
        LOG_ERROR("Не удалось заполнить структуру `bim_t`");
        return NULL;
    }

    ArrayList *zones_list = arraylist_new(1);
    ArrayList *transits_list = arraylist_new(1);

    bim_t *bim = (bim_t *)malloc(sizeof (bim_t));
    bim->transits = transits_list;
    bim->zones = zones_list;
    bim->json = bim_json;

    bim_object_t *bim_object = (bim_object_t *)malloc(sizeof (bim_object_t));
    if (!bim_object)
    {
        free(bim);
        return NULL;
    }

    bim->object = bim_object;
    bim_object->name = strdup(bim_json->name);
    bim_object->levels_count = bim_json->levels_count;

    bim_level_t *level_ext = (bim_level_t *) malloc(sizeof (bim_level_t) * bim_object->levels_count);
    bim_json_level_t *level = bim_json->levels;
    bim_object->levels = level_ext;

    for(size_t i = 0; i < bim_object->levels_count; i++, level_ext++, level++)
    {
        level_ext->name = strdup(level->name);
        level_ext->z_level = level->z_level;

        bim_zone_t *zones = (bim_zone_t *) malloc(sizeof (bim_zone_t) * level->elements_count);
        bim_transit_t *transits = (bim_transit_t *) malloc(sizeof (bim_transit_t) * level->elements_count);
        level_ext->zones = zones;
        level_ext->transits = transits;

        bim_json_element_t *element = level->elements;
        for(size_t j = 0; j < level->elements_count; j++, element++)
        {
            if (element->sign == ROOM || element->sign == STAIR)
            {
                zones->base = element;
                zones->is_blocked = false;
                zones->is_visited = false;
                zones->potential = __FLT_MAX__;
                zones->area = geom_tools_area_polygon(*element->polygon);
                zones->num_of_people = element->numofpeople;
                arraylist_append(zones_list, zones);
                zones++;
            }
            else if (element->sign == DOOR_WAY || element->sign == DOOR_WAY_OUT || element->sign == DOOR_WAY_INT)
            {
                transits->base = element;
                transits->is_blocked = false;
                transits->is_visited = false;
                transits->num_of_people = 0;
                transits->width = -1; //Calculated below
                arraylist_append(transits_list, transits);
                transits++;
            }
        }
        level_ext->zone_count = zones - level_ext->zones;
        level_ext->transit_count = transits - level_ext->transits;

        if (level_ext->zone_count == 0 || level_ext->transit_count == 0)
            fprintf(stderr, "[func: %s() | line: %u] :: zone_count (%u) or transit_count (%u) is zero\n", __func__, __LINE__, level_ext->zone_count, level_ext->transit_count);
        else
        {
            level_ext->zones = (bim_zone_t*)realloc(level_ext->zones, sizeof (bim_zone_t) * level_ext->zone_count);
            level_ext->transits = (bim_transit_t*)realloc(level_ext->transits, sizeof (bim_transit_t) * level_ext->transit_count);
        }
    }

    bim_object->outside = _outside_init(bim_json);
    arraylist_append(zones_list, bim_object->outside);

    arraylist_sort(zones_list, _id_cmp);
    arraylist_sort(transits_list, _id_cmp);

    _calculate_transits_width(zones_list, transits_list);

    return bim;
}

int _find_zone_callback(ArrayListValue value1, ArrayListValue value2)
{
    const bim_zone_t *zone = value1;
    char * uuid = value2;

    if (strcmp(zone->base->uuid, uuid) == 0) return 1;
    else return 0;

}

line_t* _intersected_edge(const polygon_t *aPolygonElement, const line_t *aLine)
{
    line_t *line = (line_t *)malloc(sizeof (line_t));
    line->p1 = (point_t *)malloc(sizeof (point_t));
    line->p2 = (point_t *)malloc(sizeof (point_t));

    uint8_t numOfIntersect = 0;
    for (size_t i = 1; i < aPolygonElement->point_count; ++i)
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

    if (numOfIntersect != 1)
        fprintf(stderr, "[func: %s() | line: %u] :: Ошибка геометрии. Проверьте правильность ввода дверей и вирутальных проемов.\n", __func__, __LINE__);

    return line;
}

double _width_door_way(const polygon_t *zone1, const polygon_t *zone2, const multiline_t *edge1, const multiline_t *edge2)
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

    point_t l1p1 = edge1->points[0];
    point_t l1p2 = edge2->points[0];
    double length1 = geom_tools_length_side(&l1p1, &l1p2);

    point_t l2p1 = edge1->points[0];
    point_t l2p2 = edge2->points[1];
    double length2 = geom_tools_length_side(&l2p1, &l2p2);

    // Короткая линия проема, которая пересекает оба помещения
    line_t dline = {NULL, NULL};
    if (length1 >= length2)
    {
        dline.p1 = &l2p1;
        dline.p2 = &l2p2;
    } else
    {
        dline.p1 = &l1p1;
        dline.p2 = &l1p2;
    }

    // Линии, которые находятся друг напротив друга и связаны проемом
    line_t *edgeElementA = _intersected_edge(zone1, &dline);
    line_t *edgeElementB = _intersected_edge(zone2, &dline);
    // Поиск точек, которые являются ближайшими к отрезку edgeElement
    // Расстояние между этими точками и является шириной проема
    point_t *pt1 = geom_tools_nearest_point(edgeElementA->p1, edgeElementB);
    point_t *pt2 = geom_tools_nearest_point(edgeElementA->p2, edgeElementB);
    double d12 = geom_tools_length_side(pt1, pt2);

    point_t *pt3 = geom_tools_nearest_point(edgeElementB->p1, edgeElementA);
    point_t *pt4 = geom_tools_nearest_point(edgeElementB->p2, edgeElementA);
    double d34 = geom_tools_length_side(pt3, pt4);

    free(edgeElementA); free(edgeElementB);
    free(pt1); free(pt2); free(pt3); free(pt4);

    return (d12 + d34) / 2;
}

// Вычисление ширины проема по данным из модели здания
int _calculate_transits_width(ArrayList *zones,    // Список всех зон
                              ArrayList *transits) // Список всех переходов
{
    for (size_t i = 0; i < transits->length; i++)
    {
        bim_transit_t *transit = transits->data[i];
        bim_json_element_t *btransit = transit->base;

        uint8_t stair_sing_counter = 0; // Если stair_sing_counter = 2, то проем межэтажный (между лестницами)
        int zuuid = -1; // Идентификатор зоны
        polygon_t zpolygons[btransit->outputs_count];

        for (size_t j = 0; j < btransit->outputs_count; j++)
        {
            zuuid = arraylist_index_of(zones, _find_zone_callback, btransit->outputs[j]);
            zpolygons[j] = *((bim_zone_t *)zones->data[zuuid])->base->polygon;
            if (((bim_zone_t *)zones->data[zuuid])->base->sign == STAIR) stair_sing_counter++;
        }

        if (zuuid == -1)
        {
            LOG_ERROR("Не найден элемент, соединенный с переходом: id=%lu, name=%s [%s]",
                      btransit->id, btransit->uuid, btransit->name);
            return -1;
        }

        if (stair_sing_counter == 2) // => Межэтажный проем
        {
            transit->width = sqrt((geom_tools_area_polygon(zpolygons[0]) + geom_tools_area_polygon(zpolygons[1]))/2);
            continue;
        }


        multiline_t edge1 = {.point_count=0, .points=(point_t *) malloc(sizeof (point_t) * 2)};
        multiline_t edge2 = {.point_count=0, .points=(point_t *) malloc(sizeof (point_t) * 2)};

        const polygon_t *tpolygon = btransit->polygon;
        for(size_t j = 1; j < tpolygon->point_count; ++j)
        {
            const point_t *tpoint = &tpolygon->points[j];
            const polygon_t *zpolygon = &zpolygons[0];
            uint8_t tpoint_in_zpolygon = geom_tools_is_point_in_polygon(tpoint, zpolygon);
            if (tpoint_in_zpolygon)
                edge1.points[edge1.point_count++] = *tpoint;
            else
                edge2.points[edge2.point_count++] = *tpoint;
        }

        double width = -1;
        if (edge1.point_count != 2 && edge2.point_count != 2)
        {
            free(edge1.points); free(edge2.points);
            LOG_ERROR("Невозможно вычислить ширину двери: id=%lu, name=%s [%s]",
                      btransit->id, btransit->uuid, btransit->name);
            return -1;
        }

        if (btransit->sign == DOOR_WAY_INT || btransit->sign == DOOR_WAY_OUT)
        {
            point_t l1p1 = edge1.points[0];
            point_t l1p2 = edge1.points[1];
            double width1 = geom_tools_length_side(&l1p1, &l1p2);

            point_t l2p1 = edge2.points[0];
            point_t l2p2 = edge2.points[1];
            double width2 = geom_tools_length_side(&l2p1, &l2p2);

            width = (width1 + width2) / 2;
        } else if (btransit->sign == DOOR_WAY)
        {
            width = _width_door_way(&zpolygons[0], &zpolygons[1], &edge1, &edge2);
        }

        transit->width = width;

        if (transit->width < 0.5)
        {
            LOG_WARN("Ширина проема меньше 0.5 м: id=%lu, name=%s [%s], width=%f",
                     btransit->id, btransit->name, btransit->uuid, transit->width);
        }

        free(edge1.points); free(edge2.points);
    }

    return 0;
}

bim_zone_t* _outside_init(const bim_json_object_t * bim_json)
{
    bim_json_element_t * outside_element = (bim_json_element_t*)malloc(sizeof (bim_json_element_t));
    if (!outside_element)
        return NULL;
    outside_element->id = 0;
    outside_element->name = strdup("Outside");
    outside_element->sign = OUTSIDE;
    outside_element->polygon = NULL;
    outside_element->uuid = strdup("00000000-0000-0000-0000-000000000000");
    outside_element->z_level = 0;
    outside_element->size_z = __FLT_MAX__;
    outside_element->numofpeople = 0;
    outside_element->outputs_count = 0;
    outside_element->outputs = (char**)malloc(sizeof (char*) * 100);
    char** ptr = outside_element->outputs;

    for(size_t i = 0; i < bim_json->levels_count; i++)
    {
        for(size_t j = 0; j < bim_json->levels[i].elements_count; j++)
        {
            bim_json_element_t *element = &bim_json->levels[i].elements[j];
            if (element->sign == DOOR_WAY_OUT)
            {
                outside_element->outputs_count++;
                *ptr++ = element->uuid;
            } else if (element->sign == ROOM || element->sign == STAIR)
                outside_element->id++;
        }
    }
    outside_element->outputs = /*(char**)*/realloc(outside_element->outputs, outside_element->outputs_count * sizeof (char*));

    bim_zone_t *outside_zone = (bim_zone_t *) malloc(sizeof (bim_zone_t));
    if (!outside_zone)
        return NULL;
    outside_zone->base = outside_element;
    outside_zone->is_blocked = false;
    outside_zone->is_visited = false;
    outside_zone->potential = 0;
    outside_zone->area = __FLT_MAX__;
    outside_zone->num_of_people = 0;

    return outside_zone;
}

bim_t* bim_tools_copy    (const bim_t* bim)
{
    return (bim_t *)bim;
}

void bim_tools_free (bim_t* bim)
{
    bim_object_t *bim_obj = bim->object;
    bim_level_t *lvl_ptr = bim_obj->levels;
    for(size_t i = 0; i < bim_obj->levels_count; i++, lvl_ptr++)
    {
        free(lvl_ptr->zones);
        free(lvl_ptr->transits);
    }
    free(bim_obj->levels);
    free(bim_obj->outside->base->name);
    free(bim_obj->outside->base->outputs);
    free(bim_obj->outside->base->uuid);
    free(bim_obj->outside->base);
    free(bim_obj->outside);

    free(bim_obj->name);
    free(bim_obj);

    bim_json_free(bim->json);
    arraylist_free(bim->zones);
    arraylist_free(bim->transits);
    free(bim);
}

void bim_tools_set_people_to_zone(bim_zone_t* zone, float num_of_people)
{
    zone->num_of_people = num_of_people;
}

double  bim_tools_get_numofpeople(const bim_t *bim)
{
    double numofpeople = 0;
    for(size_t i = 0; i < bim->object->levels_count; i++)
    {
        for (size_t j = 0; j < bim->object->levels[i].zone_count; j++)
        {
            const bim_zone_t *zone = &bim->object->levels[i].zones[j];
            numofpeople += zone->num_of_people;
        }
    }
    return numofpeople;
}

double bim_tools_get_area_bim(const bim_t* bim)
{
    double area = 0;
    for (size_t i = 0; i < bim->object->levels_count; i++)
    {
        for (size_t j = 0; j < bim->object->levels[i].zone_count; j++)
        {
            bim_zone_t zone = bim->object->levels[i].zones[j];
            if (zone.base->sign == ROOM || zone.base->sign == STAIR)
                area += bim->object->levels[i].zones[j].area;
        }
    }
    return area;
}

void bim_tools_print_element(const bim_zone_t *zone)
{
    printf("Zone 'base' info: %p\n", zone->base);
    printf("\t%s: %lu\n", "ID", zone->base->id);
    printf("\t%s: %s\n", "Name", zone->base->name);
    printf("\t%s: %u\n", "Sign", zone->base->sign);
    printf("\t%s: %f\n", "Level", zone->base->z_level);
    printf("Zone 'add' info: %p\n", zone);
    printf("\t%s: %f\n", "Area", zone->area);
    printf("\t%s: %u\n", "Is visited", zone->is_visited);
    printf("\t%s: %u\n", "Is blocked", zone->is_blocked);
}

void bim_tools_lists_delete (ArrayList ** lists)
{
    for (size_t i = 0; i < 5; i++)
    {
        arraylist_free(lists[i]);
    }
    free(lists);
}

// -------------------------------------------------------
// *******************************************************
// -------------------------------------------------------

int32_t _id_cmp (const ArrayListValue value1, const ArrayListValue value2)
{
    const bim_json_element_t *e1 = value1;
    const bim_json_element_t *e2 = value2;
    if (e1->id > e2->id) return 1;
    else if (e1->id < e2->id) return -1;
    else return 0;
}
