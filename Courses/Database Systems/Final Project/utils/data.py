import numpy as np
import pandas as pd


def preprocess_grople_inp(inp_df: pd.DataFrame, normalize=True) -> np.ndarray:
    """
    Expects input dataframe for grople syrup training.
    """
    feature_order = [
        "precip",
        "smos",
        "temp",
        "index",
        "region_id__93",
        "region_id__94",
        "region_id__95",
        "region_id__97",
        "region_id__98",
        "region_id__99",
        "region_id__102",
        "region_id__103",
        "region_id__104",
        "region_id__105",
        "year",
        "month",
        "day",
    ]
    inp = inp_df.loc[:, feature_order].values

    if normalize:
        target = inp[:, :4]
        mean, std = target.mean(axis=0), target.std(axis=0)
        inp[:, :4] = (inp[:, :4] - mean) / std

    return inp
