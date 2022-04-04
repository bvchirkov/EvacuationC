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

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "arraylist.h"          ///< https://github.com/fragglet/c-algorithms
#include "bim_json_object.h"

/// Структура, расширяющая элемент DOOR_*
typedef struct
{
    uuid_t      uuid;           ///< UUID идентификатор элемента
    size_t      id;             ///< Внутренний номер элемента
    char*       name;           ///< Название элемента
    uuid_t      *outputs;       ///< Массив UUID элементов, которые являются соседними
    polygon_t   *polygon;       ///< Полигон элемента
    double      size_z;         ///< Высота элемента
    double      z_level;        ///< Уровень, на котором находится элемент
    double      width;          ///< Ширина проема/двери
    double      nop_proceeding; ///< Количество людей, которые прошли через элемент
    uint8_t     sign;           ///< Тип элемента
    uint8_t     numofoutputs;   ///< Количество связанных с текущим элементов
    bool        is_visited;     ///< Признак посещения элемента
    bool        is_blocked;     ///< Признак недоступности элемента для движения
} bim_transit_t;

/// Структура, расширяющая элемент типа ROOM и STAIR
typedef struct
{
    uuid_t      uuid;           ///< UUID идентификатор элемента
    size_t      id;             ///< Внутренний номер элемента
    char*       name;           ///< Название элемента
    polygon_t   *polygon;       ///< Полигон элемента
    uuid_t      *outputs;       ///< Массив UUID элементов, которые являются соседними
    double      size_z;         ///< Высота элемента
    double      z_level;        ///< Уровень, на котором находится элемент
    double      numofpeople;    ///< Количество людей в элементе
    double      potential;      ///< Время достижения безопасной зоны
    double      area;           ///< Площадь элемента
    uint8_t     hazard_level;   ///< Уровень опасности, % (0, 10, 20, ..., 90, 100)
    uint8_t     sign;           ///< Тип элемента
    uint8_t     numofoutputs;   ///< Количество связанных с текущим элементов
    bool        is_visited;     ///< Признак посещения элемента
    bool        is_blocked;     ///< Признак недоступности элемента для движения
    bool        is_safe;        ///< Признак безопасности зоны, т.е. в эту зону возможна эвакуация
} bim_zone_t;

/// Структура, описывающая этаж
typedef struct
{
    bim_zone_t      *zones;         ///< Массив зон, которые принадлежат этажу
    bim_transit_t   *transits;      ///< Массив переходов, которые принадлежат этажу
    char*           name;           ///< Название этажа
    double          z_level;        ///< Высота этажа над нулевой отметкой
    uint16_t        numofzones;     ///< Количство зон на этаже
    uint16_t        numoftransits;  ///< Количство переходов на этаже
} bim_level_t;

/// Структура, описывающая здание
typedef struct
{
    bim_level_t     *levels;        ///< Массив уровней здания
    char*           name;           ///< Название здания

    ArrayList       *zones;         ///< Список зон объекта
    ArrayList       *transits;      ///< Список переходов объекта
    uint8_t         numoflevels;    ///< Количество уровней в здании
} bim_t;

bim_t* bim_tools_new    (const bim_json_object_t * const bim_json);
bim_t* bim_tools_copy   (const bim_t *const bim);
void   bim_tools_free   (bim_t* bim);

// Устанавливает в помещение заданное количество людей
void    bim_tools_set_people_to_zone (bim_zone_t* element, float num_of_people);

// Подсчитывает количество людей в здании по расширенной структуре
double  bim_tools_get_numofpeople(const bim_t * const bim);

//Подсчитывает суммарную площадь элементов всего здания
double  bim_tools_get_area_bim   (const bim_t *const bim);

void bim_tools_print_element(const bim_zone_t *zone);

#endif //BIM_TOOLS_H
