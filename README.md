
# Overview

This project utilizes the Least-Squares Monte Carlo method (LSM Monte Carlo) to approximate the market value of American Put options. I use real options prices sampled primarily from high volume, moderate volatility equities such as MSFT. The numerical results, for a certain sample (samples being real options listing at a point in time, including time to expiration and implied volatility) to compare the model results against the real market value of a given option contract with certain sample data.  This is done with randomized samples and shows model convergence to real market values with increasing paths and steps.

# 1. Overview of American Put options

An American put option gives the holder the RIGHT, not the obligation, to sell a security at a strike price at or before a certain date. The ability to excersise the option before the date of expiration significantly increases complexity over of pricing over the more simplistic european options. This is because European options are exercised once at expiry while American options can be exercised any time before or at expiry. Because of this, American option pricing models must find the optimal stopping times. 

An American put option has a few key components associated with it. First is the underlying security which the option is associated with. Second is the strike price, or the price at which the security can be sold for before expiration. Finally is the expiration time, which is the time the option can be exercised at or before. From the expiration time, the time until expiration can be derived as the expiration time - current time. 

To exercise the put option is to use to option to sell the underlying at the strike price. Note that the option does not include the underlying, it only includes rights associated with the underlying. To exercise, the option holder must hold the underlying, then invoke the put option giving the holder the right to sell the underlying at the strike price. Also note the usage of the word "right" here. The option holder is not obliagated to exercise the option. Because of this, the lowest value the option can have is zero. 

For this same reason, a factor of an option's price is it's time to expiration, with greater time to expiration correlation with a price that further exceeds it's intrinsic value. This is options price in expectation, and with more time until expiration, the option has higher likelyhood of finding more optimal stopping times. The price of an option that exceeds it's intrinsic value is called the premium (premium = option price - intrinsic value). This is the reason that even options with zero intrinsic value can still be costly.

Furthermore, the value of an option at expiration is known. This is becaues the option has no more time left to be exercised, so its price must be equal to it's intrinsic value at the terminal state. 


# 2. Overview of LSM Monte Carlo method
The Monte Carlo model itself has 3 other important model inputs aside from the data associated with the option and underlying. 
1. P = Path count
2. N = Step count
3. r = Underlying risk free rate

## The LSM monte carlo method has 3 main steps:
### 1. Generates price paths
Using Geometric brownian motion, factoring in volatility and risk free rate, the algorithm generates P price paths with N steps. The paths are generated into a matrix with 0...P-1 rows and 0...N-1 collumns.

### 2. Generates cashflow matrix
To start, an empty matrix with rows 0...P-1 and collumns 0...N-1 are generated. Using the fact that option prices are known at the terminal state, the final collumn of the matrix (collumn N-1) is filled with intrinsic values generated from collumn N-1 of price path matrix.

