# Database Systems Final Project
## Use Case: Grople Syrup Production
We have been given the monthly production quantity for a certain agricultural 
product (let’s call it Grople syrup, note - no relation to actual Maple Syrup) 
in 10 different provinces of a country between January 2015 to December 2020. 
This Grople syrup comes from a fruit. It takes a few months for the fruits to 
grow on the trees which bear them. It also takes a few days to extract the syrup 
from the fruits after they have been harvested.

## Setup
- Ensure Python 3.8 or higher is installed
- Install python dependencies
    ```shell
    pip install requirements.txt
    ```

## Usage
### Setting up the Grople DB with initial data
```shell
python setup_db.py
```
### Execute queries on the Grople DB
Use the `execute_query.py` script to perform SQL queries on Grople DB like as follows
```shell
python execute_query.py -a insert -t production
```
Enter values as per prompts that follow
```shell
For insert action on table production, please provide the following arguments:
region_id=120
start_date('dd/mm/yyy' HH:MM:SS)=04/11/2022
end_date('dd/mm/yyy' HH:MM:SS)=10/11/2022
prod=134589
point_id=112
```
> **Note**: You can pass empty value for `point_id` while inserting into `production` table as it will default to autoincrement.

You can use the `-h` option to see detailed help on the CLI.
```shell
$ python execute_query.py -h
usage: execute_query.py [-h] -a {insert,delete,update} -t {production,ndvi,moisture,temperature,precipitation}

Perform queries on the grople DB.

optional arguments:
  -h, --help            show this help message and exit
  -a {insert,delete,update}, --action {insert,delete,update}
                        Type of query
  -t {production,ndvi,moisture,temperature,precipitation}, --table {production,ndvi,moisture,temperature,precipitation}
                        Target table
```

### Training ML model
```shell
$ python execute_ml.py
Querying DB for joint production data.
Performing feature engineering and preprocessing.
Initializing training and evaluation.
Persisting trained model named grople_model_1662276154.252166.gz to models directory
Score: 0.9999939209975244
MSE: 135.66202606659385
```
- The latest model is automatically persists in the `models` directory with a timestamp in its name.
- All the data in the grople database is used for the modeling process, and it's 10% of the data is reserved for evaluation.
### Predicting grople syrup production
