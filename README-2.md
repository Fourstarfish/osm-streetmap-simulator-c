# Street Map Simulation (OSM-Based)

A high-performance C program that loads, models, and navigates OpenStreetMap (OSM)-style street data. Implements a custom graph data structure, Dijkstra’s shortest-path algorithm, and a from-scratch min-heap priority queue to efficiently compute fastest routes in real time.

---

## Table of Contents

- [Project Overview](#project-overview)  
- [Features](#features)  
- [Prerequisites](#prerequisites)  
- [Building & Installation](#building--installation)  
- [Data Format](#data-format)  
- [Usage](#usage)  
- [Code Structure](#code-structure)  
- [Performance](#performance)  
- [Contributing](#contributing)  
- [License](#license)  

---

## Project Overview

This project simulates a city-scale street network drawn from OSM “Simple Street Map” text dumps. It:

1. **Parses** ways and nodes into a compact in-memory graph (`ssmap`).
2. **Indexes** nodes ↔ ways relationships for quick neighbor look-ups.
3. **Computes** fastest travel time between any two nodes using Dijkstra’s algorithm.
4. **Manages** frontier nodes with a custom min-heap (priority queue) in O(log N).

---

## Features

- **Graph Loading & Validation**  
  - Reads `Simple Street Map` files (`*.txt`) containing “ways” (roads) and “nodes” (intersections).  
  - Validates format, prints helpful error messages on parse or data errors.

- **Custom Data Structure**  
  - `ssmap` holds arrays of `node` and `way` structs, with dynamic memory allocation sized to file metadata.

- **Shortest-Path Routing**  
  - Implements Dijkstra’s algorithm to compute minimum-time paths.  
  - Travel time = distance / speed limit (km/hr) → minutes.

- **Priority Queue**  
  - From-scratch min-heap of `(node_id, distance)` pairs supporting decrease‐key in O(log N).

- **Query & Report**  
  - Print node or way details by ID.  
  - Search roads by keyword.  
  - Compute and display path travel time or full route.

---

## Prerequisites

- **Language:** C99 compatible compiler (e.g. `gcc`, `clang`).  
- **Build Tools:** GNU Make (optional).  
- **Memory:** Sufficient RAM for graph (depends on map size).

---

## Building & Installation

1. **Clone Repository**  
   ```bash
   git clone https://github.com/your-username/street-map-sim.git
   cd street-map-sim
   ```

2. **Compile**  
   ```bash
   gcc -std=c99 -O2 -o streetmap main.c streets.c -lm
   ```  
   Or, if you include a `Makefile`:  
   ```bash
   make
   ```

3. **(Optional) Install**  
   ```bash
   sudo mv streetmap /usr/local/bin/
   ```

---

## Data Format

Input files must begin with:
```
Simple Street Map
<ways> ways
<nodes> nodes
```
Followed by blocks of:
```
way <id> <osm-id> <name>
 <speed> <normal|oneway> <num_nodes>
 <node_id_1> <node_id_2> … <node_id_k>
```
and
```
node <id> <osm-id> <lat> <lon> <num_ways>
 <way_id_1> <way_id_2> … <way_id_m>
```

See `uoft.txt` and `huntsville.txt` for examples.

---

## Usage

```bash
./streetmap <map_file.txt>
```

Once loaded, enter commands at the prompt:

- **Print a way:**  
  ```
  way <way_id>
  ```
- **Print a node:**  
  ```
  node <node_id>
  ```
- **Find roads by keyword:**  
  ```
  find way <keyword>
  ```
- **Find intersection nodes:**  
  ```
  find node <name1> [name2]
  ```
- **Compute travel time on a given path:**  
  ```
  travel <node_id1> <node_id2> … <node_idN>
  ```
- **Find quickest path between two nodes:**  
  ```
  route <start_id> <end_id>
  ```
- **Quit:**  
  ```
  quit
  ```

---

## Code Structure

```
├── main.c         # CLI, file parsing, command dispatch
├── streets.h      # ssmap, node, way & API definitions
├── streets.c      # Graph implemention, Dijkstra, min-heap
├── uoft.txt       # Example UofT map
├── huntsville.txt # Example Huntsville map
└── Makefile       # (optional) build rules
```

- **`ssmap_create/initialize/destroy`** — manage graph lifecycle  
- **`ssmap_add_way` / `ssmap_add_node`** — ingest map elements  
- **`ssmap_path_travel_time`** — verify path, compute travel minutes  
- **`ssmap_route`** — Dijkstra’s algorithm + min-heap for fastest route  

---

## Performance

- **Graph load:** O(N + E) memory, linear parse time.  
- **Dijkstra’s:** O((N + E) log N), where N = nodes, E = edges.  
- **Heap ops:** Insert/extract/decrease-key in O(log N).

With 2,000 nodes & 400 ways, route queries complete in under 50 ms on typical hardware.

---

## Contributing

1. Fork the repo  
2. Create a feature branch  
3. Commit & push  
4. Open a Pull Request  

Please include tests for new features and update this README accordingly.

---

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for full text.
