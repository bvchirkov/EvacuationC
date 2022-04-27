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

#include "bim_graph.h"

bim_graph_t*  _graph_create(const bim_edge_t edges[], size_t edge_count, size_t node_count);
void          _graph_create_edges(const ArrayList *const list_doors, ArrayListEqualFunc callback, bim_edge_t edges[], const ArrayList *const zones);
int32_t       _arraylist_equal_callback(const ArrayListValue value1, const ArrayListValue value2);

bim_graph_t *bim_graph_new(const bim_t *const bim)
{
    bim_edge_t *edges = NULL;
    edges = (bim_edge_t*)calloc(bim->transits->length, sizeof (bim_edge_t));
    if (!edges) {
        return NULL;
    }

    _graph_create_edges(bim->transits, _arraylist_equal_callback, edges, bim->zones);

    bim_graph_t *bim_graph = NULL;
    bim_graph = _graph_create(edges, bim->transits->length, bim->zones->length);
    free(edges);
    if (!bim_graph) {
        return NULL;
    }

    return bim_graph;
}

// Function to print adjacency list representation of a graph
void bim_graph_print(const bim_graph_t *const graph)
{
    LOG_TRACE("-------------------------------------------------------------");
    LOG_TRACE("It is printed the graph struct [room_id —(door_id)-> room_id]");
    for (size_t i = 0; i < graph->node_count; i++)
    {
        // print current vertex and all its neighbors
        const bim_node_t *ptr = graph->head[i];
        while (ptr != NULL)
        {
            LOG_TRACE("%zu —(%lu)-> %lu\t", i, ptr->eid, ptr->dest);
            ptr = ptr->next;
        }
        LOG_TRACE("");
    }
}

void bim_graph_free(bim_graph_t* graph)
{
    for(size_t i = 0; i < graph->node_count; ++i)
    {
        free(graph->head[i]);
    }
    free(graph->head);
    free(graph);
}

// Function to create an adjacency list from specified edges
bim_graph_t* _graph_create(const bim_edge_t edges[], size_t edge_count, size_t node_count)
{
    if (!node_count || !edge_count) {
        return NULL;
    }

    // allocate storage for the graph data structure
    bim_graph_t *graph = NULL;
    graph = (bim_graph_t*)calloc(1, sizeof(bim_graph_t));
    if (!graph) {
        return NULL;
    }

    // initialize head pointer for all vertices
    graph->head = NULL;
    graph->head = (bim_node_t**)calloc(node_count, sizeof(bim_node_t*));
    if (!graph->head) {
        free(graph);
        return NULL;
    }

    for (size_t i = 0; i < node_count; i++)
    {
        graph->head[i] = NULL;
    }
    graph->node_count = node_count;

    size_t src  = 0;
    size_t dest = 0;
    size_t eid  = 0;
    // add edges to the directed graph one by one
    for (size_t i = 0; i < edge_count; ++i)
    {
        const bim_edge_t *edge = &edges[i];
        // get the source and destination vertex
        src  = edge->src;
        dest = edge->dest;
        eid  = edge->id;

        // 1. allocate a new node of adjacency list from `src` to `dest`

        bim_node_t* newNode = (bim_node_t*)calloc(1, sizeof(bim_node_t));
        if (!newNode) {
            free(graph->head);
            free(graph);
            return NULL;
        }
        newNode->dest = dest;
        newNode->eid  = eid;

        // point new node to the current head
        newNode->next = graph->head[src];

        // point head pointer to the new node
        graph->head[src] = newNode;

        // 2. allocate a new node of adjacency list from `dest` to `src`

        newNode = (bim_node_t*)calloc(1, sizeof(bim_node_t));
        if (!newNode) {
            free(graph->head);
            free(graph);
            return NULL;
        }
        newNode->dest = src;
        newNode->eid  = eid;

        // point new node to the current head
        newNode->next = graph->head[dest];

        // change head pointer to point to the new node
        graph->head[dest] = newNode;
    }

    return graph;
}

void _graph_create_edges(const ArrayList *const list_doors, ArrayListEqualFunc callback, bim_edge_t edges[], const ArrayList *const zones)
{
    for (size_t i = 0; i < list_doors->length; ++i)
    {
        bim_edge_t *edge = &edges[i];
        edge->id = i;

        size_t ids[2] = {0, zones->length};
        for (size_t k = 0, j = 0; k < zones->length; ++k)
            if (callback(zones->data[k], list_doors->data[i]) && j != 2)
                ids[j++] = k;

        edge->src  = ids[0];
        edge->dest = ids[1];
    }
}

int32_t _arraylist_equal_callback(const ArrayListValue value1, const ArrayListValue value2)
{
    const bim_zone_t    *element1 = (bim_zone_t*)   value1;
    const bim_transit_t *element2 = (bim_transit_t*)value2;

    for(size_t i = 0; i < element1->numofoutputs; ++i)
    {
        if (strcmp(element1->outputs[i].x, element2->uuid.x) == 0)
            return 1;
    }

    return 0;
}
