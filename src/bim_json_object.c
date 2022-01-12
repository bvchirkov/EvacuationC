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

#include "bim_json_object.h"
#include "json-c/json.h"        ///< https://github.com/rbtylee/tutorial-jsonc/blob/master/tutorial/index.md

#define streq(str1, str2) strcmp(str1, str2) == 0

const bim_json_object_t *bim_json_new(const char* filename)
{
    json_object *root;
    root = json_object_from_file(filename);
    if (!root) {
        LOG_ERROR("Не удалось прочитать файл. Проверьте правильность имени файла и пути: %s", filename);
        return NULL;
    }

    bim_json_object_t *bim = (bim_json_object_t*)malloc(sizeof(bim_json_object_t));
    if (!bim) {
        LOG_ERROR("Не выделить память для структуры `bim_json_object_t`");
        free(root);
        return NULL;
    }
    json_object *name_building = json_object_object_get(root, "NameBuilding");
    bim->name = strdup(json_object_get_string(name_building));

    json_object *address    = json_object_object_get(root,    "Address");
    json_object *st_address = json_object_object_get(address, "StreetAddress");
    json_object *city       = json_object_object_get(address, "City");
    json_object *add_info   = json_object_object_get(address, "AddInfo");
    bim->address.street_address = strdup(json_object_get_string(st_address));
    bim->address.city           = strdup(json_object_get_string(city));
    bim->address.add_info       = strdup(json_object_get_string(add_info));

    json_object *levels = json_object_object_get(root, "Level");
    uint8_t levels_count = json_object_array_length(levels);
    bim_json_level_t *bim_levels = (bim_json_level_t*)malloc(sizeof (bim_json_level_t) * levels_count);
    if (!bim_levels) {
        LOG_ERROR("Не выделить память для структуры `bim_json_level_t`");
        free(root);
        free(bim);
        return NULL;
    }
    bim->numoflevels = levels_count;
    bim->levels = bim_levels;

    uint64_t bim_element_rs_id = 0;
    uint64_t bim_element_d_id = 0;
    json_object *jlevel;
    for (uint8_t i = 0; i < levels_count; i++)
    {
        bim_json_level_t *bim_level = &bim_levels[i];

        jlevel = json_object_array_get_idx(levels, i);
        json_object *level_name = json_object_object_get(jlevel, "NameLevel");
        json_object *level_z    = json_object_object_get(jlevel, "ZLevel");
        bim_level->name = strdup(json_object_get_string(level_name));
        bim_level->z_level = json_object_get_double(level_z);

        json_object *elements = json_object_object_get(jlevel, "BuildElement");
        uint8_t elements_count = json_object_array_length(elements);
        bim_json_element_t *bim_elements = (bim_json_element_t*)malloc(sizeof (bim_json_element_t) * elements_count);
        if (!bim_elements) {
            LOG_ERROR("Не выделить память для структуры `bim_json_element_t`");
            free(root);
            free(bim);
            free(bim_levels);
            return NULL;
        }
        bim_level->elements = bim_elements;
        bim_level->numofelements = elements_count;
        json_object *jelement;
        for (uint8_t j = 0; j < elements_count; j++)
        {
            bim_json_element_t *bim_element = &bim_elements[j];
            jelement = json_object_array_get_idx(elements, j);
            json_object *e_name   = json_object_object_get(jelement, "Name");
            json_object *e_size_z = json_object_object_get(jelement, "SizeZ");
            json_object *e_sign   = json_object_object_get(jelement, "Sign");
            json_object *e_id     = json_object_object_get(jelement, "Id");
            bim_element->uuid   = strndup(json_object_get_string(e_id), UUID_SIZE);
            bim_element->name   = strdup(json_object_get_string(e_name));
            bim_element->size_z = json_object_get_double(e_size_z);
            bim_element->numofpeople = 0;
            bim_element->z_level     = bim_level->z_level;

            const char *element_sign = json_object_get_string(e_sign);
            bim_element_sign_t b_element_sign;
            if (streq(element_sign, "Room"))
            {
                b_element_sign = ROOM;
                bim_element->id = bim_element_rs_id++;
                json_object *e_nop = json_object_object_get(jelement, "NumPeople");
                bim_element->numofpeople = json_object_get_double(e_nop);
            }
            else if (streq(element_sign, "Staircase"))    { b_element_sign = STAIRCASE;   bim_element->id = bim_element_rs_id++; }
            else if (streq(element_sign, "DoorWay"))      { b_element_sign = DOOR_WAY;    bim_element->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayInt"))   { b_element_sign = DOOR_WAY_INT;bim_element->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayOut"))   { b_element_sign = DOOR_WAY_OUT;bim_element->id = bim_element_d_id++;  }
            else b_element_sign = UNDEFINDED;
            bim_element->sign = b_element_sign;

            json_object *outputs = json_object_object_get(jelement, "Output");
            uint8_t outputs_count = json_object_array_length(outputs);
            char **bim_element_outputs = (char**)malloc(sizeof (char*) * elements_count);
            if (!bim_element_outputs) {
                LOG_ERROR("Не выделить память для структуры `bim_json_element_t`");
                free(root);
                free(bim);
                free(bim_levels);
                free(bim_elements);
                return NULL;
            }
            bim_element->outputs = bim_element_outputs;
            bim_element->numofoutputs = outputs_count;
            json_object *joutput;
            for (size_t k = 0; k < outputs_count; k++)
            {
                joutput = json_object_array_get_idx(outputs, k);
                bim_element_outputs[k] = strndup(json_object_get_string(joutput), UUID_SIZE);
            }

            json_object *xy = json_object_object_get(jelement, "XY");
            json_object *xy0 = json_object_array_get_idx(xy, 0);
            json_object *points = json_object_object_get(xy0, "points");
            uint8_t points_count = json_object_array_length(points);
            polygon_t *bim_polygon = (polygon_t*)malloc(sizeof (polygon_t));
            if (!bim_polygon) {
                LOG_ERROR("Не выделить память для структуры `bim_json_element_t`");
                free(root);
                free(bim);
                free(bim_levels);
                free(bim_elements);
                free(bim_element_outputs);
                return NULL;
            }
            bim_polygon->point_count = points_count;
            point_t *bim_points = (point_t*)malloc(sizeof (point_t) * points_count);
            if (!bim_points) {
                LOG_ERROR("Не выделить память для структуры `bim_json_element_t`");
                free(root);
                free(bim);
                free(bim_levels);
                free(bim_elements);
                free(bim_element_outputs);
                free(bim_polygon);
                return NULL;
            }
            bim_polygon->points = bim_points;
            bim_element->polygon = bim_polygon;
            json_object *jpoint;
            for (uint8_t k = 0; k < points_count; k++, bim_points++)
            {
                jpoint = json_object_array_get_idx(points, k);
                json_object *x = json_object_object_get(jpoint, "x");
                json_object *y = json_object_object_get(jpoint, "y");
                bim_points->x = json_object_get_double(x);
                bim_points->y = json_object_get_double(y);
            }
        }
    }

    json_object_put(root);

    return bim;
}