### 3. Fills cashflow matrix with optimal stopping times
This step is complex but will be explained in brief terms. Please read my associated paper evaluating properties and implementations of the LSM monte carlo here (link will be added shortly once paper published) or the original LSM monte carlo publication by Longstaff-Shwartz here (https://people.math.ethz.ch/~hjfurrer/teaching/LongstaffSchwartzAmericanOptionsLeastSquareMonteCarlo.pdf)

The process starts with the step before the terminal step (collumn N-2 of matrix) and works backwards until it is at the first step. At each inductive step it performs the following:

First: The model finds the paths where the option is in the money (intrinsic > 0). Then for each of those paths, it extracts the underlying price from price path matrix at time n and the cashflows from cashflow matrix at time n+1 and adds them to collumn vectors X and Y (X being prices at time n and Y being cashflows at time n+1 : for in the money paths only). It also discounts the Y vector by one step (Disounts by risk free rate)

Second: The model computes regression from X to Y using least squares and some basis function to compute an expected value function E(X). The E(X) function gives an approximation of the cashflow at step n+1 given the stock price at n.

Third: Using the expected value function, determine the expected value of continuance for each in the money path at step n. Then compute the intrinsic value of the option at step n for the in the money paths. Using this data, determine for each in the money path at step n, if it is optimal to exercsise or continue. Continuation should happen if the expected value of continuance is greater than intrinsic value. If it is instead optimal to exercise, then the cashflow table value for the given path and time step should be equal to the intrinsic value. When an option is exercised, all cashflows to the right of it are zero (because exercise can only occur once).

The result is the cashflow matrix should have no more than 1 nonzero value per row (because an option can be exercised once). These nonzero values are the cashflows at the optimal stopping times for each path.

### 4. Computes price using stopping time

Then once the cashflow matrix is filled with the optimal stopping times, computing the option price is relatively straightforward. For each path, use the risk free rate to discount the optimal stopping time cashflow back to time step n=0. Then take the average of these discounted cashflows to get the option price.

# 3. Implementation
For this section, I will segment into the four main steps discussed in the method overview. For each step I will include snippits of code that perform the bulk of what is discussed in the step. Small code structure details may by exlcluded for ease of readability (such as variable definitions or function headers). The implementation of the pricing model is done within the following function which is what will be discussed during this section:
```c++
double pricePutOption(double So, double T, int N, int P, double r, double v, double K, double q) {....
```

### 1.Generate Price Paths

In the pricing function, the price path matrix is assigned to S:
```c++
std::vector<std::vector<double>> S = generatePricePathMatrix(P,So,dt,N,r,v,q);
```
Using the function defined as follows:
```c++
std::vector<std::vector<double>> generatePricePathMatrix(int P, double So, double dt, int N, double r, double v, double q) {
    std::vector<std::vector<double>> paths(P, std::vector<double>(N, 0.0));

    double drift = (r - 0.5 * v * v) * dt;
    double vol = v * std::sqrt(dt);

    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> d(0.0, 1.0);

    for (int p = 0; p < P; p++) {
        double last = So;

        for (int n = 0; n < N; n++) {
            last = last * std::exp(drift + (vol * d(gen)));
            paths[p][n] = last;
        }
    }

    return paths;
}
```
### 2. Generate cashflow matrix
The cashflow matrix is defined given the price path matrix S defined in previous step
```c++
std::vector<std::vector<double>> C(P, std::vector<double>(N, 0.0));

for (int p = 0; p<P; p++) {
C[p][N-1] = fmax(K-S[p][N-1],0);
}
```

### 3. Fills cashflow matrix with the optimal stopping times

```c++
for (int n= N-2; n>=0; n--) {
        //get X and Y
        X.clear();
        Y.clear();
        itm_indices.clear();

        for (int p = 0; p<P; p++) {
            if (K-S[p][n] > 0 && C[p][n+1] > 0) {
                    X.push_back(S[p][n]);
                    Y.push_back(C[p][n+1] * exp(-r*dt));
                    itm_indices.push_back(p);
            }
        }

        // if it is optimal to exercise nowhere in this step, skip to next step
        if (X.size() == 0) {
            continue;
        }
        
        //here is the part where we determine E() function

        //if there are less that 3 datapoints, assume E() is zero
        bool useReg = X.size() >2;
        if (useReg) {
            std::vector<double> solution  = quadRegress(X,Y);
            c_coeff = solution[0];
            b_coeff = solution[1];
            a_coeff = solution[2];
        }
        
        for (int i = 0; i < itm_indices.size(); i++ ){
            int p = itm_indices[i];
            double intrinsic = fmax(K-S[p][n],0);
            double expectedContinuance;

            if (useReg) {
                expectedContinuance = c_coeff + (b_coeff * S[p][n]) + (a_coeff * S[p][n] * S[p][n]);
            } else {
                expectedContinuance = 0;
            }

            if (intrinsic > expectedContinuance) {
                C[p][n] = intrinsic;
            }
            
        }
}
```

### 4. Computes price using stopping times
```c++
double price = 0;
    for (int p=0; p<P; p++) {
        for (int n=0; n<N; n++) {
            if (C[p][n] > 0) {
                price += C[p][n] * exp(-r * dt * (n+1));
                break;
            }
        }
    }

return price/P;
```
# 4. Model Evaluation


