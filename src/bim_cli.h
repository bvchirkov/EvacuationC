/* Copyright © 2022 bvchirkov
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
#ifndef BIM_CLI_H
#define BIM_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

typedef struct
{
//    char *json;
    char *scenario_file;
//    char *results;
//    char *logger_config;
} cli_params_t;

// Обработка аргументов командной строки
const cli_params_t* read_cl_args(int argc, char** argv);

#endif // BIM_CLI_H
