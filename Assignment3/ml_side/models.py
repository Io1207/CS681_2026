# models .py
# will be implementing some models from scratch here
# just for fun and hopefully these will be easier to 
# tinker with and tune to our liking that the library ones

import numpy as np
import matplotlib.pyplot as plt
from eval import mape, p90_error, percentile_ape, mse, add_bias, load_csv, normalise, train_test_split
import csv
import tqdm
import torch
import torch.nn as nn
from sklearn.model_selection import train_test_split

"""
    I would not like to call Linear Regression a model but 
    just have it here to get a baseline and show the reason
    why analytical/simulation methods aren't sufficient
    
    Also I am pretty sure that the assignment expects us to do this
"""

class LinearRegression_Mundane:
    def __init__(self):
        self.theta = None

    def fit(self, X, y):
        X_b = add_bias(X)
        lambda_I = 1e-6 * np.eye(X_b.shape[1])
        self.theta = np.linalg.inv(X_b.T @ X_b + lambda_I) @ X_b.T @ y

    def predict(self, X):
        X_b = add_bias(X)
        return X_b @ self.theta

class LinearRegressionGD:
    def __init__(self, lr=0.001, epochs=1000):
        self.lr = lr
        self.epochs = epochs
        self.theta = None

    def fit(self, X, y):
        X_b = add_bias(X)
        n, d = X_b.shape
        self.theta = np.zeros(d)

        for _ in range(self.epochs):
            preds = X_b @ self.theta
            grad = (X_b.T @ (preds - y)) / n
            self.theta -= self.lr * grad

    def predict(self, X):
        X_b = add_bias(X)
        return X_b @ self.theta


"""
    Once we have concluded/accepted/"coped with" the fact that
    linear regression is very bleh, we will come here
"""

def polynomial_features(X, degree=2):
    n, d = X.shape
    features = [X]

    if degree >= 2:
        features.append(X ** 2)

        interactions = []
        for i in range(d):
            for j in range(i + 1, d):
                interactions.append((X[:, i] * X[:, j]).reshape(-1, 1))

        if interactions:
            features.append(np.hstack(interactions))

    return np.hstack(features)

class PolynomialRegression:
    def __init__(self, degree=2):
        self.degree = degree
        self.model = LinearRegression_Mundane()

    def fit(self, X, y):
        X_poly = polynomial_features(X, self.degree)
        self.model.fit(X_poly, y)

    def predict(self, X):
        X_poly = polynomial_features(X, self.degree)
        return self.model.predict(X_poly)

"""
    Decision Trees:
    might work nicely because of the non linearities in the sys behaviour
    and because behaviour is sort of divided into regions of the parameter
    space and decision trees could help in the piecewise learning/ parameter
    wise learning that is needed here
    
    Also I learnt about them in CS6104 and would like to use them somewhere
"""

class TreeNode:
    def __init__(self, feature=None, threshold=None, left=None, right=None, value=None):
        self.feature = feature
        self.threshold = threshold
        self.left = left
        self.right = right
        self.value = value

class DecisionTreeRegressor:
    def __init__(self, max_depth=5, min_samples_split=10):
        self.max_depth = max_depth
        self.min_samples_split = min_samples_split
        self.root = None

    def fit(self, X, y):
        self.root = self._build_tree(X, y, 0)

    def _build_tree(self, X, y, depth):
        n, d = X.shape

        if depth >= self.max_depth or n < self.min_samples_split:
            return TreeNode(value=np.mean(y))

        best_feature, best_thresh = self._best_split(X, y)

        if best_feature is None:
            return TreeNode(value=np.mean(y))

        left_idx = X[:, best_feature] <= best_thresh
        right_idx = X[:, best_feature] > best_thresh

        left = self._build_tree(X[left_idx], y[left_idx], depth + 1)
        right = self._build_tree(X[right_idx], y[right_idx], depth + 1)

        return TreeNode(best_feature, best_thresh, left, right)

    def _best_split(self, X, y):
        n, d = X.shape
        best_mse = float("inf")
        best_feature, best_thresh = None, None

        for f in range(d):
            thresholds = np.unique(X[:, f])

            for t in thresholds:
                left = y[X[:, f] <= t]
                right = y[X[:, f] > t]

                if len(left) == 0 or len(right) == 0:
                    continue

                mse_split = (
                    len(left) * np.var(left) +
                    len(right) * np.var(right)
                ) / n

                if mse_split < best_mse:
                    best_mse = mse_split
                    best_feature = f
                    best_thresh = t

        return best_feature, best_thresh

    def predict(self, X):
        return np.array([self._predict_one(x, self.root) for x in X])

    def _predict_one(self, x, node):
        if node.value is not None:
            return node.value

        if x[node.feature] <= node.threshold:
            return self._predict_one(x, node.left)
        else:
            return self._predict_one(x, node.right)

