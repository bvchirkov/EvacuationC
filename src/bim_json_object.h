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

#ifndef BIM_OBJECT_H
#define BIM_OBJECT_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "bim_polygon_tools.h"

/// Количество символов в UUID
#define UUID_SIZE 36

/// Набор возможных типов элеметов здания:
/// ROOM, STAIR, DOOR_WAY, DOOR_WAY_INT, DOOR_WAY_OUT, OUTSIDE
typedef enum
{
    ROOM,           ///< Указывает, что элемент здания является помещением/комнатой
    STAIR,          ///< Указывает, что элемент здания является лестницей
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
    uint64_t                id;             ///< Внутренний номер элемента (генерируется)
    char                    *uuid;          ///< [JSON] UUID идентификатор элемента
    char                    *name;          ///< [JSON] Название элемента
    float                   size_z;         ///< [JSON] Высота элемента
    float                   z_level;        ///< Уровень, на котором находится элемент
    uint16_t                numofpeople;    ///< [JSON] Количество людей в элементе
    bim_element_sign_t      sign;           ///< [JSON] Тип элемента
    polygon_t               *polygon;       ///< [JSON] Полигон элемента
    uint8_t                 outputs_count;  ///< Количество связанных с текущим элементов
    char                    **outputs;      ///< [JSON] Массив UUID элементов, которые являются соседними
} bim_json_element_t;

/// Структура поля, описывающего географическое положение объекта
typedef struct
{
    char *city;             ///< [JSON] Название города
    char *street_address;   ///< [JSON] Название улицы
    char *add_info;         ///< [JSON] Дополнительная информация о местоположении объекта
} bim_json_address_t;

/// Структура, описывающая этаж
typedef struct
{
    char                *name;          ///< [JSON] Название этажа
    float               z_level;        ///< [JSON] Высота этажа над нулевой отметкой
    uint16_t            elements_count; ///< Количство элементов на этаже
    bim_json_element_t  *elements;      ///< [JSON] Массив элементов, которые принадлежат этажу
} bim_json_level_t;

/// Структура, описывающая здание
typedef struct
{
    char                *name;          ///< [JSON] Название здания
    uint8_t             levels_count;   ///< Количество уровней в здании
    bim_json_level_t    *levels;        ///< [JSON] Массив уровней здания
    bim_json_address_t  address;        ///< [JSON] Информация о местоположении объекта
} bim_json_object_t;

/*!
Создает новый объект типа bim_object_t

\param[in] filename Имя файла
\returns Указатель на объект типа bim_object_t
*/
bim_json_object_t*  bim_json_new        (const char* filename);

/*!
Копирует объект типа bim_object_t и возвращает указатель на новый объект

\param[in] bim_object Объект для копирования
\returns Указатель на объект типа bim_object_t
*/
bim_json_object_t*  bim_json_copy       (const bim_json_object_t *bim_object);

/*!
Удаляет объект типа bim_object_t и освобождает память

\param[in] bim_object Объект типа bim_object_t
*/
void           bim_json_free     (bim_json_object_t* bim_object);

#endif //BIM_OBJECT_H
