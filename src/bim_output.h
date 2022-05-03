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
#ifndef BIM_OUTPUT_H
#define BIM_OUTPUT_H

#include <stdio.h>
#include "bim_tools.h"

// Separator
#define SEP ","
// Suffix of output file
#define OUTPUT_SUFFIX ".csv"
// Part of the output file name
#define OUTPUT_DETAIL_FILE  "_detailed"
#define OUTPUT_SHORT_FILE   "_short"

void bim_output_head  (const bim_t *const bim, FILE *fp);
void bim_output_body  (const bim_t *const bim, float time, FILE *fp);

char* bim_basename          (char *path_to_file);
char *bim_create_file_name  (const char* bfn, const char* middle_name, const char* suffix);

#endif //BIM_OUTPUT_H