"""
    Artificial Neural Network:
    It's a Black Box!!! I don't know what goes on and why
"""
class ANN_Model(nn.Module):
    def __init__(self, input, hidden, output):
        super().__init__()
        self.weights = nn.ModuleList()
        self.relus = nn.ModuleList()
        last = input
        for i in hidden:
            self.weights.append(nn.Linear(last,i))
            self.relus.append(nn.ReLU())
            last = i
        self.weights.append(nn.Linear(last, output))

    def forward(self, x):
        for w, r in zip(self.weights[:-1], self.relus):
            x = r(w(x))
        return self.weights[-1](x)

class ANN:
    def __init__(self, input, hidden, output, lr=0.001,device='cpu'):
        self.device = device
        self.model = ANN_Model(input, hidden, output).to(self.device)
        self.lr = lr
        self.loss_fn = mape
        self.optimizer = torch.optim.Adam(self.model.parameters(), lr=self.lr)

    def fit(self, X, y, epochs=1000):
        X = torch.tensor(X, dtype=torch.float32)
        y = torch.tensor(y, dtype=torch.float32).view(-1, 1)
        X, valX, y, valy = train_test_split(X, y, test_size=0.1) 
        X = X.to(self.device)
        y = y.to(self.device)

        # normalization
        # X, self.X_mean, self.X_std = normalise(X)
        # y, self.y_mean, self.y_std = normalise(y)


        # valX = valX.to(self.device)
        # valy = valy.to(self.device)

        # valX = (valX - self.X_mean)/self.X_std
        # valy_transformed = (valy - self.y_mean)/self.y_std 
        for epoch in (range(epochs)):
            self.model.train()
            self.optimizer.zero_grad()
            outputs = self.model(X)
            loss = self.loss_fn(y, outputs)
            loss.backward()
            self.optimizer.step()

            # with torch.no_grad():
                # self.model.eval()
                # val_out = self.model(valX)
                # val_loss = self.loss_fn(val_out, valy_transformed)
                # val_out = val_out * self.y_std + self.y_mean
                # p90_e = p90_error(valy, val_out)
                # p80_e = percentile_ape(valy, val_out, 80)

            # print(f"Epoch {epoch+1}/{epochs}, Loss: {loss.item()}, Validation Loss: {val_loss.item()}, 90%ile_APE: {p90_e}, 80%ile_APE: {p80_e}")

    def predict(self, X):
        X = torch.tensor(X, dtype=torch.float32)
        # X = (X - self.X_mean) / self.X_std
        X = X.to(self.device)
        with torch.no_grad():
            self.model.eval()
            out = self.model(X)
            # out = out * self.y_std + self.y_mean
            return out.cpu()


def generate_toy_data(n=500):
    """
        cpu,mem,loop,file_size (requested data size),users,response_time
    """
    np.random.seed(42)

    cpu = np.random.uniform(0.5, 2.0, n)
    memory = np.random.uniform(128, 1024, n)
    users = np.random.uniform(1, 200, n)
    io_size = np.random.uniform(1, 1000, n)

    X = np.column_stack([cpu, memory, users, io_size])
    y = []

    for i in range(n):
        c, m, u, io = cpu[i], memory[i], users[i], io_size[i]

        rt = 50
        rt += 100 / c
        rt += 0.05 * io

        if u < 50:
            rt += 0.5 * u
        elif u < 120:
            rt += 0.5 * 50 + 1.5 * (u - 50)
        else:
            rt += 0.5 * 50 + 1.5 * 70 + 5 * (u - 120)

        if u > 100 and c < 1:
            rt *= 1.8

        if m < 256:
            rt *= 1.3

        rt += np.random.normal(0, 10)
        y.append(rt)

    return np.array(X), np.array(y)


if __name__ == "__main__":
    np.random.seed(42)

    # Generate data
    X, y = generate_toy_data(500)

    # Normalize
    X, mean, std = normalise(X)

    # Split
    X_train, X_test, y_train, y_test = train_test_split(X, y)

    models = {
        "Linear_NE": LinearRegression_Mundane(),
        "Linear_GD": LinearRegressionGD(),
        "Polynomial": PolynomialRegression(),
        "DecisionTree": DecisionTreeRegressor()
    }

    for name, model in models.items():
        print(f"\n-------- {name} --------")

        model.fit(X_train, y_train)
        preds = model.predict(X_test)

        print("MAPE:", mape(y_test, preds))
        print("P90 Error:", p90_error(y_test, preds))

        # Scatter plot
        plt.scatter(y_test, preds)
        plt.xlabel("True")
        plt.ylabel("Predicted")
        plt.title(name)
        plt.show()