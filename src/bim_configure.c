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

#include "bim_configure.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    kMaxFileNameLen = 256,
    kMaxLineLen     = 512
};

_modeling        cfg_modeling;
_transit         cfg_transit;
_distribution    cfg_distribution;

static void remove_comments(char* s);
static void trim(char* s);
static void parse_line(char* line);

int bim_configure(const char* filename)
{
    FILE* fp;
    char line[kMaxLineLen];

    if (filename == NULL)
    {
        assert(0 && "filename must not be NULL");
        return 0;
    }

    if ((fp = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "ERROR: loggerconf: Failed to open file: `%s`\n", filename);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        remove_comments(line);
        trim(line);
        if (line[0] == '\0')
        {
            continue;
        }
        parse_line(line);
    }
    fclose(fp);

    return 1;
}

static void remove_comments(char* s)
{
    for (size_t i = 0; s[i] != '\0'; i++)
    {
        if (s[i] == '#')
        {
            s[i] = '\0';
        }
    }
}

static void trim(char* s)
{
    size_t len;
    int i;

    if (s == NULL) {
        return;
    }
    len = strlen(s);
    if (len == 0) {
        return;
    }
    /* trim right */
    for (i = len - 1; i >= 0 && isspace(s[i]); i--) {}
    s[i + 1] = '\0';
    /* trim left */
    for (i = 0; s[i] != '\0' && isspace(s[i]); i++) {}
    memmove(s, &s[i], len - i);
    s[len - i] = '\0';
}

static enum cfg_distr parse_distribution(const char* s);
static enum cfg_transit_width parse_transit_width(const char* s);

static void parse_line(char* line)
{
    char *key, *val;

    key = strtok(line, "=");
    val = strtok(NULL, "=");

    if (strcmp(key, "distribution") == 0)
    {
        cfg_distribution.type = parse_distribution(val);
    }
    else if (strcmp(key, "distribution.density") == 0)
    {
        cfg_distribution.density = atof(val);
    }
    else if (strcmp(key, "transit") == 0)
    {
        cfg_transit.type = parse_transit_width(val);
    }
    else if (strcmp(key, "transit.doorway.in") == 0)
    {
        cfg_transit.doorway_in = atof(val);
    }
    else if (strcmp(key, "transit.doorway.out") == 0)
    {
        cfg_transit.doorway_out = atof(val);
    }
    else if (strcmp(key, "modeling.step") == 0)
    {
        cfg_modeling.step = atof(val);
    }
    else if (strcmp(key, "modeling.speed.max") == 0)
    {
        cfg_modeling.speed_max = atof(val);
    }
    else if (strcmp(key, "modeling.density.min") == 0)
    {
        cfg_modeling.density_min = atof(val);
    }
    else if (strcmp(key, "modeling.density.max") == 0)
    {
        cfg_modeling.density_max = atof(val);
    }
}

static enum cfg_distr parse_distribution(const char* s)
{
    if (strcmp(s, "BIM") == 0) {
        return Distribution_BIM;
    } else if (strcmp(s, "UNIFORM") == 0) {
        return Distribution_UNIFORM;
    } else {
        fprintf(stderr, "ERROR: loggerconf: Invalid distribution: `%s`\n", s);
        return Distribution_BIM;
    }
}

static enum cfg_transit_width parse_transit_width(const char* s)
{
    if (strcmp(s, "BIM") == 0) {
        return TransitWidth_BIM;
    } else if (strcmp(s, "SPECIAL") == 0) {
        return TransitWidth_SPECIAL;
    } else {
        fprintf(stderr, "ERROR: loggerconf: Invalid transit_width: `%s`\n", s);
        return TransitWidth_BIM;
    }
}
