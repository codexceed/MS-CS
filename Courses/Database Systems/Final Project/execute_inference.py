#!/usr/bin/env python
from datetime import timedelta
from pathlib import Path

import pandas as pd
from joblib import load

from constants.env import MODEL_PATH
from utils.data import preprocess_grople_inp

# Prompt inputs
print("Grople Production Predictor")
model_filename = input("Enter model filename (enter nothing for latest model):")
data_path = Path(input("Please enter a valid path to your input files:"))
if not model_filename:
    model_path = sorted(list(MODEL_PATH.glob("*.gz")), reverse=True)[0]
else:
    model_path = MODEL_PATH / model_filename
precip_filename = (
    input(
        "Name of file containing precipitation data (enter nothing for default name):"
    )
    or "Daily Precipitation.csv"
)
mos_filename = (
    input("Name of file containing moisture data (enter nothing for default name):")
    or "Daily Soil Moisture.csv"
)
temperature_filename = (
    input("Name of file containing temperature data (enter nothing for default name):")
    or "Daily Temperature.csv"
)
ndvi_filename = (
    input("Name of file containing NDVI data (enter nothing for default name):")
    or "Eight Day NDVI.csv"
)
production_inp = (
    input("Name of file containing production input (enter nothing for default name):")
    or "predicted_production_qty.csv"
)

output_path = None
while output_path is None:
    output_path = Path(
        input("Please enter a valid csv prediction output file path:") or None
    )

# Load data
pred_production = pd.read_csv(data_path / production_inp)
prediction_inp = pred_production.copy(deep=True)
prediction_inp["start_date"] = pd.to_datetime(
    prediction_inp["start_date"]
).dt.tz_localize(None)
prediction_inp["end_date"] = pd.to_datetime(prediction_inp["end_date"]).dt.tz_localize(
    None
)
prediction_inp["m_days"] = (
    prediction_inp["end_date"] - prediction_inp["start_date"]
).dt.days + 1
prediction_inp = prediction_inp.drop(columns="end_date")

# Join other features
# Load NDVI values
precipitation = pd.read_csv(data_path / precip_filename)
moisture = pd.read_csv(data_path / mos_filename)
temperature = pd.read_csv(data_path / temperature_filename)
ndvi = pd.read_csv(data_path / ndvi_filename)
ndvi.rename(columns={"ndvi": "index"}, inplace=True)

# Change dates type to pandas datetime
precipitation["start_date"] = pd.to_datetime(
    precipitation["start_date"]
).dt.tz_localize(None)
moisture["start_date"] = pd.to_datetime(moisture["start_date"]).dt.tz_localize(None)
temperature["start_date"] = pd.to_datetime(temperature["start_date"]).dt.tz_localize(
    None
)
ndvi["start_date"] = pd.to_datetime(ndvi["start_date"]).dt.tz_localize(None)

precipitation.drop(columns="end_date", inplace=True)
moisture.drop(columns="end_date", inplace=True)
temperature.drop(columns="end_date", inplace=True)

joint_df = pd.merge(
    precipitation,
    moisture,
    how="left",
    left_on=["start_date", "region_id"],
    right_on=["start_date", "region_id"],
)
joint_df = pd.merge(
    joint_df,
    temperature,
    how="left",
    left_on=["start_date", "region_id"],
    right_on=["start_date", "region_id"],
)

joint_df = joint_df.sort_values(["region_id", "start_date"])
joint_df = joint_df[joint_df["smos"].notna()]
joint_df.fillna(method="ffill", inplace=True)
joint_df.fillna(method="bfill", inplace=True)
joint_df.info()

cols = list(ndvi.columns) + ["curr_date"]
merge_ndvi = pd.DataFrame(columns=cols)
for _, row in ndvi.iterrows():
    new_row = {k: [row[k] for _ in range(8)] for k in row.index}
    for i in range(8):
        curr_dates = new_row.get("curr_date", [])
        curr_dates.append(new_row["start_date"][0] + timedelta(days=i))
        new_row["curr_date"] = curr_dates
    new_row = pd.DataFrame(new_row)
    merge_ndvi = pd.concat([merge_ndvi, new_row], ignore_index=True)

merge_ndvi["curr_date"] = pd.to_datetime(merge_ndvi["curr_date"]).dt.tz_localize(None)
merge_ndvi = merge_ndvi.drop(columns="start_date")

# Join NDVI to others
joint_ndvi_df = pd.merge(
    joint_df,
    merge_ndvi,
    how="left",
    left_on=["start_date", "region_id"],
    right_on=["curr_date", "region_id"],
)
joint_ndvi_df = joint_ndvi_df.drop(columns="curr_date")

# Populate days for input months
cols = list(prediction_inp.columns) + ["curr_date"]
prediction_days = pd.DataFrame(columns=cols)
for _, row in prediction_inp.iterrows():
    m_days = row["m_days"]
    new_row = {k: [row[k] for _ in range(m_days)] for k in row.index}
    for i in range(m_days):
        curr_dates = new_row.get("curr_date", [])
        curr_dates.append(new_row["start_date"][0] + timedelta(days=i))
        new_row["curr_date"] = curr_dates

    new_row = pd.DataFrame(new_row)
    prediction_days = pd.concat([prediction_days, new_row], ignore_index=True)

prediction_days.drop(columns=["start_date", "m_days"], inplace=True)
prediction_days["curr_date"] = pd.to_datetime(
    prediction_days["curr_date"]
).dt.tz_localize(None)

# Join Production to others
joint_pred = pd.merge(
    prediction_days,
    joint_ndvi_df,
    how="left",
    left_on=["curr_date", "region_id"],
    right_on=["start_date", "region_id"],
)
joint_pred["index"] = pd.to_numeric(joint_pred["index"])
joint_pred.drop(columns=["start_date", "end_date"], inplace=True)

# Handle Categorical Data, Explode Date into Features, Impute Missing Values
# One-hot Encoding
encoded_pred = pd.merge(
    joint_pred,
    pd.get_dummies(joint_pred["region_id"], prefix="region_id_"),
    left_index=True,
    right_index=True,
)
encoded_pred = encoded_pred.sort_values(["region_id", "curr_date"])
encoded_pred = encoded_pred.drop(columns="region_id")

# Date to features
final_pred = encoded_pred
final_pred["year"] = encoded_pred["curr_date"].dt.year
final_pred["month"] = encoded_pred["curr_date"].dt.month
final_pred["day"] = encoded_pred["curr_date"].dt.day
final_pred = final_pred.drop(columns="curr_date")

# # Impute values
final_pred.fillna(method="ffill", inplace=True)
final_pred.fillna(method="bfill", inplace=True)

# Inference
model = load(model_path)
X_pred = preprocess_grople_inp(final_pred, normalize=False)
y_pred = model.predict(X_pred)


for i, row in pred_production.iterrows():
    month = pd.to_datetime(row["start_date"]).month
    rid = row["region_id"]
    prod_vals = y_pred[
        (final_pred[f"region_id__{rid}"] == 1) & (final_pred.month == month)
    ]
    pred_production.loc[i, "prod"] = sum(prod_vals)

output_path.parent.mkdir(exist_ok=True)
pred_production.to_csv(output_path, index=False)
