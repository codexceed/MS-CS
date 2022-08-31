import pandas as pd
from sqlalchemy.orm import sessionmaker

from schema import Base, Production
from sqlalchemy import create_engine


if __name__ == "__main__":
    engine = create_engine("sqlite:///grople.db", echo=True)
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)

    # Load csvs
    production_df = pd.read_csv("../csvs/Production Quantity.csv")
    production_df["start_date"] = pd.to_datetime(production_df["start_date"])
    production_df["end_date"] = pd.to_datetime(production_df["end_date"])

    # Insert rows into tables
    production_df.to_sql(Production.__tablename__, engine, if_exists="append", index_label="point_id")
