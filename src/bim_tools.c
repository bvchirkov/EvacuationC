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


void        _bim_tools_list_sort            (ArrayList *list, ArrayListCompareFunc compare_func);
double      _bim_tools_length_side          (const bim_geometry_point_t *p1, const bim_geometry_point_t *p2);
int32_t*    _bim_tools_triangle_polygon     (const bim_geometry_polygon_t *element_polygon);
int32_t     _bim_tools_element_cmp          (ArrayListValue value1, ArrayListValue value2);
double      _bim_tools_get_area_element     (const bim_geometry_polygon_t *element);
bim_zone_t* _outside_init                   (const bim_json_object_t *bim_json);

ArrayList *zones_list = NULL;
ArrayList *transits_list = NULL;
bim_json_object_t *bim_json = NULL;

bim_object_t* bim_tools_new(const char* file)
{
    bim_json = bim_json_new(file);
    zones_list = arraylist_new(1);
    transits_list = arraylist_new(1);

    bim_object_t *bim = (bim_object_t *)malloc(sizeof (bim_object_t));
    bim->name = strdup(bim_json->name);
    bim->levels_count = bim_json->levels_count;
    bim->outside = _outside_init(bim_json);
    arraylist_append(zones_list, bim->outside);

    bim_level_t *level_ext = (bim_level_t *) malloc(sizeof (bim_level_t) * bim->levels_count);
    bim_json_level_t *level = bim_json->levels;
    bim->levels = level_ext;

    for(size_t i = 0; i < bim->levels_count; i++, level_ext++, level++)
    {
        level_ext->name = strdup(level->name);
        level_ext->z_level = level->z_level;

        bim_zone_t *zones = (bim_zone_t *) malloc(sizeof (bim_zone_t) * level->elements_count);
        uint16_t zone_count = 0;
        bim_transit_t *transits = (bim_transit_t *) malloc(sizeof (bim_transit_t) * level->elements_count);
        uint16_t transit_count = 0;

        for(size_t j = 0; j < level->elements_count; j++)
        {
            bim_json_element_t *element = &level->elements[j];
            if (element->sign == ROOM || element->sign == STAIR)
            {
                zones[zone_count].base = element;
                zones[zone_count].is_blocked = false;
                zones[zone_count].is_visited = false;
                zones[zone_count].potential = __FLT_MAX__;
                zones[zone_count].area = _bim_tools_get_area_element(element->polygon);
                zones[zone_count].num_of_people = element->numofpeople;
                arraylist_append(zones_list, &zones[zone_count]);
                zone_count++;
            }
            else if (element->sign == DOOR_WAY || element->sign == DOOR_WAY_OUT || element->sign == DOOR_WAY_INT)
            {
                transits[transit_count].base = element;
                transits[transit_count].is_blocked = false;
                transits[transit_count].is_visited = false;
                transits[transit_count].num_of_people = 0;
                transits[transit_count].width = 0.8; //TODO write special function
                arraylist_append(transits_list, &transits[transit_count]);
                transit_count++;
            }
        }

        if (zone_count == 0 || transit_count == 0)
            fprintf(stderr, "[func: %s() | line: %u] :: zone_count (%u) or transit_count (%u) is zero\n", __func__, __LINE__, zone_count, transit_count);
        else
        {
            zones = (bim_zone_t*)realloc(zones, sizeof (bim_zone_t) * zone_count);
            transits = (bim_transit_t*)realloc(transits, sizeof (bim_transit_t) * transit_count);
        }
        level_ext->zones = zones;
        level_ext->zone_count = zone_count;
        level_ext->transits = transits;
        level_ext->transit_count = transit_count;
    }

    arraylist_sort(zones_list, _bim_tools_element_cmp);
    arraylist_sort(transits_list, _bim_tools_element_cmp);

    return bim;
}

