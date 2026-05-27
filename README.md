# NITCbase - Relational Database Management System

## Overview
NITCbase is a single-user Relational Database Management System (RDBMS) built entirely from scratch in C++. This project serves as a comprehensive implementation of database internals, spanning from low-level disk storage interfaces to high-level query processing and relational algebra execution.

This repository contains the complete implementation of all 12 stages of the NITCbase roadmap, supporting core relational operations including selection, projection, B+ Tree indexing, and equi-joins.

## Key Features
* **Custom File System (XFS):** Disk block emulation and management using a simulated disk interface.
* **Data Types:** Robust support for `NUMBER` and `STRING` attributes.
* **Buffer & Cache Management:** In-memory caching for active relations, attributes, and disk blocks to drastically minimize I/O overhead.
* **B+ Tree Indexing:** Efficient search, insertion, and deletion using multi-level B+ tree structures (internal and leaf nodes).
* **Equi-Join:** Support for joining relations via both nested loop and indexed join algorithms.
* **DDL & DML Operations:** Complete support for creating/deleting relations, inserting records, and dynamically creating/dropping indexes.

### Custom Enhancements
In addition to the standard NITCbase specifications, this project implements enhanced output flexibility for relational algebra queries (`SELECT`, `PROJECT`, `JOIN`):
* **Terminal Output:** Query results can be dynamically printed directly to the terminal for immediate viewing and debugging.
* **Target Table Redirection:** Output can optionally be written into a newly instantiated target relation instead of the terminal. This allows for complex query chaining and persistent data manipulation.

---

## Architecture & Layered Implementation
The codebase is structured into a modular, multi-layered architecture where each layer abstracts the complexity of the one below it:

1. **Physical Layer:** Emulates a disk interface managing 2KB blocks.
2. **Buffer Layer:** Handles fetching disk blocks into memory, managing dirty bits, and writing blocks back to the disk.
3. **Cache Layer:** Maintains in-memory structures (`RelCache` and `AttrCache`) for open relations and their respective schema definitions.
4. **B+ Tree Layer:** Manages B+ Tree indexing operations (`bPlusSearch`, `bPlusInsert`, `bPlusCreate`, `bPlusDestroy`) for `O(log n)` lookups.
5. **Block Access Layer:** Acts as a bridge between high-level logic and raw data blocks. Implements linear search, block-level insertion, and deletion.
6. **Schema Layer:** Handles Data Definition Language (DDL) queries (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`, `DROP INDEX`, `RENAME`).
7. **Algebra Layer:** Handles Data Manipulation Language (DML) operations and relational algebra computations.
8. **Frontend Interface:** Parses user commands and triggers the appropriate Schema or Algebra layer functions.

---

## Development Roadmap (Completed up to Stage 12)
This project systematically implements the database internals over 12 distinct stages:

* **Stage 1: Disk Interface** - Understanding the raw disk layout and emulation.
* **Stage 2: Record Blocks & Catalogs** - Designing the Relation and Attribute Catalogs to persistently store metadata.
* **Stage 3: Buffer & Cache Management** - Setting up the Disk Buffer array and catalog caches for fast access.
* **Stage 4: Linear Search** - Fetching specific records using brute-force block traversal.
* **Stage 5: Opening Relations** - Managing the Open Relation Table and allocating cache entries for active queries.
* **Stage 6: Disk Write-back** - Implementing dirty bit logic and committing modified buffer blocks back to the disk.
* **Stage 7: Record Insertion** - Finding free slots via slot maps and inserting new tuples into data blocks.
* **Stage 8: Relation Management** - Implementing `CREATE TABLE` and `DROP TABLE` functionality.
* **Stage 9: Selection & Projection** - Implementing fundamental relational algebra queries.
* **Stage 10: B+ Tree Search** - Introducing B+ Tree data structures for optimized index lookups.
* **Stage 11: Index Creation & Deletion** - Building and destroying B+ Trees dynamically for specified attributes.
* **Stage 12: Equi-Join on Relations** - Combining two relations based on an equality condition, utilizing B+ Trees to reduce time complexity from `O(m*n)` to `O(m*log n + n)`.

---

### Building and Running
## Getting Started

### Prerequisites
** **Operating System:** Linux
* **Compiler:** GNU `g++` (supports C++11 or higher)
* **Build Tool:** `make` utility

### Build and Execution
Follow these steps to set up the required directory structure, compile the database engine, and initialize the simulated file system.

1. **Clone the repository and navigate into the project:**
   ```bash
   git clone [https://github.com/X-0606/mynitcbase.git](https://github.com/X-0606/mynitcbase.git)
   cd mynitcbase
   ```
2. ** Create the folder environment **
  ```bash
mkdir Disk Files
cd Files
mkdir Batch_Execution_Files Input_Files Output_Files
cd ..
```
3. ** Compile the source code **
   ``` bash
   make
   ```
4. ** Run the binary file **
   ``` bash
   ./nitcbase
   ```
Now you should be inside the interactive prompt. Inside that run 
``` bash
#fdisk
```
To format the disk.

 
  
