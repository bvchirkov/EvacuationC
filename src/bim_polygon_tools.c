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

#include "bim_polygon_tools.h"
#include "triangle/triangle.h"

// https://userpages.umbc.edu/~rostamia/cbook/triangle.html
int* geom_tools_triangle_polygon(const polygon_t *polygon)
{
    struct triangulateio *in = (struct triangulateio *) malloc(sizeof (struct triangulateio));
    struct triangulateio *out = (struct triangulateio *) malloc(sizeof (struct triangulateio));
    const uint8_t point_count = polygon->point_count * 2;
    REAL pointlist[point_count];
    const point_t *points = polygon->points;
    for (size_t i = 0; i < point_count; points++)
    {
        pointlist[i++] = points->x;
        pointlist[i++] = points->y;
    }

    in->pointlist = pointlist;
    in->numberofpoints = polygon->point_count;
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

double geom_tools_length_side(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

double geom_tools_area_polygon(const polygon_t *polygon)
{
    int *trianglelist = geom_tools_triangle_polygon(polygon);

    //Вычисляем площадь по формуле S=(p(p-ab)(p-bc)(p-ca))^0.5;
    //p=(ab+bc+ca)0.5
    double areaElement = 0;
    for (size_t i = 0; i < polygon->point_count + 1; i+=3)
    {
        const point_t *a = &polygon->points[trianglelist[i+0]];
        const point_t *b = &polygon->points[trianglelist[i+1]];
        const point_t *c = &polygon->points[trianglelist[i+2]];
        double ab = geom_tools_length_side(a->x, a->y, b->x, b->y);
        double bc = geom_tools_length_side(b->x, b->y, c->x, c->y);
        double ca = geom_tools_length_side(c->x, c->y, a->x, a->y);
        double p = (ab + bc + ca) * 0.5;
        areaElement += sqrt(p * (p - ab) * (p - bc) * (p - ca));
    }

    free(trianglelist);
    return areaElement;
}
