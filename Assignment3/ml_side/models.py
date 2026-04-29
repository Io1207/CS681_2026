 #models .py
# will be implementing some models from scratch here
# just for fun and hopefully these will be easier to 
# tinker with and tune to our liking that the library ones

import numpy as np
import matplotlib.pyplot as plt
from ml_side.eval import mape, p90_error, mse, add_bias, load_csv, normalise, train_test_split
import csv

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