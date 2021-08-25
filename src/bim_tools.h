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

/*!
\file
\brief Заголовочный файл с описанием функций для работы с моделью здания
\author bvchirkov
\version 0.1
\date Август 2021 года

Данный файл содержит в себе определения структур, расширяющих основные структуры
bim_level_element_t, и функции для работы с моделью здания.
*/

#ifndef BIM_TOOLS_H
#define BIM_TOOLS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "bim_json_object.h"
#include "bim_polygon_tools.h"
#include "arraylist/arraylist.h" ///< https://github.com/fragglet/c-algorithms

/// Структура, расширяющая элемент DOOR_*
typedef struct
{
    bim_json_element_t   *base;
    bool            is_visited;     ///< Признак посещения элемента
    bool            is_blocked;     ///< Признак недоступности элемента для движения
    float           width;          ///< Ширина проема/двери
    float           num_of_people;  ///< Количество людей, которые прошли через элемент
} bim_transit_t;

/// Структура, расширяющая элемент типа ROOM и STAIR
typedef struct
{
    bim_json_element_t   *base;
    bool            is_visited;     ///< Признак посещения элемента
    bool            is_blocked;     ///< Признак недоступности элемента для движения
    float           num_of_people;  ///< Количество людей в элементе
    float           potential;      ///< Время достижения безопасной зоны
    float           area;           ///< Площадь элемента
} bim_zone_t;

/// Структура, описывающая этаж
typedef struct
{
    char            *name;          ///< [JSON] Название этажа
    float           z_level;        ///< [JSON] Высота этажа над нулевой отметкой
    uint16_t        zone_count;     ///< Количство зон на этаже
    uint16_t        transit_count;  ///< Количство переходов на этаже
    bim_zone_t      *zones;         ///< Массив зон, которые принадлежат этажу
    bim_transit_t   *transits;      ///< Массив переходов, которые принадлежат этажу
} bim_level_t;

/// Структура, описывающая здание
typedef struct
{
    char        *name;          ///< [JSON] Название здания
    uint8_t     levels_count;   ///< Количество уровней в здании
    bim_level_t *levels;        ///< [JSON] Массив уровней здания
    bim_zone_t  *outside;       ///< Зона вне здания
} bim_object_t;

typedef struct
{
    bim_json_object_t   *json;      ///< Ссылка на структуру, полученную из json файла
    bim_object_t        *object;    ///< Сслыка на расширенную структуру здания
    ArrayList           *zones;     ///< Список зон объекта
    ArrayList           *transits;  ///< Список переходов объекта
} bim_t;

bim_t *bim_tools_new    (const char *file);
bim_t *bim_tools_copy   (const bim_t *bim);
void   bim_tools_free   (bim_t* bim);

bim_json_object_t* bim_tools_get_json_bim (void);

// Устанавливает в помещение заданное количество людей
void    bim_tools_set_people_to_zone (bim_zone_t* element, float num_of_people);

// Подсчитывает количество людей в здании по расширенной структуре
double  bim_tools_get_numofpeople(const bim_t *bim);

//Подсчитывает суммарную площадь элементов всего здания
double  bim_tools_get_area_bim   (const bim_t *bim);

void bim_tools_print_element(const bim_zone_t *zone);

#endif //BIM_TOOLS_H
