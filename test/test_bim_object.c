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

#include <string.h>
#include <assert.h>
#include "bim_json_object.h"

#define TEST_CASE       void
#define SUCCESS         "ОК"
#define FAIL            "FAIL"
#define __LOG_INFO__(str)   fprintf(stdout, "[%-5s()] :: %s\n", __func__, str)

TEST_CASE one_zone_one_exit(void)
{
    const char * filename = ROOT_PATH"/one_zone_one_exit.json";
    __LOG_INFO__(filename);
    bim_json_object_t *bim_json = bim_json_new(filename);
    assert(bim_json->levels_count == 1);

    assert(bim_json->levels[0].elements_count == 2);
    assert(bim_json->levels[0].z_level == 0.0);

    bim_json_element_t element1 = bim_json->levels[0].elements[0];
    bim_json_element_t element2 = bim_json->levels[0].elements[1];

    if (element1.sign != DOOR_WAY_OUT)
    {
        element1 = bim_json->levels[0].elements[1];
        element2 = bim_json->levels[0].elements[0];
    }

    // transit
    assert(element1.outputs_count == 1);
    assert(element1.size_z == 2);
    assert(element1.z_level == 0);
    assert(element1.id == 0);
    assert(element1.numofpeople == 0);
    assert(element1.polygon->point_count == 5);

    // zone
    assert(element2.outputs_count == 1);
    assert(element2.size_z == 3);
    assert(element2.z_level == 0);
    assert(element2.numofpeople == 15);
    assert(element2.polygon->point_count == 5);

    // adjacency
    assert(strcmp(element1.uuid, element2.outputs[0]) == 0);
    assert(strcmp(element2.uuid, element1.outputs[0]) == 0);

    __LOG_INFO__(SUCCESS);
}

TEST_CASE three_zone_three_transit(void)
{
    const char * filename = ROOT_PATH"/three_zone_three_transit.json";
    __LOG_INFO__(filename);
    bim_json_object_t *bim_json = bim_json_new(filename);
    assert(bim_json->levels_count == 1);

    bim_json_level_t level = bim_json->levels[0];
    assert(level.elements_count == 6);
    assert(level.z_level == 0.0);

    for (size_t i = 0; i < level.elements_count; i++)
    {
        bim_json_element_t element = level.elements[i];
        if (element.sign == ROOM)
        {
            if (strcmp(element.name, "Room_3 (00 : 70250)") == 0)
                assert(element.outputs_count == 1);
            else
                assert(element.outputs_count == 2);

            assert(element.size_z == 3);
            assert(element.z_level == 0);
            assert(element.polygon->point_count == 5);
        }
        else if (element.sign == DOOR_WAY || element.sign == DOOR_WAY_INT)
        {
            assert(element.outputs_count == 2);
            assert(element.size_z == 2);
            assert(element.z_level == 0);
            assert(element.numofpeople == 0);
            assert(element.polygon->point_count == 5);
        }
    }

    __LOG_INFO__(SUCCESS);
}

TEST_CASE two_levels(void)
{
    const char * filename = ROOT_PATH"/two_levels.json";
    __LOG_INFO__(filename);
    bim_json_object_t *bim_json = bim_json_new(filename);
    assert(bim_json->levels_count == 2);

    bim_json_level_t level1 = bim_json->levels[0];
    assert(level1.elements_count == 8 + 1); // trsnsit between levels
    assert(level1.z_level == 0.0);

    bim_json_level_t level2 = bim_json->levels[1];
    assert(level2.elements_count == 7);
    assert(level2.z_level == 3.0);

    for (size_t j = 0; j < bim_json->levels_count; j++)
    {
        for (size_t i = 0; i < bim_json->levels[j].elements_count; i++)
        {
            bim_json_element_t element = bim_json->levels[j].elements[i];
            if (element.sign == STAIR)
            {
                assert(element.outputs_count == 2);
            }
        }
    }

    __LOG_INFO__(SUCCESS);
}

int main (void)
{
    printf("====== TESTS STARTS ======\n");

    one_zone_one_exit();
    three_zone_three_transit();
    two_levels();

    printf("====== TESTS END ======\n");
}
