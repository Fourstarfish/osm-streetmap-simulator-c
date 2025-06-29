#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "streets.h"




// Represents a single point or location in the map.
struct node {
    int id; // Unique identifier for the node.
    double lat; // Latitude of the node, specifying its north-south position.
    double lon; // Longitude of the node, specifying its east-west position.
    struct way **ways; // Array of pointers to 'way' structures that this node is part of.
    int num_ways; // Number of ways that this node is connected to.
};

// Represents a road segment between nodes in the map.
struct way {
  int id; // Unique identifier for the way.
  char *name; // Pointer to a dynamically allocated character array.
  float max_speed; // Maximum legal speed on this way in some unit (e.g., kilometers per hour).
  bool one_way; // Boolean flag indicating if the way is one-way (true) or two-way (false).
  int *node_ids; // Array of node IDs that make up this way, ordered from start to end.
  int num_nodes; // Number of nodes in this way.
};

// Represents the entire map, consisting of nodes and ways.
struct ssmap {
  struct node **nodes; // Array of pointers to 'node' structures in the map.
  struct way **ways; // Array of pointers to 'way' structures in the map.
  int num_nodes; // Total number of nodes in the map.
  int num_ways; // Total number of ways in the map.
};

// Used within the min_heap to associate a node with its current shortest distance from the start node.
typedef struct {
  int node_id; // Identifier for the node this entry corresponds to.
  double distance; // Current shortest distance from the start node to this node.
} heap_node;

// A min-heap (priority queue) structure used to efficiently select the next node to process based on distance.
typedef struct {
  heap_node *elements; // Dynamic array of heap_node elements making up the heap.
  int size; // Current number of elements in the heap.
  int capacity; // Maximum number of elements the heap can contain.
} min_heap;


/**
 * Creates a new min_heap with a specified capacity.
 *
 * @param capacity The maximum number of elements the heap can hold.
 * @return A pointer to the newly created min_heap structure.
 *
 * This function dynamically allocates memory for a min_heap structure and its array of elements,
 * initializing the size to 0 and setting its capacity to the specified value. The function returns
 * a pointer to the allocated min_heap. If memory allocation fails at any step, the function will
 * return NULL to indicate failure.
 */
 min_heap* create_min_heap(int capacity) {
     // Allocate memory for the min_heap structure itself.
     min_heap *heap = (min_heap *)malloc(sizeof(min_heap)); // Changed variable name to 'heap'
     if (heap == NULL) {
         return NULL;
     }
     // Allocate memory for the array of heap_node elements within the min_heap.
     heap->elements = (heap_node *)malloc(sizeof(heap_node) * capacity);
     if (heap->elements == NULL) {
         free(heap);
         return NULL;
     }
     // Initialize the size of the heap to 0, indicating it's currently empty.
     heap->size = 0;
     // Set the capacity of the heap to the specified value.
     heap->capacity = capacity;
     // Return a pointer to the successfully created min_heap structure.
     return heap;
 }

/**
 * Swaps two heap_node elements in the min_heap.
 *
 * @param a Pointer to the first heap_node to be swapped.
 * @param b Pointer to the second heap_node to be swapped.
 *
 * This function performs a swap operation between two heap_node elements. It's used to maintain
 * the min-heap property during heap operations such as insertion, deletion, and key decrease.
 * The swap is done by copying the contents of one node to a temporary node, then copying
 * the contents of the second node to the first, and finally copying the contents of the
 * temporary node to the second. This ensures that the actual nodes in the heap are swapped,
 * not just their distances or node IDs.
 */
void swap_heap_node(heap_node* a, heap_node* b) {
    // Temporary storage to hold the contents of the first node
    heap_node t = *a;
    // Copy the contents of the second node to the first node
    *a = *b;
    // Copy the contents of the temporary storage (originally the first node) to the second node
    *b = t;

}

/**
 * Maintains the min-heap property of a given subtree in the min_heap data structure.
 *
 * @param min_heap Pointer to the min_heap structure.
 * @param idx Index of the root node of the subtree within the heap's array to be heapified.
 *
 * This function ensures that the subtree rooted at the given index adheres to the min-heap
 * property, where the key (distance in this context) of a parent node is less than or equal to
 * the keys of its children. It recursively adjusts parts of the heap that violate this property
 * by swapping nodes and heapifying the affected sub-trees.
 *
 * The function calculates the indices of the left and right children of the current node based
 * on the current node's index. It then compares the distances of these children to the distance
 * of the current node to find the smallest among the three. If the current node is not the smallest,
 * it swaps the current node with the smallest of its children and recursively applies the same
 * process to the subtree rooted at the new position of the swapped node.
 */
void min_heapify(min_heap *min_heap, int idx) {
    // Initialize indices for the current node, its left child, and its right child.
    int smallest, left, right;
    smallest = idx;
    left = 2 * idx + 1;
    right = 2 * idx + 2;

    // Check if the left child exists and is smaller than the current node.
    if (left < min_heap->size && min_heap->elements[left].distance < min_heap->elements[smallest].distance) {
        smallest = left; // Update smallest if the left child is smaller.
    }

    // Check if the right child exists and is smaller than the current (or left child) node.
    if (right < min_heap->size && min_heap->elements[right].distance < min_heap->elements[smallest].distance) {
        smallest = right; // Update smallest if the right child is smaller.
    }

    // If the smallest node is not the current node, swap and heapify the affected subtree.
    if (smallest != idx) {
        swap_heap_node(&min_heap->elements[idx], &min_heap->elements[smallest]);
        min_heapify(min_heap, smallest); // Recursively apply min_heapify to the subtree affected by the swap.
    }

}

