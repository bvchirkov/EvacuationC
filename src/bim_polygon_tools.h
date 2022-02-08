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

#ifndef BIM_POLYGON_TOOLS_H
#define BIM_POLYGON_TOOLS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef struct
{
    double x;
    double y;
} point_t;

typedef struct
{
    point_t *p1;
    point_t *p2;
} line_t;

typedef struct
{
    size_t  numofpoints;
    point_t *points;
} polygon_t;

typedef polygon_t multiline_t;

double   geom_tools_area_polygon        (const polygon_t *const polygon);
uint8_t  geom_tools_is_point_in_polygon (const point_t   *const point,       const polygon_t *const polygon);
uint8_t  geom_tools_is_intersect_line   (const line_t    *const l1,          const line_t    *const l2);
double   geom_tools_length_side         (const point_t   *const p1,          const point_t   *const p2);
point_t* geom_tools_nearest_point       (const point_t   *const point_start, const line_t    *const line);

#endif //BIM_POLYGON_TOOLS_H