const bim_json_object_t *bim_json_copy(const bim_json_object_t* bim_object)
{
    bim_json_object_t *bim;

    char *name;
    uint8_t levels_count;
    bim_json_level_t *levels;
    bim_json_address_t address;

    bim = (bim_json_object_t*)malloc(sizeof(bim_json_object_t));
    if (!bim) {
        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t` при копировании");
        return NULL;
    }
    levels = (bim_json_level_t*)malloc(sizeof(bim_json_level_t));
    if (!levels) {
        LOG_ERROR("Не удалось выделить память для структуры `bim_json_level_t` при копировании");
        free(bim);
        return NULL;
    }
    name = strdup(bim_object->name);
    levels_count = bim_object->numoflevels;

    address.add_info = strdup(bim_object->address.add_info);
    address.city = strdup(bim_object->address.city);
    address.street_address = strdup(bim_object->address.street_address);

    bim->name = name;
    bim->levels = levels;
    bim->numoflevels = levels_count;
    bim->address = address;

    for (size_t i = 0; i < levels_count; ++i)
    {
        bim_json_level_t *level = &levels[i];
        bim_json_level_t level_original = bim_object->levels[i];
        level->name = strdup(level_original.name);
        level->z_level = level_original.z_level;
        level->numofelements = level_original.numofelements;

        bim_json_element_t *elements = (bim_json_element_t*)malloc(sizeof(bim_json_element_t) * level->numofelements);
        if (!elements) {
            LOG_ERROR("Не удалось выделить память для структуры `bim_json_element_t` при копировании");
            free(bim);
            free(levels);
            return NULL;
        }

        level->elements = elements;
        for (size_t j = 0; j < level->numofelements; ++j)
        {
            bim_json_element_t *element = &elements[j];
            bim_json_element_t element_original = level_original.elements[j];

            element->id = element_original.id;
            element->name = strdup(element_original.name);
            element->numofpeople = element_original.numofpeople;
            element->uuid = strdup(element_original.uuid);
            element->size_z = element_original.size_z;
            element->z_level = element_original.z_level;
            element->numofoutputs = element_original.numofoutputs;
            element->sign = element_original.sign;

            polygon_t *polygon = (polygon_t*)malloc(sizeof(polygon_t));
            if (!polygon) {
                LOG_ERROR("Не удалось выделить память для структуры `polygon_t` при копировании");
                free(bim);
                free(levels);
                free(elements);
                return NULL;
            }

            {
                polygon->point_count = element_original.polygon->point_count;
                point_t *points = (point_t*)malloc(sizeof (point_t) * polygon->point_count);
                if (!points) {
                    LOG_ERROR("Не удалось выделить память для структуры `bim_json_element_t` при копировании");
                    free(bim);
                    free(levels);
                    free(elements);
                    free(polygon);
                    return NULL;
                }

                for (size_t k = 0; k < polygon->point_count; ++k)
                {
                    point_t *point = &points[k];
                    point_t point_original = element_original.polygon->points[k];
                    point->x = point_original.x;
                    point->y = point_original.y;
                }
                polygon->points = points;
            }
            element->polygon = polygon;

            char **outputs = (char **)malloc(sizeof(char*) * element->numofoutputs);
            {
                for (size_t k = 0; k < element->numofoutputs; ++k)
                {
                    outputs[k] = strdup(element_original.outputs[k]);
                }
            }
            element->outputs = outputs;
        }
    }

    return bim;
}

static void element_free(bim_json_element_t *element);
static void level_free(bim_json_level_t *level);

void bim_json_free(bim_json_object_t* bim)
{
    bim_json_level_t *levels_ptr = bim->levels;
    for (uint8_t i = 0; i < bim->numoflevels; i++, levels_ptr++)
    {
        level_free(levels_ptr);
    }
    free(bim->levels);
    free(bim->address.add_info);
    free(bim->address.city);
    free(bim->address.street_address);
    free(bim->name);
    free(bim);
}

static void level_free(bim_json_level_t *level)
{
    bim_json_element_t *elements_ptr = level->elements;
    for (uint8_t j = 0; j < level->numofelements; j++, elements_ptr++)
    {
        element_free(elements_ptr);
    }
    free(level->name);
    free(level->elements);
}

static void element_free(bim_json_element_t *element)
{
    free(element->uuid);
    free(element->name);
    for (size_t i = 0; i < element->numofoutputs; i++)
    {
        free(element->outputs[i]);
    }
    free(element->outputs);

    if (element->polygon)
    {
        free(element->polygon->points);
        free(element->polygon);
    }
}