/**
 * Extracts and returns the minimum element from the min_heap.
 *
 * @param min_heap Pointer to the min_heap structure from which to extract the minimum element.
 * @return The heap_node representing the minimum element in the heap. If the heap is empty,
 *         returns a heap_node with an ID of -1 and a distance of 0 as an indication of failure.
 *
 * This function removes the root of the min_heap, which is the minimum element due to the
 * min-heap property, and returns it. To maintain the min-heap property after removal,
 * it places the last element of the heap at the root position and then applies the
 * min_heapify function to restructure the heap. This ensures that the structure remains
 * a valid min-heap after the minimum element's extraction.
 */
heap_node extract_min(min_heap *min_heap) {
    // Check if the heap is empty; if so, return a default heap_node indicating failure.
    if (min_heap->size == 0) {
        return (heap_node){-1, 0}; // Use -1 as an invalid node_id and 0 as the distance.
    }

    // Store the root of the heap (the minimum element) to return it later.
    heap_node root = min_heap->elements[0];

    // Move the last element in the heap to the root position.
    min_heap->elements[0] = min_heap->elements[min_heap->size - 1];

    // Decrease the heap's size since the minimum element is being removed.
    min_heap->size--;

    // Reapply the min-heap property starting from the new root.
    min_heapify(min_heap, 0);

    // Return the originally stored root, which is the minimum element.
    return root;

}

/**
 * Decreases the distance value for a specified node in the min_heap and restructures
 * the heap if necessary to maintain the min-heap property.
 *
 * @param min_heap Pointer to the min_heap structure.
 * @param node_id The identifier of the node for which the distance value is to be decreased.
 * @param distance The new distance value for the node, which is assumed to be less than
 *        the node's current distance value.
 *
 * This function first locates the node with the given node_id within the heap. Once found,
 * it updates the node's distance to the new, lower value. To maintain the min-heap property
 * (where the parent node's distance is always less than or equal to its children's distances),
 * the function then iteratively swaps the updated node with its parent node as long as the
 * updated node's distance is less than its parent's distance. This process continues until
 * the node reaches a position where the min-heap property is restored.
 */
void decrease_key(min_heap *min_heap, int node_id, double distance) {
    // Loop through the heap to find the node with the specified node_id.
    for (int i = 0; i < min_heap->size; i++) {
        if (min_heap->elements[i].node_id == node_id) {
            // Once found, update the node's distance to the new, decreased value.
            min_heap->elements[i].distance = distance;

            // Move the node up the heap to its correct position to maintain the min-heap property.
            // This is done by comparing and potentially swapping the node with its parent.
            while (i != 0 && min_heap->elements[(i - 1) / 2].distance > min_heap->elements[i].distance) {
                // Swap the current node with its parent if the current node's distance is smaller.
                swap_heap_node(&min_heap->elements[i], &min_heap->elements[(i - 1) / 2]);

                // Update the index 'i' to the parent's index after swapping.
                i = (i - 1) / 2;
            }
            break; // Exit the loop after updating the node's distance and repositioning it in the heap.
        }
    }

}

/**
 * Checks whether a node is present in the min_heap.
 *
 * @param min_heap Pointer to the min_heap structure to be searched.
 * @param node_id The identifier of the node to search for within the min_heap.
 * @return A boolean value indicating whether the node is found in the min_heap.
 *         Returns true if the node is found; otherwise, returns false.
 *
 * This function iterates over all elements in the min_heap's elements array, comparing
 * each element's node_id with the specified node_id. The search continues until either
 * the node is found (in which case true is returned) or all elements have been checked
 * (resulting in false being returned if the node is not found). This function is useful
 * for determining whether to insert a new node into the min_heap or to update an existing
 * node's distance value using the decrease_key function.
 */
bool is_in_min_heap(min_heap *min_heap, int node_id) {
    // Loop through all the nodes currently in the min_heap.
    for (int i = 0; i < min_heap->size; i++) {
        // Check if the current node's ID matches the specified node_id.
        if (min_heap->elements[i].node_id == node_id) {
            // If a match is found, return true to indicate the node is in the min_heap.
            return true;
        }
    }
    // If no match is found after checking all nodes, return false.
    return false;

}

/**
 * Inserts a new node into the min_heap with a given distance.
 *
 * @param min_heap Pointer to the min_heap structure where the new node will be inserted.
 * @param node_id The identifier of the node to be inserted.
 * @param distance The distance value associated with the node, used as the key for heap insertion.
 *
 * This function inserts a new node into the min_heap while maintaining the min-heap property,
 * which requires that every parent node's distance is less than or equal to the distances of its children.
 * If the heap is already at full capacity, the function will not insert the new node. After insertion,
 * the function ensures the min-heap property is maintained by adjusting the position of the newly inserted
 * node as necessary through a series of swaps with its parent nodes.
 */
