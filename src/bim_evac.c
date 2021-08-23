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

#include "bim_evac.h"

#ifndef _MAX_SPEED_REDEFINE_
static float MAX_SPEED = 100;
#endif

#ifndef _DENSITY_MIN_REDEFINE_
static float DENSITY_MIN = 0.1;
#endif

#ifndef _DENSITY_MAX_REDEFINE_
static float DENSITY_MAX = 5;
#endif

float MODELING_STEP = 0.01; //min
double _evac_time = 0;

double evac_time_s (void)
{
    return _evac_time * 60;
}

double evac_time_m (void)
{
    return _evac_time;
}

void evac_time_inc (void)
{
    _evac_time += MODELING_STEP;
}

void evac_time_reset (void)
{
    _evac_time = 0;
}

void evac_def_modeling_step(const bim_t *bim, uint64_t bim_element_count)
{
    double numofpeople = 0;
    for(size_t i = 0; i < bim->object->levels_count; i++)
    {
        for (size_t j = 0; j < bim->object->levels[i].zone_count; j++)
        {
            const bim_zone_t *zone = &bim->object->levels[i].zones[j];
            numofpeople += zone->area;
        }
    }

    double averageSize = numofpeople / bim_element_count;
    double hxy = sqrt(averageSize);             // характерный размер области, м
    MODELING_STEP = (MODELING_STEP == 0) ? hxy / MAX_SPEED * 0.1 : MODELING_STEP;      // Шаг моделирования, мин
}

/**
 * Функция скорости. Базовая зависимость, которая позволяет определить скорость людского
 * потока по его плотности
 * @brief _velocity
 * @param v0   начальная скорость потока
 * @param a    коэффициент вида пути
 * @param d    текущая плотность людского потока на участке, чел./м2
 * @param d0   допустимая плотность людского потока на участке, чел./м2
 * @return      скорость, м/мин.
 */
double _velocity(double v0, double a, double d, double d0)
{
    return v0 * (1.0 - a * log(d / d0));
}

/**
 * @brief _speed_through_door
 * @param transit_width            ширина проема, м
 * @param density_in_zone    плотность в элементе, чел/м2
 * @return                  скорость потока в проеме в зависимости от плотности, м/мин
 */
double _speed_through_door(double transit_width, double density_in_zone, double v_max)
{
    double v0 = v_max;
    double d0 = 0.65;
    double a = 0.295;
    double v0k = -1;

    if (density_in_zone > d0)
    {
        double m = (density_in_zone > 5) ? (1.25 - 0.05 * density_in_zone) : 1;
        v0k = _velocity(v0, a, density_in_zone, d0) * m;

        if (density_in_zone >= 9 && transit_width < 1.6)
        {
            v0k = 10 * (2.5 + 3.75 * transit_width) / d0;
        }
    } else
    {
        v0k = v0;
    }
    assert(v0k > 0);
    return v0k;
}

/**
 * @param density_in_zone плотность в элементе, из которого выходит поток
 * @return Скорость потока по горизонтальному пути, м/мин
 */
double _speed_in_room(double density_in_zone, double v_max)
{
    double v0 = v_max; // м/мин
    double d0 = 0.51;
    double a = 0.295;

    return density_in_zone > d0 ? _velocity(v0, a, density_in_zone, d0) : v0;
}

/**
 * @param direction направление движения (direct = 1 - вверх, = -1 - вниз)
 * @param density_in_zone  плотность в элементе
 * @return Скорость потока при движении по лестнице в зависимости от
 * плотности, м/мин
 */
double _evac_speed_on_stair(double density_in_zone, int direction)
{
    double d0 = 0, v0 = 0, a = 0;

    if (direction > 0)
    {
        d0 = 0.67;
        v0 = 50;
        a = 0.305;
    }
    else if (direction < 0)
    {
        d0 = 0.89;
        v0 = 80;
        a = 0.4;
    }

    return density_in_zone > d0 ? _velocity(v0, a, density_in_zone, d0) : v0;
}

/**
 * Метод определения скорости движения людского потока по разным зонам
 *
 * @param aReceivingElement    зона, в которую засасываются люди
 * @param aGiverElement        зона, из которой высасываются люди
 * @return Скорость людского потока в зоне
 */
