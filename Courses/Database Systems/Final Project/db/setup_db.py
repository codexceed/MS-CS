from datetime import timedelta

import pandas as pd
from sqlalchemy.orm import sessionmaker

from schema import Base, Production, DataPoints, NDVI, Moisture, Temperature, Precipitation
from sqlalchemy import create_engine


if __name__ == "__main__":
    engine = create_engine("sqlite:///grople.db", echo=True)
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)

    # Load csvs
    production_df = pd.read_csv("../csvs/Production Quantity.csv")
    ndvi_df = pd.read_csv("../csvs/Eight Day NDVI.csv")
    moisture_df = pd.read_csv("../csvs/Daily Soil Moisture.csv")
    temperature_df  = pd.read_csv("../csvs/Daily Temperature.csv")
    precipitation_df = pd.read_csv("../csvs/Daily Precipitation.csv")
    all_dfs = [production_df, ndvi_df, moisture_df, temperature_df, precipitation_df]

    # Process data
    for df in all_dfs:
        df["start_date"] = pd.to_datetime(df["start_date"])
        df["end_date"] = pd.to_datetime(df["end_date"])

    ndvi_df = ndvi_df.rename(columns={"ndvi": "index"})

    # Generate data points for all dates
    data_points_df = pd.DataFrame(columns=["region_id", "date", "point_id"])
    for idx, row in production_df[["region_id", "start_date"]].iterrows():
        monthly_rows = {"region_id": [row["region_id"] for _ in range(row.start_date.days_in_month)],
                        "point_id": [idx for _ in range(row.start_date.days_in_month)],
                        "date": [row["start_date"] + timedelta(days=i) for i in range(row.start_date.days_in_month)]}
        monthly_rows = pd.DataFrame(monthly_rows)
        data_points_df = pd.concat([data_points_df, monthly_rows], ignore_index=True)

    # Insert rows into tables
    production_df.to_sql(Production.__tablename__, engine, if_exists="append", index_label="point_id")
    data_points_df.to_sql(DataPoints.__tablename__, engine, if_exists="append", index=False)
    ndvi_df.to_sql(NDVI.__tablename__, engine, if_exists="append", index=False)
    moisture_df.to_sql(Moisture.__tablename__, engine, if_exists="append", index=False)
    temperature_df.to_sql(Temperature.__tablename__, engine, if_exists="append", index=False)
    precipitation_df.to_sql(Precipitation.__tablename__, engine, if_exists="append", index=False)

