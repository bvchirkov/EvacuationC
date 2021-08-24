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

bim_json_object_t* bim_json_new(const char* filename)
{
    json_object *root;
    root = json_object_from_file(filename);
    if (!root)
    {
        LOG_ERROR("Не удалось прочитать файл. Проверьте правильность имени файла и пути: %s", filename);
        return NULL;
    }

    bim_json_object_t *bim = (bim_json_object_t*)malloc(sizeof(bim_json_object_t));
    json_object *name_building = json_object_object_get(root, "NameBuilding");
    bim->name = strdup(json_object_get_string(name_building));

    json_object *address = json_object_object_get(root, "Address");
    json_object *st_address = json_object_object_get(address, "StreetAddress");
    json_object *city = json_object_object_get(address, "City");
    json_object *add_info = json_object_object_get(address, "AddInfo");
    bim->address.street_address = strdup(json_object_get_string(st_address));
    bim->address.city = strdup(json_object_get_string(city));
    bim->address.add_info = strdup(json_object_get_string(add_info));

    json_object *levels = json_object_object_get(root, "Level");
    uint8_t levels_count = json_object_array_length(levels);
    bim_json_level_t *bim_levels = (bim_json_level_t*)malloc(sizeof (bim_json_level_t) * levels_count);
    bim->levels_count = levels_count;
    bim->levels = bim_levels;

    uint64_t bim_element_rs_id = 0;
    uint64_t bim_element_d_id = 0;
    json_object *temp1;
    for (uint8_t i = 0; i < levels_count; i++, bim_levels++)
    {
        temp1 = json_object_array_get_idx(levels, i);
        json_object *level_name = json_object_object_get(temp1, "NameLevel");
        json_object *level_z = json_object_object_get(temp1, "ZLevel");
        bim_levels->name = strdup(json_object_get_string(level_name));
        bim_levels->z_level = json_object_get_double(level_z);

        json_object *elements = json_object_object_get(temp1, "BuildElement");
        uint8_t elements_count = json_object_array_length(elements);
        bim_json_element_t *bim_elements = (bim_json_element_t*)malloc(sizeof (bim_json_element_t) * elements_count);
        bim_levels->elements = bim_elements;
        bim_levels->elements_count = elements_count;
        json_object *temp2;
        for (uint8_t j = 0; j < elements_count; j++, bim_elements++)
        {
            temp2 = json_object_array_get_idx(elements, j);
            json_object *e_name = json_object_object_get(temp2, "Name");
            json_object *e_size_z = json_object_object_get(temp2, "SizeZ");
            json_object *e_sign = json_object_object_get(temp2, "Sign");
            json_object *e_id = json_object_object_get(temp2, "Id");
            bim_elements->uuid = strndup(json_object_get_string(e_id), UUID_SIZE);
            bim_elements->name = strdup(json_object_get_string(e_name));
            bim_elements->size_z = json_object_get_double(e_size_z);

            bim_elements->numofpeople = 0;
            bim_elements->z_level = bim_levels->z_level;

            const char *element_sign = json_object_get_string(e_sign);
            bim_element_sign_t b_element_sign;
            if (streq(element_sign, "Room"))
            {
                b_element_sign = ROOM;
                bim_elements->id = bim_element_rs_id++;
                json_object *e_nop = json_object_object_get(temp2, "NumPeople");
                bim_elements->numofpeople = json_object_get_double(e_nop);
            }
            else if (streq(element_sign, "Staircase"))    { b_element_sign = STAIR;       bim_elements->id = bim_element_rs_id++; }
            else if (streq(element_sign, "DoorWay"))      { b_element_sign = DOOR_WAY;    bim_elements->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayInt"))   { b_element_sign = DOOR_WAY_INT;bim_elements->id = bim_element_d_id++;  }
            else if (streq(element_sign, "DoorWayOut"))   { b_element_sign = DOOR_WAY_OUT;bim_elements->id = bim_element_d_id++;  }
            else b_element_sign = UNDEFINDED;
            bim_elements->sign = b_element_sign;

            json_object *outputs = json_object_object_get(temp2, "Output");
            uint8_t outputs_count = json_object_array_length(outputs);
            char **bim_element_outputs = (char**)malloc(sizeof (char*) * elements_count);
            bim_elements->outputs = bim_element_outputs;
            bim_elements->outputs_count = outputs_count;
            json_object *temp3;
            for (size_t k = 0; k < outputs_count; k++)
            {
                temp3 = json_object_array_get_idx(outputs, k);
                bim_element_outputs[k] = strndup(json_object_get_string(temp3), UUID_SIZE);
            }

            json_object *xy = json_object_object_get(temp2, "XY");
            json_object *xy0 = json_object_array_get_idx(xy, 0);
            json_object *points = json_object_object_get(xy0, "points");
            uint8_t points_count = json_object_array_length(points);
            polygon_t *bim_polygon = (polygon_t*)malloc(sizeof (polygon_t));
            bim_polygon->point_count = points_count;
            point_t *bim_points = (point_t*)malloc(sizeof (point_t) * points_count);
            bim_polygon->points = bim_points;
            bim_elements->polygon = bim_polygon;
            json_object *temp4;
            for (uint8_t k = 0; k < points_count; k++, bim_points++)
            {
                temp4 = json_object_array_get_idx(points, k);
                json_object *x = json_object_object_get(temp4, "x");
                json_object *y = json_object_object_get(temp4, "y");
                bim_points->x = json_object_get_double(x);
                bim_points->y = json_object_get_double(y);
            }
        }
    }

    json_object_put(root);

    return bim;
}

bim_json_object_t* bim_json_copy (const bim_json_object_t* bim_object)
{
    // TODO
    return (bim_json_object_t *)bim_object;
}

static void _element_delete(bim_json_element_t *element)
{
    free(element->uuid);
    free(element->name);
    for (size_t i = 0; i < element->outputs_count; i++)
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

static void _level_delete(bim_json_level_t *level)
{
    bim_json_element_t *elements_ptr = level->elements;
    for (uint8_t j = 0; j < level->elements_count; j++, elements_ptr++)
    {
        _element_delete(elements_ptr);
    }
    free(level->name);
    free(level->elements);
}

void bim_json_free (bim_json_object_t* bim)
{
    bim_json_level_t *levels_ptr = bim->levels;
    for (uint8_t i = 0; i < bim->levels_count; i++, levels_ptr++)
    {
        _level_delete(levels_ptr);
    }
    free(bim->levels);
    free(bim->address.add_info);
    free(bim->address.city);
    free(bim->address.street_address);
    free(bim->name);
    free(bim);
}