double _speed_in_element(const bim_zone_t *receiving_zone,  // принимающая зона
                         const bim_zone_t *giver_zone)      // отдающая зона
{
    double density_in_giver_zone = giver_zone->base->z_level / giver_zone->area;
    // По умолчанию, используется скорость движения по горизонтальной поверхности
    double v_zone = _speed_in_room(density_in_giver_zone, MAX_SPEED);

    double dh = receiving_zone->base->z_level - giver_zone->base->z_level;   // Разница высот зон

    // Если принимающее помещение является лестницей и находится на другом уровне,
    // то скорость будет рассчитываться как по наклонной поверхности
    if (fabs(dh) > 1e-3 && receiving_zone->base->sign == STAIR)
    {
      /* Иначе определяем направление движения по лестнице
       * -1 вниз, 1 вверх
       *         ______   aGiverItem
       *        /                         => direction = -1
       *       /
       * _____/           aReceivingItem
       *      \
       *       \                          => direction = 1
       *        \______   aGiverItem
       */
        int direction = (dh > 0) ? -1 : 1;
        v_zone = _evac_speed_on_stair(density_in_giver_zone, direction);
    }

    return v_zone;
}

double _speed_at_exit( const bim_zone_t *receiving_zone,  // принимающая зона
                       const bim_zone_t *giver_zone,      // отдающая зона
                             double     transit_width)
{
    // Определение скорости на выходе из отдающего помещения
    double zone_speed = _speed_in_element(receiving_zone, giver_zone);
    double density_in_giver_element = giver_zone->num_of_people / giver_zone->area;
    double transition_speed = _speed_through_door(transit_width, density_in_giver_element, MAX_SPEED);
    double exit_speed = fmin(zone_speed, transition_speed);

    assert(exit_speed > 0 && " Speed less than 0 ");
    return exit_speed;
}

double _change_numofpeople(const bim_zone_t *giver_zone,
                                 double      transit_width,
                                 double      speed_at_exit)     // Скорость перехода в принимающую зону
{
    double densityInElement = giver_zone->num_of_people / giver_zone->area;
    // Величина людского потока, через проем шириной aWidthDoor, чел./мин
    double P = densityInElement * speed_at_exit * transit_width;
    // Зная скорость потока, можем вычислить конкретное количество человек,
    // которое может перейти в принимющую зону (путем умножения потока на шаг моделирования)
    return P * MODELING_STEP;
}

// Подсчет потенциала
//TODO Потенциал должен считаться до эвакуации из помещения или после?
//TODO Когда возникает ситуация, что потенциал принимающего больше отдающего
double _potential_element(const bim_zone_t    *receiving_zone,  // принимающая зона
                          const bim_zone_t    *giver_zone,      // отдающая зона
                          const bim_transit_t *transit)
{
    return sqrt(giver_zone->area) / _speed_at_exit(receiving_zone, giver_zone, transit->width);
}

/**
 * @brief _part_people_flow
 * @param receiving_zone    принимающее помещение
 * @param giver_zone        отдающее помещение
 * @param transit             дверь между этими помещениями
 * @return  количество людей
 */
double _part_people_flow( const bim_zone_t    *receiving_zone,  // принимающая зона
                          const bim_zone_t    *giver_zone,      // отдающая зона
                          const bim_transit_t *transit)
{
    double area_giver_zone = giver_zone->area;
    double people_in_giver_zone = giver_zone->num_of_people;
    double density_in_giver_zone= people_in_giver_zone / area_giver_zone;
    double density_min_giver_zone = DENSITY_MIN > 0 ? DENSITY_MIN : 0.5 / area_giver_zone;

    // Ширина перехода между зонами зависит от количества человек,
    // которое осталось в помещении. Если там слишком мало людей,
    // то они переходя все сразу, чтоб не дробить их
    double door_width = transit->width; //(densityInElement > densityMin) ? aDoor.VCn().getWidth() : std::sqrt(areaElement);
    double speed_at_exit = _speed_at_exit(receiving_zone, giver_zone, door_width);

    // Кол. людей, которые могут покинуть помещение
    double part_of_people_flow = (density_in_giver_zone > density_min_giver_zone)
            ? _change_numofpeople(giver_zone, door_width, speed_at_exit)
            : people_in_giver_zone;

    // Т.к. зона вне здания принята безразмерной,
    // в нее может войти максимально возможное количество человек
    // Все другие зоны могут принять ограниченное количество человек.
    // Т.о. нужно проверить может ли принимающая зона вместить еще людей.
    // capacity_reciving_zone - количество людей, которое еще может
    // вместиться до достижения максимальной плотности
    // => если может вместить больше, чем может выйти, то вмещает всех вышедших,
    // иначе вмещает только возможное количество.
    double max_numofpeople = DENSITY_MAX * receiving_zone->area;
    double capacity_reciving_zone = max_numofpeople - receiving_zone->num_of_people;
    // Такая ситуация возникает при плотности в принимающем помещении более Dmax чел./м2
    // Фактически capacity_reciving_zone < 0 означает, что помещение не может принять людей
    if (capacity_reciving_zone < 0)
    {
        return 0;
    }
    return (capacity_reciving_zone > part_of_people_flow) ? part_of_people_flow : capacity_reciving_zone;
}

