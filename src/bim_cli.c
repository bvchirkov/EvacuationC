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

#include "bim_cli.h"

static cli_params_t cli_params;

// Справка по использованию аргументов программы
static void usage(const char *argv0, int exitval, const char *errmsg)
{
    FILE *fp = stdout;
    if (exitval != 0)
        fp = stderr;
    if (errmsg != NULL)
        fprintf(fp, "ОШИБКА: %s\n\n", errmsg);

    fprintf(fp, "Использование: %s -s путь/до/файла\n", argv0);
    fprintf(fp, "  -s - Файл конфигурции сценария моделирования\n");

    exit(exitval);
}

const cli_params_t* read_cl_args(int argc, char** argv)
{
    int c;
    while ((c = getopt(argc, argv, "s:h")) != -1)
    {
        switch (c)
        {
        case 's': cli_params.scenario_file = optarg;    break;
        case 'h': usage(argv[0], EXIT_SUCCESS, NULL);   break;
        default: /* '?' */ usage(argv[0], EXIT_FAILURE, "Неизвестный аргумент");
        }
    }

    if (argc == 1)
        usage(argv[0], EXIT_FAILURE, "Ожидаются аргументы");

    return &cli_params;
}