bim_json_object_t* bim_tools_get_json_bim (void)
{
    return bim_json;
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


bim_object_t* bim_tools_copy    (const bim_object_t* bim)
{
    return (bim_object_t *)bim;
}

void bim_tools_free (bim_object_t* bim)
{
    bim_level_t *lvl_ptr = bim->levels;
    for(size_t i = 0; i < bim->levels_count; i++, lvl_ptr++)
    {
        free(lvl_ptr->zones);
        free(lvl_ptr->transits);
    }
    free(bim->levels);
    free(bim->outside->base->name);
    free(bim->outside->base->outputs);
    free(bim->outside->base->uuid);
    free(bim->outside->base);
    free(bim->outside);

    free(bim->name);
    free(bim);

    bim_json_free(bim_json);
}

void bim_tools_set_people_to_zone(bim_zone_t* zone, float num_of_people)
{
    zone->num_of_people = num_of_people;
}

double  bim_tools_get_numofpeople(const bim_object_t *bim)
{
    double numofpeople = 0;
    for(size_t i = 0; i < bim->levels_count; i++)
    {
        for (size_t j = 0; j < bim->levels[i].zone_count; j++)
        {
            const bim_zone_t *zone = &bim->levels[i].zones[j];
            numofpeople += zone->num_of_people;
        }
    }
    return numofpeople;
}

double bim_tools_get_area_bim           (const bim_object_t* bim)
{
    double area = 0;
    for (size_t i = 0; i < bim->levels_count; i++)
    {
        for (size_t j = 0; j < bim->levels[i].zone_count; j++)
        {
            bim_zone_t zone = bim->levels[i].zones[j];
            if (zone.base->sign == ROOM || zone.base->sign == STAIR)
                area += bim->levels[i].zones[j].area;
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


ArrayList * bim_tools_get_zones_list(void)
{
    return zones_list;
}

ArrayList * bim_tools_get_transits_list(void)
{
    return transits_list;
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

double _bim_tools_get_area_element   (const bim_geometry_polygon_t *polygon)
{
    int *trianglelist = _bim_tools_triangle_polygon(polygon);

    //Вычисляем площадь по формуле S=(p(p-ab)(p-bc)(p-ca))^0.5;
    //p=(ab+bc+ca)0.5
    double areaElement = 0;
    for (size_t i = 0; i < polygon->points_count + 1; i+=3)
    {
        const bim_geometry_point_t *a = &polygon->points[trianglelist[i+0]];
        const bim_geometry_point_t *b = &polygon->points[trianglelist[i+1]];
        const bim_geometry_point_t *c = &polygon->points[trianglelist[i+2]];
        double ab = _bim_tools_length_side(a, b);
        double bc = _bim_tools_length_side(b, c);
        double ca = _bim_tools_length_side(c, a);
        double p = (ab + bc + ca) * 0.5;
        areaElement += sqrt(p * (p - ab) * (p - bc) * (p - ca));
    }

    free(trianglelist);
    return areaElement;
}

int32_t _bim_tools_element_cmp (ArrayListValue value1, ArrayListValue value2)
{
    const bim_json_element_t *e1 = value1;
    const bim_json_element_t *e2 = value2;
    if (e1->id > e2->id) return 1;
    else if (e1->id < e2->id) return -1;
    else return 0;
}

// https://userpages.umbc.edu/~rostamia/cbook/triangle.html
int* _bim_tools_triangle_polygon(const bim_geometry_polygon_t *element_polygon)
{
    struct triangulateio *in = (struct triangulateio *) malloc(sizeof (struct triangulateio));
    struct triangulateio *out = (struct triangulateio *) malloc(sizeof (struct triangulateio));
    const uint8_t points_count = element_polygon->points_count * 2;
    REAL pointlist[points_count];
    const bim_geometry_point_t *points = element_polygon->points;
    for (size_t i = 0; i < points_count; points++)
    {
        pointlist[i++] = points->x;
        pointlist[i++] = points->y;
    }

    in->pointlist = pointlist;
    in->numberofpoints = element_polygon->points_count;
    in->pointattributelist = NULL;
    in->pointmarkerlist = NULL;
    in->numberofpointattributes = 0;

    out->pointlist = NULL;
    out->pointmarkerlist = NULL;
    int *trianglelist = (int *) malloc(in->numberofpoints);
    out->trianglelist = trianglelist;  // Индексы точек треугольников против часовой стрелки

    char triswitches[2] = "zQ";
    triangulate(triswitches, in, out, NULL);

    free(in);
    free(out);

    return trianglelist;
}

double _bim_tools_length_side(const bim_geometry_point_t *p1, const bim_geometry_point_t *p2)
{
    return sqrt(pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2));
}
