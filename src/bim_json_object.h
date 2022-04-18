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
\brief Заголовочный файл с описанием полей входного json-файла
\author bvchirkov
\version 0.1
\date Август 2021 года

Данный файл содержит в себе определения основных
структур, используемых для представления здания в программе
*/

#ifndef BIM_JSON_OBJECT_H
#define BIM_JSON_OBJECT_H

#include <stdint.h>
#include "bim_polygon_tools.h"
#include "logger.h"
#include "bim_uuid.h"

/// Набор возможных типов элеметов здания:
/// ROOM, STAIR, DOOR_WAY, DOOR_WAY_INT, DOOR_WAY_OUT, OUTSIDE
typedef enum
{
    ROOM,           ///< Указывает, что элемент здания является помещением/комнатой
    STAIRCASE,      ///< Указывает, что элемент здания является лестницей
    DOOR_WAY,       ///< Указывает, что элемент здания является проемом (без дверного полотна)
    DOOR_WAY_INT,   ///< Указывает, что элемент здания является дверью, которая соединяет
                    ///< два элемента: ROOM и ROOM или ROOM и STAIR
    DOOR_WAY_OUT,   ///< Указывает, что элемент здания является эвакуационным выходом
    OUTSIDE,        ///< Указывает, что элемент является зоной вне здания
    UNDEFINDED      ///< Указывает, что тип элемента не определен
} bim_element_sign_t;

/// Структура, описывающая элемент
typedef struct
{
    uuid_t              uuid;           ///< [JSON] UUID идентификатор элемента
    const char          *name;          ///< [JSON] Название элемента
    polygon_t           *polygon;       ///< [JSON] Полигон элемента
    uuid_t              *outputs;       ///< [JSON] Массив UUID элементов, которые являются соседними
    size_t              id;             ///< Внутренний номер элемента (генерируется)
    size_t              numofpeople;    ///< [JSON] Количество людей в элементе
    size_t              numofoutputs;   ///< Количество связанных с текущим элементов
    double              size_z;         ///< [JSON] Высота элемента
    double              z_level;        ///< Уровень, на котором находится элемент
    bim_element_sign_t  sign;           ///< [JSON] Тип элемента
} bim_json_element_t;

/// Структура поля, описывающего географическое положение объекта
typedef struct
{
    const char *city;             ///< [JSON] Название города
    const char *street_address;   ///< [JSON] Название улицы
    const char *add_info;         ///< [JSON] Дополнительная информация о местоположении объекта
} bim_json_address_t;

/// Структура, описывающая этаж
typedef struct
{
    const char          *name;          ///< [JSON] Название этажа
    bim_json_element_t  *elements;      ///< [JSON] Массив элементов, которые принадлежат этажу
    double              z_level;        ///< [JSON] Высота этажа над нулевой отметкой
    size_t              numofelements;  ///< Количство элементов на этаже
} bim_json_level_t;

/// Структура, описывающая здание
typedef struct
{
    bim_json_address_t  *address;       ///< [JSON] Информация о местоположении объекта
    const char          *name;          ///< [JSON] Название здания
    bim_json_level_t    *levels;        ///< [JSON] Массив уровней здания
    size_t              numoflevels;    ///< Количество уровней в здании
} bim_json_object_t;

/*!
Создает новый объект типа bim_object_t

\param[in] filename Имя файла
\returns Указатель на объект типа bim_object_t
*/
const bim_json_object_t*  bim_json_new        (const char *filename);

/*!
Копирует объект типа bim_object_t и возвращает указатель на новый объект

\param[in] bim_object Объект для копирования
\returns Указатель на объект типа bim_object_t
*/
const bim_json_object_t*  bim_json_copy       (const bim_json_object_t *bim_object);

/*!
Удаляет объект типа bim_object_t и освобождает память

\param[in] bim_object Объект типа bim_object_t
*/
void           bim_json_free     (bim_json_object_t *bim_object);

#endif //BIM_JSON_OBJECT_H