void evac_bim_ext_init(const ArrayList *zones, const ArrayList *transits)
{
    for (size_t i = 0; i < zones->length; i++)
    {
        bim_zone_t *zone = zones->data[i];
        zone->area = 0;
        zone->is_visited = false;
        zone->is_blocked = false;
        zone->num_of_people = zone->base->numofpeople;
        zone->potential = __DBL_MAX__;
    }

    for (size_t i = 0; i < transits->length; i++)
    {
        bim_transit_t *transit = transits->data[i];
        transit->is_visited = false;
        transit->width = 0.8;
        transit->is_blocked = false;
    }
}

void _reset_zones(const ArrayList *zones)
{
    for (size_t i = 0; i < zones->length; i++)
    {
        bim_zone_t *zone = zones->data[i];
        zone->is_visited = false;
        zone->potential = (zone->base->sign == OUTSIDE) ? 0 : __DBL_MAX__;
    }
}

void _reset_transits(const ArrayList *transits)
{
    for (size_t i = 0; i < transits->length; i++)
    {
        bim_transit_t *transit = transits->data[i];
        transit->is_visited = false;
        transit->num_of_people = 0;
    }
}

int _elementideq_callback(const ArrayListValue value1, const ArrayListValue value2)
{
    return ((bim_zone_t *)value1)->base->id == ((bim_zone_t *)value2)->base->id;
}

int _potentialcmp_callback (const ArrayListValue value1, const ArrayListValue value2)
{
    return ((bim_zone_t *)value1)->potential < ((bim_zone_t *)value2)->potential;
}

uint64_t _transit_blocked_count(const ArrayList *transits)
{
    uint64_t transit_blocked_count = 0;
    for (size_t i = 0; i < transits->length; i++)
    {
        bim_transit_t *transit = transits->data[i];
        if (transit->is_blocked) transit_blocked_count++;
    }

    return transit_blocked_count;
}

void evac_moving_step(const bim_graph *graph, const ArrayList *zones, const ArrayList *transits)
{
    _reset_zones(zones);
    _reset_transits(transits);

    size_t unprocessed_zones_count    = zones->length;
    ArrayList *zones_to_process = arraylist_new(unprocessed_zones_count);

    uint64_t outside_id = graph->node_count - 1;
    bim_node* ptr = graph->head[outside_id];
    bim_zone_t *outside = zones->data[outside_id];
    bim_zone_t *receiving_zone = outside;

    while (1)
    {
        for (size_t i = 0; i < receiving_zone->base->outputs_count && ptr != NULL; i++, ptr = ptr->next)
        {
            bim_transit_t *transit = transits->data[ptr->eid];
            if (transit->is_visited || transit->is_blocked) continue;

            bim_zone_t *giver_zone  = zones->data[ptr->dest];

            receiving_zone->potential += _potential_element(receiving_zone, giver_zone, transit);
            double moved_people = _part_people_flow(receiving_zone, giver_zone, transit);
            receiving_zone->num_of_people += moved_people;
            giver_zone->num_of_people -= moved_people;
            transit->num_of_people = moved_people;

            giver_zone->is_visited = true;
            transit->is_visited = true;

            if (giver_zone->base->outputs_count > 1 && !giver_zone->is_blocked
                && arraylist_index_of(zones_to_process, _elementideq_callback, giver_zone) < 0)
            {
                arraylist_append(zones_to_process, giver_zone);
            }
        }

        arraylist_sort(zones_to_process, _potentialcmp_callback);

        if (zones_to_process->length > 0)
        {
            receiving_zone = zones_to_process->data[0];
            ptr = graph->head[receiving_zone->base->id];
            arraylist_remove(zones_to_process, 0);
        }

        if (unprocessed_zones_count == 0) break;
        --unprocessed_zones_count;
    }

    arraylist_free(zones_to_process);
}
