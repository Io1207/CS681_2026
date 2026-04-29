# eval metrics

import numpy as np
import torch
import csv

def mape(y_true, y_pred):
    return np.mean(np.abs((y_true - y_pred) / y_true + 1e-8)) * 100

def p90_error(y_true, y_pred):
    errors = np.abs((y_true - y_pred) / y_true + 1e-8) * 100
    return np.percentile(errors, 90)

def percentile_ape(y_true, y_pred, p):
    errors = np.abs((y_true - y_pred) / y_true + 1e-8) * 100
    return np.percentile(errors, p)

def add_bias(X):
    return np.c_[np.ones(X.shape[0]), X]

def mse(y_true, y_pred):
    return np.mean((y_true - y_pred) ** 2)

def train_test_split(X, y, test_ratio=0.2):
    n = len(X)
    idx = np.random.permutation(n)
    split = int(n * (1 - test_ratio))
    return X[idx[:split]], X[idx[split:]], y[idx[:split]], y[idx[split:]]

def normalise(X):
    mean = X.mean(axis=0)
    std = X.std(axis=0) + 1e-8
    return (X - mean) / std, mean, std


def load_csv(path):
    """
        assumes that the csv file's headers are:
        cpu,mem,loop,file_size (requested data size),users,response_time
    """
    data = []
    with open(path, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            data.append([float(x) for x in row])
    data = np.array(data)

    X = data[:, :-1]
    y = data[:, -1]
    return X, y