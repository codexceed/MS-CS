#!/usr/bin/env python
from datetime import timedelta

import pandas as pd
from sqlalchemy import create_engine
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import sessionmaker

from constants.env import DB_PATH
from db.schema import (
    NDVI,
    Base,
    DataPoints,
    Moisture,
    Precipitation,
    Production,
    Temperature,
)


def safe_insert(df, schema_class, Session):
    with Session() as session:
        for _, row in df.iterrows():
            try:
                session.add(schema_class(**row))
                session.commit()
            except IntegrityError as e:
                session.rollback()
                print(f"Insertion exeption: {e}\nIgnoring insertion.")


if __name__ == "__main__":
    engine = create_engine(f"sqlite:///{DB_PATH}", echo=True)
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)

    # Load csvs
    production_df = pd.read_csv("csvs/Production Quantity.csv")
    ndvi_df = pd.read_csv("csvs/Eight Day NDVI.csv")
    moisture_df = pd.read_csv("csvs/Daily Soil Moisture.csv")
    temperature_df = pd.read_csv("csvs/Daily Temperature.csv")
    precipitation_df = pd.read_csv("csvs/Daily Precipitation.csv")
    all_dfs = [production_df, ndvi_df, moisture_df, temperature_df, precipitation_df]

    # Process data
    for df in all_dfs:
        df["start_date"] = pd.to_datetime(df["start_date"])
        df["end_date"] = pd.to_datetime(df["end_date"])

    ndvi_df = ndvi_df.rename(columns={"ndvi": "index"})

    # Generate data points for all dates
    data_points_df = pd.DataFrame(columns=["region_id", "date", "point_id"])
    for idx, row in production_df[["region_id", "start_date", "end_date"]].iterrows():
        m_days = (row["end_date"] - row["start_date"]).days + 1
        monthly_rows = {
            "region_id": [row["region_id"] for _ in range(m_days)],
            "point_id": [idx for _ in range(m_days)],
            "date": [row["start_date"] + timedelta(days=i) for i in range(m_days)],
        }
        monthly_rows = pd.DataFrame(monthly_rows)
        data_points_df = pd.concat([data_points_df, monthly_rows], ignore_index=True)

    # Insert rows into tables
    production_df.to_sql(
        Production.__tablename__, engine, if_exists="append", index_label="point_id"
    )
    data_points_df.to_sql(
        DataPoints.__tablename__, engine, if_exists="append", index=False
    )

    safe_insert(ndvi_df, NDVI, Session)
    safe_insert(moisture_df, Moisture, Session)
    safe_insert(temperature_df, Temperature, Session)
    safe_insert(precipitation_df, Precipitation, Session)