void insert_min_heap(min_heap *min_heap, int node_id, double distance) {
    // Check if the heap has reached its maximum capacity.
    if (min_heap->size == min_heap->capacity) {
        // If the heap is full, do not insert the new node and return early.
        return;
    }

    // Increase the size of the heap to accommodate the new node.
    min_heap->size++;
    // Calculate the index where the new node will be placed (at the end of the heap).
    int i = min_heap->size - 1;

    // Initialize the new node at the calculated position with the given node_id and distance.
    min_heap->elements[i].node_id = node_id;
    min_heap->elements[i].distance = distance;

    // Fix the min-heap property if it is violated due to the insertion of the new node.
    // This is done by comparing the new node's distance with its parent's distance and
    // swapping them if the parent's distance is greater, thereby moving the new node up the heap.
    while (i != 0 && min_heap->elements[(i - 1) / 2].distance > min_heap->elements[i].distance) {

        // Swap the new node with its parent.
        swap_heap_node(&min_heap->elements[i], &min_heap->elements[(i - 1) / 2]);

        // Update the index 'i' to that of the parent after the swap, and repeat the process
        // until the new node is in the correct position or it becomes the root of the heap.
        i = (i - 1) / 2;
    }

}

/**
 * Searches for a specific node within a way and returns its index.
 *
 * @param way Pointer to the 'way' structure in which to search for the node.
 * @param node_id The identifier of the node to search for within the way.
 * @return The index of the node within the way's node_ids array if found;
 *         otherwise, returns -1 to indicate that the node is not part of the way.
 *
 * This function iterates through the array of node IDs in the specified way to find
 * a node with the given node_id. It's used to determine the position of a node within
 * a way, which is essential for understanding the connectivity and traversal possibilities
 * within the map, especially when dealing with adjacency in pathfinding algorithms.
 */
int find_node_index_in_way(const struct way *way, int node_id) {
    // Loop through all nodes in the way's node_ids array.
    for (int i = 0; i < way->num_nodes; ++i) {
        // Check if the current node's ID matches the specified node_id.
        if (way->node_ids[i] == node_id) {
            // If a match is found, return the current index.
            return i;
        }
    }
    // If no match is found after checking all nodes, return -1 to indicate failure.
    return -1;

}

/**
 * Frees the memory allocated for a min_heap structure.
 *
 * @param min_heap Pointer to the min_heap structure to be freed.
 *
 * This function ensures that all memory allocated for the min_heap, including its
 * elements array, is properly freed. It's crucial to call this function to avoid
 * memory leaks once the min_heap is no longer needed.
 */
void free_min_heap(min_heap *min_heap) {
    if (min_heap != NULL) {
        // Free the elements array if it's not NULL.
        if (min_heap->elements != NULL) {
            free(min_heap->elements);
            min_heap->elements = NULL; // Set to NULL to avoid dangling pointer.
        }
        // Free the min_heap structure itself.
        free(min_heap);
        min_heap = NULL; // Set to NULL to avoid dangling pointer.
    }
}

/**
 * Creates a new Simple Street Map (ssmap) with specified numbers of nodes and ways.
 *
 * @param nr_nodes The number of nodes to be included in the map.
 * @param nr_ways The number of ways to be included in the map.
 * @return A pointer to the newly created ssmap structure, or NULL if the map could not be created.
 *
 * This function initializes a new ssmap structure, allocating memory for the specified
 * numbers of nodes and ways. It ensures that all necessary memory allocations are successful
 * before returning a pointer to the initialized map. If any allocation fails, it cleans up
 * previously allocated memory and returns NULL to indicate failure. This function is essential
 * for setting up the data structure that represents the map before adding actual nodes and ways.
 */
struct ssmap *
ssmap_create(int nr_nodes, int nr_ways)
{
    // Check if the number of nodes or ways is zero, which is not valid for creating a map.
    if (nr_nodes == 0 || nr_ways == 0) {
        return NULL;
    }

    // Allocate memory for the ssmap structure itself.
    struct ssmap *map = malloc(sizeof(struct ssmap));
    if (map == NULL) {
        return NULL;
    }

    // Allocate memory for the array of pointers to 'way' structures.
    map->ways = malloc(nr_ways * sizeof(struct way *));
    if (map->ways == NULL) {
        free(map);
        return NULL;
    }

    // Allocate memory for the array of pointers to 'node' structures.
    map->nodes = malloc(nr_nodes * sizeof(struct node *));
    if (map->nodes == NULL) {
        free(map->ways);
        free(map);
        return NULL;
    }

    // Set the number of nodes and ways in the map to the specified values.
    map->num_nodes = nr_nodes;
    map->num_ways = nr_ways;

    // Return a pointer to the successfully created ssmap structure.
    return map;

}

/**
 * Performs additional initialization tasks for a Simple Street Map (ssmap) structure.
 *
 * @param m Pointer to the ssmap structure to be further initialized.
 * @return A boolean value indicating the success of the initialization process.
 *         Returns true if initialization is successful, and false if there are any issues.
 *
 * This function is designed to perform any additional initialization logic required for
 * the ssmap structure after its basic setup. This might include setting up data structures,
 * pre-computing values, or any other initialization tasks that are necessary before the map
 * can be used. The current implementation returns true to indicate success, but this function
 * can be expanded to include actual initialization logic as needed. If during the initialization
 * any issue is encountered (e.g., out of memory), the function should return false.
 *
 */
