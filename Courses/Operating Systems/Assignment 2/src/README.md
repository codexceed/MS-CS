# OS Lab 2 - Scheduler

## Instructions
1. Load the `gcc-11.2` module as the makefile doesn't seem to execute the same correctly.
   ```bash
   module load gcc-11.2
    ```
2. Check if `gcc-11.2` (look for version 11.2.0) is loaded. Try reconnecting to the server and repeating the steps if the module fails to load.
   ```bash
   gcc --version
   gcc (GCC) 11.2.0
   Copyright (C) 2021 Free Software Foundation, Inc.
   This is free software; see the source for copying conditions.  There is NO
   warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   ```
3. Run makefile.
   ```bash
   make
    ```
4. Run the evaluation scripts against the `scheduler` executable.
   