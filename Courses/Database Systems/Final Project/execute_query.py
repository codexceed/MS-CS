#!/usr/bin/env python
import re
from argparse import ArgumentParser
from datetime import datetime
from typing import Tuple

from sqlalchemy import Date, Float, Integer, create_engine
from sqlalchemy.orm import sessionmaker

from constants.env import DB_PATH
from db.schema import NDVI, Base, Moisture, Precipitation, Production, Temperature


class Action:
    insert = "insert"
    delete = "delete"
    update = "update"


def parse_cli_args() -> Tuple:
    """Parse CLI input, set globals.

    Returns:
        Argument values from CLI
    """
    parser = ArgumentParser(description="Perform queries on the grople DB.")
    parser.add_argument(
        "-a",
        "--action",
        help="Type of query",
        type=str,
        required=True,
        choices=[Action.insert, Action.delete, Action.update],
    )
    parser.add_argument(
        "-t",
        "--table",
        help="Target table",
        type=str,
        required=True,
        choices=["production", "ndvi", "moisture", "temperature", "precipitation"],
    )

    args = parser.parse_args()

    return args.action, args.table


if __name__ == "__main__":
    # Parse CLI input
    action, table_name = parse_cli_args()

    # Identify a valid target table
    schema_classes = [Production, NDVI, Moisture, Temperature, Precipitation]
    target_schema = None
    for table in schema_classes:
        if table.__tablename__ == table_name:
            target_schema = table

    if target_schema is None:
        raise Exception(f"Specified schema not found: {table_name}")

    # Get query arguments
    valid_column_types = {
        attrib.name: type(attrib.type) for attrib in target_schema.__table__._columns
    }
    query_args = {}
    print(
        f"For {action} action on table {table_name}, please provide the following arguments:"
    )
    if action == Action.insert:
        for col in valid_column_types.keys():
            query_args[col] = input(f"{col}=")
    else:
        filter_str = input(
            f"Please specify row filter in the format: 'column1=value1 column2=value2'. Dates should be formatted "
            f"'dd/mm/yyyy HH:MM:SS' "
        )
        for col, val in [
            arg_pair.split("=") for arg_pair in re.findall(r"\w+\s*=\s*\w+", filter_str)
        ]:
            if col not in valid_column_types.keys():
                raise Exception(
                    f"Column {col.strip()} not present in table {table_name}"
                )
            query_args[col.strip()] = val.strip()

    # Preprocess values
    for col, val in query_args.items():
        if valid_column_types[col] == Integer:
            query_args[col] = int(query_args[col])
        elif valid_column_types[col] == Float:
            query_args[col] = float(query_args[col])
        elif valid_column_types[col] == Date:
            try:
                query_args[col] = datetime.strptime(
                    query_args[col], "%d/%m/%Y %H:%M:%S"
                )
            except ValueError:
                query_args[col] = datetime.strptime(query_args[col], "%d/%m/%Y")
        else:
            raise Exception(f"Unsupported value type for column {col}.")

    # Create a session and execute the query
    engine = create_engine(f"sqlite:///{DB_PATH}", echo=True)
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)

    with Session() as session:
        if action == Action.insert or action == Action.update:
            session.add(target_schema(**query_args))
        elif action == Action.delete:
            session.delete(target_schema(**query_args))

        session.commit()