bool
ssmap_initialize(struct ssmap * m)
{
    // Currently, there's no additional initialization logic, so return true.
    return true;

}

/**
 * Frees all memory allocated for a Simple Street Map (ssmap) structure and its components.
 *
 * @param m Pointer to the ssmap structure to be destroyed.
 *
 * This function is responsible for properly freeing all dynamically allocated memory associated
 * with the ssmap structure, including memory for nodes, ways, and their respective components.
 * It iterates through the arrays of nodes and ways, freeing each individual node and way, along
 * with their dynamically allocated members, before finally freeing the arrays themselves and the
 * ssmap structure. This cleanup helps prevent memory leaks and ensures that the resources are
 * properly returned to the system upon the map's disposal.
 */
void
ssmap_destroy(struct ssmap * m)
{
    // Check if the nodes array is not NULL to avoid dereferencing a NULL pointer.
    if (m->nodes != NULL) {
        // Iterate through each node in the nodes array.
        for (int i = 0; i < m->num_nodes; i++) {
            struct node *current_node = m->nodes[i];
            // Ensure the current node is not NULL before attempting to free.
            if (current_node != NULL) {
                // Free the ways array associated with the current node.
                free(current_node->ways);
                // Free the current node itself.
                free(current_node);
            }
        }
        // After all nodes have been processed, free the nodes array.
        free(m->nodes);
    }

    // Repeat a similar process for the ways array.
    if (m->ways != NULL) {
        // Iterate through each way in the ways array.
        for (int i = 0; i < m->num_ways; i++) {
          struct way *current_way = m->ways[i];
          // Ensure the current way is not NULL before attempting to free.
          if (current_way != NULL) {
            // Free the node_ids array associated with the current way.
            free(current_way->node_ids);
            // Free the dynamically allocated name.
            free(current_way->name);
            // Free the current way itself.
            free(current_way);
          }
        }
        // After all ways have been processed, free the ways array.
        free(m->ways);
    }

    // Finally, free the ssmap structure itself.
    free(m);

}

/**
 * Adds a new way to the Simple Street Map (ssmap) structure.
 *
 * @param m Pointer to the ssmap structure where the new way will be added.
 * @param id Unique identifier for the new way.
 * @param name Name of the new way. If the way is unnamed, this should be an empty string or a placeholder.
 * @param maxspeed Maximum legal speed on the new way, in the unit specified by the map's convention (e.g., km/h).
 * @param oneway Boolean flag indicating if the new way is one-way (true) or two-way (false).
 * @param num_nodes Number of nodes that make up the new way.
 * @param node_ids Array of node IDs that constitute the way, ordered from start to end.
 * @return A pointer to the newly created way structure, or NULL if the creation fails.
 *
 * This function creates and initializes a new way structure with the specified properties,
 * including its connectivity (nodes that make up the way). It dynamically allocates memory
 * for the new way and its array of node IDs. After successful creation, the new way is added
 * to the ssmap's array of ways using the way's ID as the index. If any memory allocation fails,
 * the function ensures to clean up any allocated memory before returning NULL to prevent leaks.
 */
struct way *
ssmap_add_way(struct ssmap * m, int id, const char * name, float maxspeed, bool oneway,
              int num_nodes, const int node_ids[num_nodes])
{
    // Allocate memory for the new way structure.
    struct way *new_way = malloc(sizeof(struct way));
    if (new_way == NULL) {
        // If memory allocation fails, return NULL.
        return NULL;
    }

    // Initialize the new way's properties.
    new_way->id = id;

    // Dynamically allocate memory for the name and copy it.
    new_way->name = malloc(strlen(name) + 1); // Plus one for null terminator
    if (new_way->name == NULL) {
        // If memory allocation for name fails, clean up and return NULL.
        free(new_way); // Only need to free new_way since name allocation failed.
        return NULL;
    }
    strcpy(new_way->name, name); // Safe to use strcpy as memory is allocated based on name length.

    new_way->max_speed = maxspeed;
    new_way->one_way = oneway;
    new_way->num_nodes = num_nodes;

    // Allocate memory for the array of node IDs that make up the new way.
    new_way->node_ids = malloc(num_nodes * sizeof(int));
    if (new_way->node_ids == NULL) {
        // If memory allocation for node IDs fails, clean up and return NULL.
        free(new_way->name);
        free(new_way); // Need to free both new_way and name as they were successfully allocated.
        return NULL;
    }

    // Copy the provided node IDs to the new way's node_ids array.
    memcpy(new_way->node_ids, node_ids, num_nodes * sizeof(int));

    // Add the new way to the ssmap's array of ways.
    m->ways[id] = new_way;

    // Return a pointer to the successfully created and added new way.
    return new_way;

}

