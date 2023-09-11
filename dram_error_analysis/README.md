# Project Instructions
## Prerequisite
Need to setup the environment using conda-requirements

    
    conda create --name dram_error --file conda-requirements.txt  
    conda activate dram_error
   
This project involves running several processes to achieve the desired output. There are two main options to follow:

## Option 1: Direct Process
1. Execute `process_1.py`:
    ```bash
    python process_1.py
    ```
2. Once completed, run `process_4.py`:
    ```bash
    python process_4.py
    ```

## Option 2: Intermediate Processes with Manual Intervention
1. Start by executing `process_1.py`:
    ```bash
    python process_1.py
    ```
2. Once completed, run `process_2.py`:
    ```bash
    python process_2.py
    ```

3. **Manual Step**:
    - At this point, you can manually modify the categories.
    - Navigate to the generated directory structure, which follows the pattern `{category}/{sub_category}/{manufacture}`.
    - Modify the `{category}` if you want to change the main classification.
    - Adjust the `{sub_category}` for a more detailed explanation or classification of each pattern.
   
   *Note*: The intermediate data from `process_1` will be stored in `category.pkl`. This file can be used for any modifications.
   
4. After making any necessary modifications, run `process_3.py`:
    ```bash
    python process_3.py
    ```
5. Once completed, execute `process_4.py`:
    ```bash
    python process_4.py
    ```
