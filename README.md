
# Least Squares Monte Carlo Option Pricer

> **Technical notes** — implementation details, numerical analysis, and findings are documented in the accompanying PDF (`notes.pdf`). This README provides a brief overview only.

This repository implements the Least Squares Monte Carlo (LSM) algorithm for pricing American and path-dependent options in C++. LSM prices American options by backwards induction over simulated price paths, using least-squares regression at each time step to estimate the continuation value. See Longstaff & Schwartz (2001) for the original formulation.

This repository is part of a larger portfolio implementing multiple American option pricing methods, including a [binomial tree pricer](https://github.com/clbra/Binomial-Tree-Options) and a forthcoming neural network pricer.

---

## Supported Instruments

- American puts
- American calls
- European puts
- European calls
- Asian calls
- Asian puts

---

## Validation

Results are benchmarked against QuantLib's finite difference engine (`FdBlackScholesVanillaEngine`) using a 3200×3200 time/asset grid as the reference solution. Validation datasets are in `Validation/` ([AmericanCalls.csv](Validation/AmericanCalls.csv), [AmericanPuts.csv](Validation/AmericanPuts.csv), [EuropeanCalls.csv](Validation/EuropeanCalls.csv), [EuropeanPuts.csv](Validation/EuropeanPuts.csv), [AsianCalls.csv](Validation/AsianCalls.csv), [AsianPuts.csv](Validation/AsianPuts.csv)) and results can be reproduced via [Validation/quantlibTester.py](Validation/quantlibTester.py).

**American options** — MAPE convergence across path/step configurations (benchmarked against FD 3200×3200 grid)

| Instrument | 1000P / 50N | 5000P / 100N | 10000P / 200N |
|---|---|---|---|
| American puts | *[fill in]* | *[fill in]* | *[fill in]* |
| American calls | *[fill in]* | *[fill in]* | *[fill in]* |

**European & Asian options** (benchmarked against FD 3200×3200 grid)

| Instrument | MAPE for 1000P |
|---|---|
| European puts | *[fill in]* |
| European calls | *[fill in]* |
| Asian puts | *[fill in]* |
| Asian calls | *[fill in]* |

*[Convergence plot placeholder]*

---

## Key Findings

*[Fill in your observations — e.g., convergence rate with respect to path count, basis function sensitivity, early exercise premium behavior, comparison to Table 1 of Longstaff & Schwartz (2001), etc.]*

---

## Usage

```cpp
#include "Pricers.h"

// Price an American put
// Args: S0, T (years), N (steps), P (paths), r, sigma, K, q (dividend yield)
double price = priceAmericanPut(100.0, 1.0, 50, 10000, 0.06, 0.20, 100.0, 0.0);
```

Build with a C++17-compatible compiler. Eigen is included as a header-only dependency.

---

## References

F. A. Longstaff and E. S. Schwartz, "Valuing American Options by Simulation: A Simple Least-Squares Approach," *The Review of Financial Studies*, vol. 14, no. 1, pp. 113–147, 2001.