/**
 * Adds a new node to the Simple Street Map (ssmap) structure.
 *
 * @param m Pointer to the ssmap structure to which the node will be added.
 * @param id The unique identifier for the new node.
 * @param lat The latitude coordinate of the new node.
 * @param lon The longitude coordinate of the new node.
 * @param num_ways The number of ways (roads) that the new node is connected to.
 * @param way_ids Array containing the identifiers of the ways that the new node is connected to.
 * @return A pointer to the newly created node structure, or NULL if the node could not be created.
 *
 * This function dynamically allocates memory for a new node and initializes it with the
 * provided details, including its location (latitude and longitude), connections to ways,
 * and its unique identifier. It also links the node to its associated ways based on the provided
 * way_ids array. If successful, the new node is added to the ssmap structure and a pointer to the
 * node is returned. If any memory allocation fails, the function cleans up any allocated resources
 * and returns NULL.
 */
struct node *
ssmap_add_node(struct ssmap * m, int id, double lat, double lon,
               int num_ways, const int way_ids[num_ways])
{
    // Check if the ssmap structure is NULL, indicating an invalid map.
    if (m == NULL) {
        return NULL;
    }

    // Dynamically allocate memory for the new node.
    struct node *new_node = malloc(sizeof(struct node));
    if (new_node == NULL) { // Check for successful memory allocation.
        return NULL;
    }

    // Initialize the new node with the provided parameters.
    new_node->id = id;
    new_node->lat = lat;
    new_node->lon = lon;
    new_node->num_ways = num_ways;

    // Allocate memory for the array of pointers to the ways connected to this node.
    new_node->ways = malloc(num_ways * sizeof(struct way *));
    if (new_node->ways == NULL) { // Check for successful memory allocation.
        free(new_node); // Clean up the previously allocated node memory.
        return NULL;
    }

    // Populate the ways array for the new node using the provided way_ids.
    for (int i = 0; i < num_ways; i++) {
        // Assign pointers to the actual way structures based on the provided way_ids.
        new_node->ways[i] = m->ways[way_ids[i]];
    }

    // Add the new node to the ssmap structure, using its ID as the index.
    m->nodes[id] = new_node;

    // Return a pointer to the newly created node.
    return new_node;

}

/**
 * Prints information about a specific way within the Simple Street Map (ssmap).
 *
 * @param m Pointer to the ssmap structure from which the way's information will be printed.
 * @param id The unique identifier of the way to be printed.
 *
 * This function looks up a way by its identifier within the provided ssmap structure
 * and prints its details, including its ID and name. If the specified way ID is out of range
 * or does not correspond to an existing way in the map, an error message is printed instead.
 *
 */
void
ssmap_print_way(const struct ssmap * m, int id)
{
    // Validate the way ID is within the valid range and the way exists.
    if (id < 0 || id >= m->num_ways || m->ways[id] == NULL) {
        // If the way ID is invalid or the way does not exist, print an error message.
        printf("error: way %d does not exist\n", id);
    } else {
        // If the way exists, retrieve the pointer to the way structure.
        struct way *way_to_print = m->ways[id];
        // Retrieve the pointer to the way's name for easier access.
        char *name_ptr = way_to_print->name;
        // Print the way's ID and name.
        printf("Way %d: %s\n", id, name_ptr);

    }

}

/**
 * Prints information about a specific node within the Simple Street Map (ssmap).
 *
 * @param m Pointer to the ssmap structure from which the node's information will be printed.
 * @param id The unique identifier of the node to be printed.
 *
 * This function searches for a node by its identifier within the provided ssmap structure
 * and prints its details, including its ID, latitude, and longitude. If the specified node ID
 * is out of range or does not correspond to an existing node in the map, an error message is
 * printed instead.
 *
 */
void
ssmap_print_node(const struct ssmap * m, int id)
{
    // Validate the node ID is within the valid range and the node exists.
    if (id < 0 || id >= m->num_nodes || m->nodes[id] == NULL) {
        // If the node ID is invalid or the node does not exist, print an error message.
        printf("error: node %d does not exist\n", id);
    } else {
        // If the node exists, retrieve the pointer to the node structure.
        struct node *node_to_print = m->nodes[id];
        // Print the node's ID and its coordinates (latitude and longitude) to a precision of 6 decimal places.
        printf("Node %d: (%.7f, %.7f)\n", id, node_to_print->lat, node_to_print->lon);

    }

}

/**
 * Searches for and prints the IDs of all ways within the Simple Street Map (ssmap)
 * that contain a specified name.
 *
 * @param m Pointer to the ssmap structure to be searched.
 * @param name The name (or substring of the name) to search for within the ways' names.
 *
 * This function iterates over all ways in the provided ssmap structure, checking if
 * each way's name contains the specified search string. If a match is found (indicating
 * that the current way's name contains the specified name as a substring), the ID of
 * the way is printed to the standard output. This allows for a simple search functionality
 * within the map, useful for identifying specific roads or paths by name or part of a name.
 *
 * Note: The search is case-sensitive and looks for the presence of the specified name
 * as a substring within each way's name, using the strstr() function from the C standard library.
 */
void
ssmap_find_way_by_name(const struct ssmap * m, const char * name)
{
    // Iterate through all the ways in the map.
    for (int i = 0; i < m->num_ways; i++) {
        // Check if the current way is not NULL and its name contains the specified substring.
        if (m->ways[i] != NULL && strstr(m->ways[i]->name, name) != NULL) {
            // If a match is found, print the ID of the way.
            printf("%d ", i);
        }
    }

    // Print a newline character after listing all matching way IDs to format the output.
    printf("\n");
}

