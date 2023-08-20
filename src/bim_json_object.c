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
#include "json.h"        ///< https://github.com/rbtylee/tutorial-jsonc/blob/master/tutorial/index.md

#define streq(str1, str2) strcmp(str1, str2) == 0

const bim_json_object_t *bim_json_new(const char* filename)
{
    json_object *root = NULL;
    bim_json_object_t *bim = NULL;

    root = json_object_from_file(filename);
    if (!root) {
        LOG_ERROR("Не удалось прочитать файл. Проверьте правильность имени файла и пути: %s", filename);
        return NULL;
    }

    bim = (bim_json_object_t*)calloc(1, sizeof(bim_json_object_t));
    if (!bim) {
        LOG_ERROR("Не удалось выделить память для структуры `bim_json_object_t`");
        free(root);
        return NULL;
    }

    json_object *name_building  = NULL;
    json_object *address        = NULL;
    json_object *levels         = NULL;
    json_object_object_get_ex(root, "NameBuilding", &name_building);
    json_object_object_get_ex(root, "Address",      &address);
    json_object_object_get_ex(root, "Level",        &levels);

    bim->name        = strdup(json_object_get_string(name_building));
    bim->numoflevels = json_object_array_length(levels);
    bim->levels      = (bim_json_level_t*)calloc(bim->numoflevels, sizeof (bim_json_level_t));
    if (!bim->levels) {
        LOG_ERROR("Не выделить память для структуры `bim_json_level_t`");
        free(bim);
        free(root);
        return NULL;
    }

    json_object *st_address = NULL;
    json_object *city       = NULL;
    json_object *add_info   = NULL;
    json_object_object_get_ex(address, "StreetAddress", &st_address);
    json_object_object_get_ex(address, "City",          &city);
    json_object_object_get_ex(address, "AddInfo",       &add_info);

    bim->address = (bim_json_address_t*)calloc(1, sizeof (bim_json_address_t));
    if (!bim->address) {
        free(bim->levels);
        free(bim);
        free(root);
        return NULL;
    }

    bim->address->street_address = strdup(json_object_get_string(st_address));
    bim->address->city           = strdup(json_object_get_string(city));
    bim->address->add_info       = strdup(json_object_get_string(add_info));

    size_t bim_element_rs_id = 0;
    size_t bim_element_d_id  = 0;
    json_object *jlevel = NULL;
    for (size_t i = 0; i < bim->numoflevels; ++i)
    {
        bim_json_level_t *bim_level = &bim->levels[i];

        jlevel = json_object_array_get_idx(levels, i);
        json_object *level_name = NULL;
        json_object *level_z = NULL;
        json_object_object_get_ex(jlevel, "NameLevel", &level_name);
        json_object_object_get_ex(jlevel, "ZLevel",    &level_z);
        bim_level->name    = strdup(json_object_get_string(level_name));
        bim_level->z_level = json_object_get_double(level_z);

        json_object *elements = NULL;
        json_object_object_get_ex(jlevel, "BuildElement", &elements);
        bim_level->numofelements = json_object_array_length(elements);
        bim_level->elements      = (bim_json_element_t*)calloc(bim_level->numofelements, sizeof (bim_json_element_t));
        if (!bim_level->elements) {
            LOG_ERROR("Не выделить память для структуры `bim_json_element_t`");
            free(bim->levels);
            free(bim);
            free(root);
            return NULL;
        }

        for (size_t j = 0; j < bim_level->numofelements; ++j)
        {
            const json_object * const jelement = json_object_array_get_idx(elements, j);
            bim_json_element_t *bim_element = &(bim_level->elements[j]);

            json_object *e_name   = NULL;
            json_object *e_size_z = NULL;
            json_object *e_sign   = NULL;
            json_object *e_id     = NULL;
            json_object_object_get_ex(jelement, "Name",  &e_name);
            json_object_object_get_ex(jelement, "SizeZ", &e_size_z);
            json_object_object_get_ex(jelement, "Sign",  &e_sign);
            json_object_object_get_ex(jelement, "Id",    &e_id);
            bim_element->name        = strdup(json_object_get_string(e_name));
            bim_element->size_z      = json_object_get_double(e_size_z);
            bim_element->numofpeople = 0;
            bim_element->z_level     = bim_level->z_level;
            strcpy((void *)bim_element->uuid.x, json_object_get_string(e_id));

            const char *element_sign = json_object_get_string(e_sign);
            bim_element_sign_t b_element_sign;
            if (streq(element_sign, "Room"))
            {
                b_element_sign = ROOM;
                bim_element->id = bim_element_rs_id++;

                json_object *e_nop = NULL;
                json_object_object_get_ex(jelement, "NumPeople", &e_nop);
                bim_element->numofpeople = json_object_get_double(e_nop);
            }
            else if (streq(element_sign, "Staircase"))    { b_element_sign = STAIRCASE;   bim_element->id = bim_element_rs_id++; }
            else if (streq(element_sign, "DoorWay"))      { b_element_sign = DOOR_WAY;    bim_element->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayInt"))   { b_element_sign = DOOR_WAY_INT;bim_element->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayOut"))   { b_element_sign = DOOR_WAY_OUT;bim_element->id = bim_element_d_id++;  }
            else b_element_sign = UNDEFINDED;
            bim_element->sign = b_element_sign;

            json_object *outputs = NULL;
            json_object_object_get_ex(jelement, "Output", &outputs);
            bim_element->numofoutputs = json_object_array_length(outputs);
            bim_element->outputs = (uuid_t*)NULL;
            bim_element->outputs = (uuid_t*)calloc(bim_element->numofoutputs, sizeof(uuid_t));
            if (!bim_element->outputs) {
                LOG_ERROR("Не выделить память для списка соседних элементов");
                free(bim_level->elements);
                free(bim->levels);
                free(bim);
                free(root);
                return NULL;
            }

            for (size_t k = 0; k < bim_element->numofoutputs; ++k)
            {
                json_object *joutput = NULL;
                joutput = json_object_array_get_idx(outputs, k);
                strcpy((void *)bim_element->outputs[k].x, json_object_get_string(joutput));
            }

            json_object *xy = NULL;
            json_object_object_get_ex(jelement, "XY", &xy);
            json_object *xy0 = json_object_array_get_idx(xy, 0);

            bim_element->polygon = (polygon_t*)NULL;
            bim_element->polygon = (polygon_t*)calloc(1, sizeof (polygon_t));
            if (!bim_element->polygon) {
                LOG_ERROR("Не удалось выделить память для структуры `polygon_t`");
                free(bim_element->outputs);
                free(bim_level->elements);
                free(bim->levels);
                free(bim);
                free(root);
                return NULL;
            }

            json_object *points = NULL;
            json_object_object_get_ex(xy0, "points", &points);
            bim_element->polygon->numofpoints = json_object_array_length(points);
            bim_element->polygon->points = (point_t*)NULL;
            bim_element->polygon->points = (point_t*)calloc(bim_element->polygon->numofpoints, sizeof (point_t));
            if (!bim_element->polygon->points) {
                LOG_ERROR("Не удалось выделить память для структуры `point_t`");
                free(bim_element->polygon);
                free(bim_element->outputs);
                free(bim_level->elements);
                free(bim->levels);
                free(bim);
                free(root);
                return NULL;
            }

            for (size_t k = 0; k < bim_element->polygon->numofpoints; ++k)
            {
                json_object *jpoint = NULL;
                jpoint = json_object_array_get_idx(points, k);

                json_object *x = NULL;
                json_object *y = NULL;
                json_object_object_get_ex(jpoint, "x", &x);
                json_object_object_get_ex(jpoint, "y", &y);

                point_t *pt = NULL;
                pt = &bim_element->polygon->points[k];
                pt->x = json_object_get_double(x);
                pt->y = json_object_get_double(y);
           }


        }
    }

    json_object_put(root);

    return bim;
}

