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

bim_graph*  _graph_create(const bim_edge *edges, uint32_t edge_count, uint32_t node_count);
void        _graph_create_edges(const ArrayList *list_doors, ArrayListEqualFunc callback, bim_edge *edges, const ArrayList *rooms_and_stairs);
int32_t     _arraylist_equal_callback(const ArrayListValue value1, const ArrayListValue value2);

bim_graph *bim_graph_new(const ArrayList *zones, const ArrayList *transits)
{
    bim_edge edges[transits->length];
    _graph_create_edges(transits, _arraylist_equal_callback, edges, zones);

    bim_graph *bim_graph = _graph_create(edges, transits->length, zones->length);
    if (!bim_graph)
    {
        return NULL;
    }

    return bim_graph;
}

// Function to print adjacency list representation of a graph
void bim_graph_print(const bim_graph* graph)
{
    for (size_t i = 0; i < graph->node_count; i++)
    {
        // print current vertex and all its neighbors
        const bim_node* ptr = graph->head[i];
        while (ptr != NULL)
        {
            printf("%zu —(%lu)-> %lu\t", i, ptr->eid, ptr->dest);
            ptr = ptr->next;
        }
        printf("\n");
    }
}

void bim_graph_free(bim_graph* graph)
{
    for(size_t i = 0; i < graph->node_count; i++)
    {
        free(graph->head[i]);
    }
    free(graph->head);
    free(graph);
}

// Function to create an adjacency list from specified edges
bim_graph* _graph_create(const bim_edge *edges, uint32_t edge_count, uint32_t node_count)
{
    if (!edges)
        return NULL;

    if (!node_count || !edge_count)
        return NULL;

    // allocate storage for the graph data structure
    bim_graph* graph = (bim_graph*)malloc(sizeof(bim_graph));
    if (!graph)
        return NULL;

    // initialize head pointer for all vertices
    graph->head = (bim_node**)malloc(sizeof(bim_node*) * node_count);
    for (size_t i = 0; i < node_count; i++)
    {
        graph->head[i] = NULL;
    }
    graph->node_count = node_count;

    uint64_t src = 0;
    uint64_t dest = 0;
    uint64_t eid = 0;
    // add edges to the directed graph one by one
    for (uint32_t i = 0; i < edge_count; i++, edges++)
    {
        // get the source and destination vertex
        src = edges->src;
        dest = edges->dest;
        eid = edges->id;

        // 1. allocate a new node of adjacency list from `src` to `dest`

        bim_node* newNode = (bim_node*)malloc(sizeof(bim_node));
        newNode->dest = dest;
        newNode->eid = eid;

        // point new node to the current head
        newNode->next = graph->head[src];

        // point head pointer to the new node
        graph->head[src] = newNode;

        // 2. allocate a new node of adjacency list from `dest` to `src`

        newNode = (bim_node*)malloc(sizeof(bim_node));
        newNode->dest = src;
        newNode->eid = eid;

        // point new node to the current head
        newNode->next = graph->head[dest];

        // change head pointer to point to the new node
        graph->head[dest] = newNode;
    }

    return graph;
}

void _graph_create_edges(const ArrayList *list_doors, ArrayListEqualFunc callback, bim_edge *edges, const ArrayList *rooms_and_stairs)
{
    for (size_t i = 0; i < list_doors->length; i++, edges++)
    {
        edges->id = i;

        uint64_t ids[] = {0, rooms_and_stairs->length};
        for (size_t k = 0, j = 0; k < rooms_and_stairs->length; ++k)
            if (callback(rooms_and_stairs->data[k], list_doors->data[i]) && j != 2)
                ids[j++] = k;

        edges->src = ids[0];
        edges->dest = ids[1];
    }
}

int32_t _arraylist_equal_callback(const ArrayListValue value1, const ArrayListValue value2)
{
    const bim_zone_t *element1 = value1;
    const bim_transit_t *element2 = value2;

    for(size_t i = 0; i < element1->base->outputs_count; i++)
    {
        if (strcmp(element1->base->outputs[i], element2->base->uuid) == 0)
        {
            return 1;
        }
    }

    return 0;
}
