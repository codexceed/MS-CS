from sqlalchemy import (
    Column,
    DateTime,
    Float,
    ForeignKey,
    ForeignKeyConstraint,
    Integer,
    create_engine,
    event,
)
from sqlalchemy.engine import Engine
from sqlalchemy.orm import declarative_base

from constants.env import DB_PATH

Base = declarative_base()


@event.listens_for(Engine, "connect")
def set_sqlite_pragma(dbapi_connection, connection_record):
    cursor = dbapi_connection.cursor()
    cursor.execute("PRAGMA foreign_keys=ON")
    cursor.close()


def default_point():
    engine = create_engine(f"sqlite:///{DB_PATH}")
    with engine.connect() as con:
        return con.execute("select max(point_id) from production;").all()[0][0] + 1


class Production(Base):
    __tablename__ = "production"

    region_id = Column(Integer, primary_key=True)
    start_date = Column(DateTime, primary_key=True)
    end_date = Column(DateTime)
    prod = Column(Integer)
    point_id = Column(Integer, default=default_point, unique=True, nullable=False)


class DataPoints(Base):
    __tablename__ = "points"

    region_id = Column(Integer, primary_key=True)
    date = Column(DateTime, primary_key=True)
    point_id = Column(Integer, ForeignKey("production.point_id"))


class NDVI(Base):
    __tablename__ = "ndvi"

    region_id = Column(Integer, primary_key=True)
    start_date = Column(DateTime, primary_key=True)
    end_date = Column(DateTime)
    index = Column(Float)
    __table_args__ = (
        ForeignKeyConstraint(
            ["region_id", "start_date"], ["points.region_id", "points.date"]
        ),
    )


class Moisture(Base):
    __tablename__ = "moisture"

    region_id = Column(Integer, primary_key=True)
    start_date = Column(DateTime, primary_key=True)
    end_date = Column(DateTime)
    smos = Column(Float)

    __table_args__ = (
        ForeignKeyConstraint(
            ["region_id", "start_date"], ["points.region_id", "points.date"]
        ),
    )


class Temperature(Base):
    __tablename__ = "temperature"

    region_id = Column(Integer, primary_key=True)
    start_date = Column(DateTime, primary_key=True)
    end_date = Column(DateTime)
    temp = Column(Float)

    __table_args__ = (
        ForeignKeyConstraint(
            ["region_id", "start_date"], ["points.region_id", "points.date"]
        ),
    )


class Precipitation(Base):
    __tablename__ = "precipitation"

    region_id = Column(Integer, primary_key=True)
    start_date = Column(DateTime, primary_key=True)
    end_date = Column(DateTime)
    precip = Column(Float)

    __table_args__ = (
        ForeignKeyConstraint(
            ["region_id", "start_date"], ["points.region_id", "points.date"]
        ),
    )
