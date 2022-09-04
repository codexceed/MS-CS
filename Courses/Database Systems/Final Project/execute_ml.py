#!/usr/bin/env python
from datetime import datetime

import pandas as pd
from joblib import dump
from sklearn.ensemble import ExtraTreesRegressor
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import train_test_split
from sqlalchemy import create_engine

from constants.env import DB_PATH, MODEL_PATH

# Setup DB
from utils.data import preprocess_grople_inp

grople_db = create_engine(f"sqlite:///{DB_PATH}")

# Load Data
# Query joint data via the 'points' table
sql_query = (
    "select joint_features.*, production.start_date as prod_start, production.end_date as prod_end, production.prod "
    "from (select joint_precip.*, ndvi.'index' "
    "from (select joint_temp.*, precipitation.precip "
    "from (select joint_smos.*, temperature.temp "
    "from (select points.*, moisture.smos "
    "from points left outer join moisture on points.region_id=moisture.region_id and points.date=moisture.start_date) as joint_smos "
    "left outer join temperature on joint_smos.region_id=temperature.region_id and joint_smos.date=temperature.start_date) as joint_temp "
    "left outer join precipitation on joint_temp.region_id=precipitation.region_id and joint_temp.date=precipitation.start_date) as joint_precip "
    "left outer join ndvi on joint_precip.region_id=ndvi.region_id and joint_precip.date=ndvi.start_date) as joint_features "
    "inner join production on joint_features.point_id=production.point_id;"
)
print("Querying DB for joint production data.")
joint_mos_temp_prec_df = pd.read_sql_query(sql_query, con=grople_db)

# Feature Engineering
# Change dates type to pandas datetime
print("Performing feature engineering and preprocessing.")
joint_mos_temp_prec_df["date"] = pd.to_datetime(joint_mos_temp_prec_df["date"])
joint_mos_temp_prec_df["prod_start"] = pd.to_datetime(
    joint_mos_temp_prec_df["prod_start"]
)
joint_mos_temp_prec_df["prod_end"] = pd.to_datetime(joint_mos_temp_prec_df["prod_end"])

# Impute missing values
joint_mos_temp_prec_df = joint_mos_temp_prec_df.sort_values(["region_id", "date"])
joint_mos_temp_prec_df = joint_mos_temp_prec_df[joint_mos_temp_prec_df["smos"].notna()]
joint_mos_temp_prec_df.fillna(method="ffill", inplace=True)
joint_mos_temp_prec_df.fillna(method="bfill", inplace=True)

# Adjust production value for each day
joint_mos_temp_prec_df["m_days"] = (
    joint_mos_temp_prec_df["prod_end"] - joint_mos_temp_prec_df["prod_start"]
).dt.days + 1
joint_mos_temp_prec_df = joint_mos_temp_prec_df.drop(columns="prod_end")
joint_mos_temp_prec_df["prod"] = (
    joint_mos_temp_prec_df["prod"] / joint_mos_temp_prec_df["m_days"]
)

# Drop unnecessary columns
joint_final = joint_mos_temp_prec_df.copy()
joint_final.drop(columns=["point_id", "prod_start", "m_days"], inplace=True)

# Handle Categorical Data
encoded_df = pd.merge(
    joint_final,
    pd.get_dummies(joint_final["region_id"], prefix="region_id_"),
    left_index=True,
    right_index=True,
)
encoded_df = encoded_df.drop(columns="region_id")

# Handle Dates
final_df = encoded_df.copy()
final_df["year"] = encoded_df["date"].dt.year
final_df["month"] = encoded_df["date"].dt.month
final_df["day"] = encoded_df["date"].dt.day
final_df = final_df.drop(columns="date")

# Preprocessing
inputs_df, target_df = final_df.loc[:, final_df.columns != "prod"], final_df["prod"]


# Normalize only columns corresponding to `precip`, `smos`, `temp` and `ndvi`.
inputs = preprocess_grople_inp(inputs_df)

print("Initializing training and evaluation.")
X, y = inputs, target_df.values
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.1)

ex_tree = ExtraTreesRegressor(criterion="absolute_error")
ex_tree.fit(X_train, y_train)

model_name = f"grople_model_{datetime.utcnow().timestamp()}.gz"
print(f"Persisting trained model named {model_name} to {MODEL_PATH}")
MODEL_PATH.mkdir(exist_ok=True)
dump(ex_tree, f"{MODEL_PATH}/{model_name}")
print(f"Score: {ex_tree.score(X_test, y_test)}")
y_pred = ex_tree.predict(X_test)
print(f"MSE: {mean_squared_error(y_test, y_pred)}")
