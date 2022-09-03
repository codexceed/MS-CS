import re
from argparse import ArgumentParser
from typing import Tuple

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

from db.schema import *


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
    columns = [
        attrib
        for attrib in target_schema.__dir__()
        if "_" not in attrib and attrib not in ["metadata", "registry"]
    ]
    query_args = {}
    print(
        f"For {action} action on table {table_name}, please provide the following arguments:"
    )
    if action == Action.insert:
        for col in columns:
            query_args[col] = input(f"{col}=")
    else:
        filter_str = input(
            f"Please specify row filter in the format: 'column1=value1 column2=value2'"
        )
        query_args = {
            col.strip(): val.strip()
            for col, val in [
                arg_pair.split("=")
                for arg_pair in re.findall(r"\w+\s*=\s*\w+", filter_str)
            ]
        }

    # Create a session and execute the query
    engine = create_engine("sqlite:///../db/grople.db", echo=True)
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)

    with Session() as session:
        if action == Action.insert or action == Action.update:
            session.add(target_schema(**query_args))
        elif action == Action.delete:
            session.delete(target_schema(**query_args))

        session.commit()
