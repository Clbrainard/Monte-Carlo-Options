
# Least Squares Monte Carlo Option Pricer

C++ implementation of the Least Squares Monte Carlo (LSM) algorithm for pricing American and path-dependent options. LSM works by backwards induction over simulated GBM price paths, fitting a regression at each time step to approximate the continuation value and determine when early exercise is optimal. See Longstaff & Schwartz (2001).

Part of a larger portfolio of option pricing implementations, including a [binomial tree pricer](https://github.com/Clbrainard/Binomial-Options). For more detailed information on the LSM Monte Carlo method for American options look at `notes.pdf`

---

## Supported Instruments

| Instrument | Method |
|---|---|
| American put / call | LSM backwards induction |
| European put / call | Discounted payoff average |
| Asian put / call (arithmetic) | Discounted average-price payoff |

---

## Validation

Benchmarked against QuantLib's finite difference engine (`FdBlackScholesVanillaEngine`) on a 3200×3200 grid. Validation datasets and the generation script are in `Validation/`. Tables show MAPE (%).

**American options**

| | 500 paths / 25 steps | 1000 paths / 50 steps | 5000 paths / 250 steps |
|---|---|---|---|
| Put | 2.942 | 2.105 | 1.465 |
| Call | 3.338 | 2.317 | 0.971 |

**Asian options** (255 steps)

| | 1 000 paths | 10 000 paths | 100 000 paths |
|---|---|---|---|
| Put | 3.190 | 1.518 | 0.874 |
| Call | 3.335 | 1.877 | 1.001 |

**European options**

| | 1 000 paths | 10 000 paths |
|---|---|---|
| Put | 2.037 | 0.710 |
| Call | 2.401 | 0.765 |

---

## Key Findings

All instruments converge steadily as path count increases. European and Asian options show roughly a 3× error reduction for a 10× increase in paths, broadly consistent with Monte Carlo's $1/\sqrt{P}$ rate. American options improve with both path count and step count — going from 500P/25N to 5000P/250N reduces MAPE by half for puts and by two-thirds for calls.

Asian options reach sub-1% MAPE at 100 000 paths. European options reach it at 10 000 paths. American options have the additional discretisation error from the backwards induction sweep, so they require both more paths and more steps (the 1.465% / 0.971% figures above are at 250 steps).

---

## Usage

```cpp
#include "Pricers.h"

// American — S0, T (years), N (steps), P (paths), r, sigma, K, regType
// regType: 
//  1=Poly- deg2  2=Poly- deg3  3=Legendre- deg2  4=Legendre- deg3
//  5=Hermite- deg2  6=Hermite- deg3  7=Laguerre- deg2  8=Laguerre- deg3
double put  = priceAmericanPut (100.0, 1.0, 50, 10000, 0.06, 0.20, 100.0, 1);
double call = priceAmericanCall(100.0, 1.0, 50, 10000, 0.06, 0.20, 100.0, 1);

// European — S0, T (years), P (paths), r, sigma, K
double eu_put  = priceEuropeanPut (100.0, 1.0, 10000, 0.06, 0.20, 100.0);
double eu_call = priceEuropeanCall(100.0, 1.0, 10000, 0.06, 0.20, 100.0);

// Asian — S0, T (years), N (steps), P (paths), r, sigma, K
double asian_put  = priceAsianPut (100.0, 1.0, 255, 10000, 0.06, 0.20, 100.0);
double asian_call = priceAsianCall(100.0, 1.0, 255, 10000, 0.06, 0.20, 100.0);
```

Requires a C++17 compiler. Eigen is bundled as a header-only dependency.

---

## References

F. A. Longstaff and E. S. Schwartz, "Valuing American Options by Simulation: A Simple Least-Squares Approach," *The Review of Financial Studies*, vol. 14, no. 1, pp. 113–147, 2001.


