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
This sets up the initial state of the DB with some default data for grople syrup production.
> **Note**: Setup requires row insertions which need to be checked against table integrity constraints. Due to this, the setup can take a few minutes to finish. You will see the insertion logs output on the console.
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
> **Note**: The training is done on an ExtraTreesRegressor model which take less than 10 mins to complete.
### Predicting grople syrup production
```shell
$ python execute_inference.py
Grople Production Predictor
Enter model filename (enter nothing for latest model):
Please enter a valid path to your input files:csvs
Name of file containing precipitation data (enter nothing for default name):
Name of file containing moisture data (enter nothing for default name):
Name of file containing temperature data (enter nothing for default name):
Name of file containing NDVI data (enter nothing for default name):
Name of file containing production input (enter nothing for default name):
Please enter a valid csv prediction output file path:predictions/out1.csv
C:\Users\perfe\PycharmProjects\test\MS-CS\Courses\Database Systems\Final Project\venv\lib\site-packages\pandas\core\algorithms.py:798: FutureWarning: In a future version, the Index constructor will not infer numeric dtypes when pas
sed object-dtype sequences (matching Series behavior)
  uniques = Index(uniques)
```
- In order to perform inference, we need to provide input data for all production factors (precipitation, temperature, moisture, etc) and the target months and region. This is done via csv files for each of the prediction factors, all of which need to be located at the same path. In this case, the `csvs` directory already contains some testing input so we use the same.
- The file names for input data can be ignored if we're using the default `csvs` input path.
- The output is in the form of a csv, with production values populated alongside the months and regions specified in the input. Please specify the exact full path to the output csv file, including the file name.

The output looks like as follows
```shell
$ head -10 predictions/out1.csv
start_date,end_date,prod,region_id
2021-01-01T00:00:00.000Z,2021-01-31T00:00:00.000Z,198359.16317463852,93
2021-02-01T00:00:00.000Z,2021-02-28T00:00:00.000Z,222003.8052636264,93
2021-03-01T00:00:00.000Z,2021-03-31T00:00:00.000Z,244018.29270226147,93
2021-04-01T00:00:00.000Z,2021-04-30T00:00:00.000Z,272363.70633147896,93
2021-05-01T00:00:00.000Z,2021-05-31T00:00:00.000Z,260506.67135483894,93
2021-06-01T00:00:00.000Z,2021-06-30T00:00:00.000Z,311105.72231182834,93
2021-07-01T00:00:00.000Z,2021-07-31T00:00:00.000Z,300329.87567741884,93
2021-08-01T00:00:00.000Z,2021-08-31T00:00:00.000Z,300353.6891397853,93
2021-09-01T00:00:00.000Z,2021-09-30T00:00:00.000Z,297747.5853440855,93
```

## Modeling
There are two notebooks in the `ml` directory,
- **Grople Syrup DB to ML.ipynb**: Contains the initial modeling process metrics and figures which were done using csv files as input. 
- **Grople Syrup DB to ML - SQL Joins.ipynb**: Contains the refined training and inference code using data directly sourced from SQLite tables.

## Future Goals
- Create a single interaction CLI or microservice for interacting with all the current scripts, effectively turning them into interactive functionalities
- Create a web-based GUI