const bim_json_object_t* bim_json_copy(const bim_json_object_t *bim_object)
{
    bim_json_object_t *bim;

    uint8_t            levels_count;
    bim_json_level_t   *levels = (bim_json_level_t *) NULL;
    bim_json_address_t *address = (bim_json_address_t *) NULL;

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
    levels_count = bim_object->numoflevels;

    address = (bim_json_address_t*)malloc(sizeof(bim_json_address_t));
    if (!address) {
        free(levels);
        free(bim);
        return NULL;
    }

    address->city            = strdup(bim_object->address->city);
    address->add_info        = strdup(bim_object->address->add_info);
    address->street_address  = strdup(bim_object->address->street_address);

    bim->name        = strdup(bim_object->name);
    bim->address     = address;
    bim->levels      = levels;
    bim->numoflevels = levels_count;

    for (size_t i = 0; i < levels_count; ++i)
    {
        bim_json_level_t *level = &levels[i];
        bim_json_level_t level_original = bim_object->levels[i];
        level->name          = strdup(level_original.name);
        level->z_level       = level_original.z_level;
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

            element->id           = element_original.id;
            element->name         = strdup(element_original.name);
            element->numofpeople  = element_original.numofpeople;
            element->size_z       = element_original.size_z;
            element->z_level      = element_original.z_level;
            element->numofoutputs = element_original.numofoutputs;
            element->sign         = element_original.sign;
            strcpy((void *)element->uuid.x, element_original.uuid.x);

            polygon_t *polygon = (polygon_t*)malloc(sizeof(polygon_t));
            if (!polygon) {
                LOG_ERROR("Не удалось выделить память для структуры `polygon_t` при копировании");
                free(bim);
                free(levels);
                free(elements);
                return NULL;
            }

            {
                polygon->numofpoints = element_original.polygon->numofpoints;
                point_t *points = (point_t*)malloc(sizeof (point_t) * polygon->numofpoints);
                if (!points) {
                    LOG_ERROR("Не удалось выделить память для структуры `bim_json_element_t` при копировании");
                    free(bim);
                    free(levels);
                    free(elements);
                    free(polygon);
                    return NULL;
                }

                for (size_t k = 0; k < polygon->numofpoints; ++k)
                {
                    point_t *point = &points[k];
                    point_t point_original = element_original.polygon->points[k];
                    point->x = point_original.x;
                    point->y = point_original.y;
                }
                polygon->points = points;
            }
            element->polygon = polygon;

            uuid_t* outputs = (uuid_t *)malloc(sizeof(uuid_t) * element->numofoutputs);
            {
                for (size_t k = 0; k < element->numofoutputs; ++k)
                {
                    strcpy((void *)outputs[k].x, element_original.outputs[k].x);
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
    free((void*)bim->address->add_info);
    free((void*)bim->address->city);
    free((void*)bim->address->street_address);
    free((void*)bim->name);
    free(bim);
}

static void level_free(bim_json_level_t *level)
{
    bim_json_element_t *elements_ptr = level->elements;
    for (uint8_t j = 0; j < level->numofelements; j++, elements_ptr++)
    {
        element_free(elements_ptr);
    }
    free((void*)level->name);
    free(level->elements);
}

static void element_free(bim_json_element_t *element)
{
    free((void*)element->name);
    free(element->outputs);

    if (element->polygon)
    {
        free(element->polygon->points);
        free(element->polygon);
    }
}