/**
 * Searches for and prints the IDs of nodes connected to ways that contain specified names.
 *
 * @param m Pointer to the ssmap structure to be searched.
 * @param name1 The first name (or substring of the name) to search for within the ways' names.
 * @param name2 The second name (or substring of the name) to search for within the ways' names.
 *        If name2 is NULL, the function will only search for name1.
 *
 * This function performs a search to find nodes that are connected to ways containing either
 * one or both specified names in their names. It first identifies all ways that match each
 * name and then checks each node to see if it is connected to the identified ways. If a node
 * is connected to ways containing both names (or just one name if name2 is NULL), its ID is printed.
 * This function is useful for finding intersections or common points related to specified road
 * or path names within the map.
 */
void
ssmap_find_node_by_names(const struct ssmap * m, const char * name1, const char * name2)
{
    // Create arrays to store ways containing name1 and name2
    int ways_with_name1[m->num_ways];
    int num_ways_with_name1 = 0;
    int ways_with_name2[m->num_ways];
    int num_ways_with_name2 = 0;

    // Populate the arrays based on whether each way contains name1 or name2 or both
    for (int i = 0; i < m->num_ways; i++) {
        if (strstr(m->ways[i]->name, name1) != NULL) {
            ways_with_name1[num_ways_with_name1++] = i;
            if (name2 != NULL && strstr(m->ways[i]->name, name2) != NULL) {
                ways_with_name2[num_ways_with_name2++] = i;
            }
        }
        else if (name2 != NULL && strstr(m->ways[i]->name, name2) != NULL) {
            ways_with_name2[num_ways_with_name2++] = i;
        }
    }

    // Iterate through each node to check if it's connected to the identified ways.
    for (int i = 0; i < m->num_nodes; i++) {
        // Access the node using node_id
        int node_id = i;
        const struct node *node = m->nodes[node_id];

        // Allocate memory for the array of way IDs
        int *node_way_ids = malloc(node->num_ways * sizeof(int));
        // Ensure memory allocation succeeded.
        if (node_way_ids == NULL) {
            continue; // Skip this node if memory allocation failed.
        }

        // Populate the array with IDs of ways associated with the node
        for (int j = 0; j < node->num_ways; j++) {
            node_way_ids[j] = node->ways[j]->id;
        }

        // If only searching for name1.
        if (name2 == NULL) {
            // Check if the current node is connected to any way containing name1.
            bool bool_checker = false;
            for (int j = 0; j < node->num_ways; j++) {
                bool found_name1 = false;
                for (int k = 0; k < num_ways_with_name1; k++) {
                    if (node_way_ids[j] == ways_with_name1[k]) {
                        found_name1 = true;
                        break; // Exit the loop once a match is found.
                    }
                }

                if (found_name1) {
                    bool_checker = true;
                    break; // Exit the loop once a match is found.
                }

            }

            // Print the node ID if it's connected to a way containing name1.
            if (bool_checker) {
                printf("%d ", node_id);
            }


        } else {
              // If searching for both name1 and name2.
              bool bool_checker1 = false;
              bool bool_checker2 = false;
              for (int j = 0; j < node->num_ways; j++) {
                  // Check for connections to ways containing name1.
                  bool found_name1 = false;
                  bool found_name2 = false;
                  for (int k = 0; k < num_ways_with_name1; k++) {
                      if (node_way_ids[j] == ways_with_name1[k]) {
                          found_name1 = true;
                          for (int l = 0; l < node->num_ways; l++) {
                              // Check for connections to ways containing name2.
                              for (int m = 0; m < num_ways_with_name2; m++) {
                                  if (node_way_ids[l] == ways_with_name2[m] && ways_with_name1[k] != ways_with_name2[m]) {
                                      found_name2 = true;
                                      break; // Exit the loop once a match is found.
                                  }
                              }

                              if (found_name1 && found_name2) {
                                  break; // Exit the loop once a match is found.
                              }

                          }
                      }

                      if (found_name1 && found_name2) {
                          break; // Exit the loop once a match is found.
                      }

                  }

                  if (found_name1 && found_name2) {
                      bool_checker1 = found_name1;
                      bool_checker2 = found_name2;
                      break; // Exit the loop once both matches are found.
                  }

              }

              // Print the node ID if it's connected to ways containing both names.
              if (bool_checker1 && bool_checker2) {
                  printf("%d ", node_id);
              }

        }

        // Free the temporary array of way IDs.
        free(node_way_ids);

    }

    // Print a newline character to properly format the output.
    printf("\n");

}

/**
 * Converts from degree to radian
 *
 * @param deg The angle in degrees.
 * @return the equivalent value in radian
 */
#define d2r(deg) ((deg) * M_PI/180.)

/**
 * Calculates the distance between two nodes using the Haversine formula.
 *
 * @param x The first node.
 * @param y the second node.
 * @return the distance between two nodes, in kilometre.
 */
