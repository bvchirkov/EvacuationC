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

#ifndef BIM_GRAPH_H
#define BIM_GRAPH_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>

#include "bim_tools.h"

typedef struct graph bim_graph_t;
typedef struct node  bim_node_t;
typedef struct edge  bim_edge_t;

//https://www.techiedelight.com/implement-graph-data-structure-c

// Data structure to store a graph object
struct graph
{
    // An array of pointers to Node to represent an adjacency list
    bim_node_t**  head;
    size_t        node_count;
};

// Data structure to store adjacency list nodes of the graph
struct node
{
    size_t      dest;
    size_t      eid; // edge id
    bim_node_t* next;
};

// Data structure to store a graph edge
struct edge
{
    size_t src;
    size_t dest;
    size_t id;
};

bim_graph_t*  bim_graph_new    (const bim_t *const bim);
void          bim_graph_print  (const bim_graph_t *const graph);
void          bim_graph_free   (bim_graph_t* graph);

#endif //BIM_GRAPH_H