static double
distance_between_nodes(const struct node * x, const struct node * y) {
    double R = 6371.;
    double lat1 = x->lat;
    double lon1 = x->lon;
    double lat2 = y->lat;
    double lon2 = y->lon;
    double dlat = d2r(lat2-lat1);
    double dlon = d2r(lon2-lon1);
    double a = pow(sin(dlat/2), 2) + cos(d2r(lat1)) * cos(d2r(lat2)) * pow(sin(dlon/2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

/**
 * Calculates the total travel time for a specified path through nodes in the Simple Street Map (ssmap).
 *
 * @param m Pointer to the ssmap structure representing the map.
 * @param size The number of nodes in the path.
 * @param node_ids Array containing the IDs of the nodes in the path, in order.
 * @return The total travel time for traversing the path, in minutes, or -1.0 if an error occurs.
 *
 * This function verifies the validity of the specified path, including the existence of each node
 * and the connectivity between consecutive nodes as per the ways they are part of. It checks for
 * duplicate nodes, direct connectivity, and adherence to one-way restrictions. Upon validation,
 * it calculates the total travel time based on the distances between nodes and the speed limits
 * of the connecting ways. The travel time is summed up and returned in minutes.
 */
double
ssmap_path_travel_time(const struct ssmap * m, int size, int node_ids[size])
{
    // Preliminary checks for node existence.
    for (int i = 0; i < size; i++) {
        int id = node_ids[i];
        if (id < 0 || id >= m->num_nodes || m->nodes[id] == NULL) {
            printf("error: node %d does not exist.\n", id);
            return -1.0;
        }
    }

    // Check for duplicate nodes in the path.
    for (int i = 0; i < size; i++) {
        int count = 0;
        for (int j = 0; j < size; j++) {
            if (node_ids[j] == node_ids[i]) {
                count++;
            }
        }
        if (count > 1) {
            printf("error: node %d appeared more than once.\n", node_ids[i]);
            return -1.0;
        }
    }

    // Check for Way Object connectivity between adjacent nodes in node_ids.
    for (int i = 0; i < size - 1; i++) {
        bool error_three_bool = false;
        int current = node_ids[i];
        int next = node_ids[i + 1];

        struct node *node1 = m->nodes[current];
        struct node *node2 = m->nodes[next];
        for (int j = 0; j < node1->num_ways; j++) {
            for (int k = 0; k < node2->num_ways; k++) {
                if (node1->ways[j]->id == node2->ways[k]->id) {
                    error_three_bool = true;
                    break;
                }
            }
        }

        if (!error_three_bool) {
            printf("error: there are no roads between node %d and node %d.\n", current, next);
            return -1.0;
        }
    }

    // Check for adjacency of adjacent node ids in the Way Object connecting them.
    for (int i = 0; i < size - 1; i++) {
        bool error_four_bool = false;
        int current = node_ids[i];
        int next = node_ids[i + 1];

        struct node *node1 = m->nodes[current];
        struct node *node2 = m->nodes[next];
        for (int j = 0; j < node1->num_ways; j++) {
            for (int k = 0; k < node2->num_ways; k++) {
                if (node1->ways[j]->id == node2->ways[k]->id) {
                    for (int l = 0; l < node1->ways[j]->num_nodes - 1; l++) {
                        if ((node1->ways[j]->node_ids[l] == current && node1->ways[j]->node_ids[l + 1] == next) || (node1->ways[j]->node_ids[l] == next && node1->ways[j]->node_ids[l + 1] == current))  {
                            error_four_bool = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!error_four_bool) {
            printf("error: cannot go directly from node %d to node %d.\n", current, next);
            return -1.0;
        }
    }

    // Check for violation of one_way-ness of Way Objects.
    for (int i = 0; i < size - 1; i++) {
        bool error_five_bool = false;
        int current = node_ids[i];
        int next = node_ids[i + 1];

        struct node *node1 = m->nodes[current];
        struct node *node2 = m->nodes[next];
        for (int j = 0; j < node1->num_ways; j++) {
            for (int k = 0; k < node2->num_ways; k++) {
                if (node1->ways[j]->id == node2->ways[k]->id) {
                    for (int l = 0; l < node1->ways[j]->num_nodes - 1; l++) {
                        if (node1->ways[j]->one_way && node1->ways[j]->node_ids[l] == current && node1->ways[j]->node_ids[l + 1] == next) {
                            error_five_bool = true;
                            break;
                        } else if (!node1->ways[j]->one_way && ((node1->ways[j]->node_ids[l] == current && node1->ways[j]->node_ids[l + 1] == next) || (node1->ways[j]->node_ids[l] == next && node1->ways[j]->node_ids[l + 1] == current))) {
                              error_five_bool = true;
                              break;
                        }
                    }
                }
            }
        }

      if (!error_five_bool) {
          printf("error: cannot go in reverse from node %d to node %d.\n", current, next);
          return -1.0;
      }
    }

    // Accumulator for travel_time to be returned.
    double travel_time = 0.0;

    // Loop over all the indices of node_ids to calculate the sole way_id
    // connecting the each pair of Nodes
    for (int i = 0; i < size - 1; i++) {
        int current = node_ids[i];
        int next = node_ids[i + 1];
        struct node *node1 = m->nodes[current];
        struct node *node2 = m->nodes[next];
        int way_id = 0;
        for (int j = 0; j < node1->num_ways; j++) {
            for (int k = 0; k < node2->num_ways; k++) {
                if (node1->ways[j]->id == node2->ways[k]->id) {
                    for (int l = 0; l < node1->ways[j]->num_nodes - 1; l++) {
                        if (node1->ways[j]->one_way && node1->ways[j]->node_ids[l] == current && node1->ways[j]->node_ids[l + 1] == next) {
                            way_id = node1->ways[j]->id;
                            break;
                        } else if (!node1->ways[j]->one_way && ((node1->ways[j]->node_ids[l] == current && node1->ways[j]->node_ids[l + 1] == next) || (node1->ways[j]->node_ids[l] == next && node1->ways[j]->node_ids[l + 1] == current))) {
                              way_id = node1->ways[j]->id;
                              break;
                        }
                    }
                }
            }
        }

        // Getting the Way object for the pair of nodes.
        struct way *way = m->ways[way_id];

        // The Max speed limit of the Way Object.
        float speed = way->max_speed;

        // The distance between the nodes, calculated using the Haversine Formula.
        double distance = distance_between_nodes(node1, node2);

        // The Time taken for each pair of nodes added to the existing travel_time.
        travel_time = travel_time + (distance / speed);

    }

    // Return the travel time in minutes.
    return travel_time * 60;

}

/**
 * Generates and prints a path from a start node to an end node within the Simple Street Map (ssmap).
 *
 * @param m Pointer to the ssmap structure representing the map.
 * @param start_id The unique identifier of the starting node.
 * @param end_id The unique identifier of the ending node.
 *
 * This function implements Dijkstra's algorithm to find the shortest path from the start node to the
 * end node based on the distances between connected nodes. It initializes necessary structures for
 * tracking distances, visited nodes, and parent nodes to reconstruct the path. The function then
 * iterates through the map, updating distances and parents until it processes all reachable nodes or
 * finds the shortest path to the end node. Finally, it reconstructs and prints the path from the
 * end node back to the start node.
 */
void
ssmap_path_create(const struct ssmap * m, int start_id, int end_id)
{
    // Validate start and end node existence.
    if (start_id < 0 || start_id >= m->num_nodes || m->nodes[start_id] == NULL) {
        printf("error: node %d does not exist.\n", start_id);
        return;
    }

    if (end_id < 0 || end_id >= m->num_nodes || m->nodes[end_id] == NULL) {
        printf("error: node %d does not exist.\n", end_id);
        return;
    }

    // Handle the case where the start and end nodes are the same.
    if (start_id == end_id) {
        printf("%d %d\n", start_id, end_id);
        return;
    }

    // Initialize arrays for distances, visited flags, and parent node IDs.
    double dist[m->num_nodes];
    bool visited[m->num_nodes];
    int parent[m->num_nodes];

    // Initialize distances as infinite, visited as false, and parent as -1.
    for (int i = 0; i < m->num_nodes; ++i) {
        dist[i] = INFINITY;
        visited[i] = false;
        parent[i] = -1;
    }

    // Initialize the priority queue (min_heap) and set the start node's distance to 0.
    min_heap *min_heap = create_min_heap(m->num_nodes);
    insert_min_heap(min_heap, start_id, 0.0);
    dist[start_id] = 0.0;

    // Dijkstra's algorithm main loop.
    while (min_heap->size > 0) {
        heap_node min_heap_node = extract_min(min_heap); // Extract the node with minimum distance.
        int u = min_heap_node.node_id;
        visited[u] = true; // Mark the node as visited.

        // If the end node is reached, exit the loop.
        if (u == end_id) {
            break;
        }

        // Explore all ways connected to the current node.
        for (int i = 0; i < m->nodes[u]->num_ways; ++i) {
            struct way *way = m->nodes[u]->ways[i];

            int node_index = find_node_index_in_way(way, u);

            // Explore nodes adjacent to the current node within the way.
            for (int offset = -1; offset <= 1; offset += 2) { // Checks both directions.
                int next_node_index = node_index + offset;

                if (next_node_index >= 0 && next_node_index < way->num_nodes) {
                    int v = way->node_ids[next_node_index];

                    // Skip reverse traversal on one-way ways.
                    if (way->one_way && offset < 0) {
                        continue;
                    }

                    // Update the distance if a shorter path is found.
                    if (!visited[v] && dist[u] != INFINITY) {
                        double alt = dist[u] + distance_between_nodes(m->nodes[u], m->nodes[v]) / way->max_speed * 60;

                        if (alt < dist[v]) {
                            dist[v] = alt;
                            parent[v] = u;
                            // Add or update the node in the min_heap.
                            if (!is_in_min_heap(min_heap, v)) {
                              insert_min_heap(min_heap, v, alt);
                            } else {
                              decrease_key(min_heap, v, alt);
                            }
                        }
                    }
                }
            }
        }
    }

    // Path reconstruction
    if (parent[end_id] == -1) {
    } else {
        int stack[m->num_nodes], top = -1;
        // Reconstruct the path by tracing parent nodes from the end node to the start node.
        for (int at = end_id; at != -1; at = parent[at]) {
            stack[++top] = at;
        }
        // Print the path in reverse order (start to end).
        while (top >= 0) {
            printf("%d ", stack[top--]);
        }
        printf("\n");
    }

    free_min_heap(min_heap);
}
