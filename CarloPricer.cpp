#include <string>
#include <cmath>
#include <random>
#include <vector>
#include <iterator>
#include "Eigen/Dense"
#include <pybind11/pybind11.h>

std::vector<double> generateFlatPricePathMatrix(int P, double So, double dt, int N, double r, double v) {
    std::vector<double> flatArray(P * N);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> d(0.0, 1.0);

    // Pre-calculate constant terms to save clock cycles inside the loop
    double drift = (r - 0.5 * v * v) * dt;
    double vol = v * std::sqrt(dt);

    double last;
    for (int p = 0; p<P; p++) {
        last = So;
        for (int n = 0; n<N; n++) {
            last = last * exp(
                ( drift + (vol * d(gen)) )
            );
            flatArray[(p*N)+n] = last;
        }
    }

    return flatArray;
}

//############################################
//          PUT IMPLEMENTATION
//############################################

double pricePutOption(double So, double T, int N, int P, double r, double v, double K) {
    double dt = T / N;
    std::vector<double> S = generateFlatPricePathMatrix(P,So,dt,N,r,v);
    std::vector<double> C(P * N);
    std::vector<int> itm_indices;
    itm_indices.reserve(P);

    for (int p = 0; p<P; p++) {
        C[(p*N)+(N-1)] = fmax(K-S[(p*N)+(N-1)],0);
    }
    
    for (int n= N-2; n>=0; n--) {
        //get X and Y

        std::vector<double> X(P);
        std::vector<double> Y(P);
        std::vector<double> X_filtered;
        std::vector<double> Y_filtered;
        X_filtered.reserve(P);
        Y_filtered.reserve(P);

        for (int p = 0; p<P; p++) {
            X[p] = S[p*N+n];
            Y[p] = C[p*N+n+1] * exp(-r*dt);
            if (K-X[p] > 0 || Y[p] != 0) {
                X_filtered.push_back(X[p]);
                Y_filtered.push_back(Y[p]);
                itm_indices.push_back(p);
            }
        }

        // if it is optimal to continue nowhere in this step, skip to next step
        if (X_filtered.size() == 0) {
            continue;
        }

        
        //here is the poart where we determine E() function

        //if there are less that 3 datapoints, assume E() is mean of Y_filtered
        double c_coeff;
        double b_coeff;
        double a_coeff;
        double EV = 0;
        if (X_filtered.size() < 3) {
            EV = std::accumulate(Y_filtered.begin(), Y_filtered.end(), 0.0) / Y_filtered.size();
        } else {

        //Implement more efficient way using determinants and stuff to solve linear alg problem LATER
        // for now just used gemini and eigen library
        // X and Y are your ITM prices and cash flows
        Eigen::MatrixXd A(X_filtered.size(), 3);
        Eigen::VectorXd b(Y_filtered.size());

        for(int i = 0; i < X_filtered.size(); i++) {
            A(i, 0) = 1.0;          // Constant term
            A(i, 1) = X_filtered[i];         // x term
            A(i, 2) = X_filtered[i] * X[i];  // x^2 term
            b(i) = Y_filtered[i];
        }

        // The "Magic" line that replaces all the manual math:
        Eigen::Vector3d beta = A.householderQr().solve(b);

        c_coeff = beta(0); // intercept
        b_coeff = beta(1); // x coefficient
        a_coeff = beta(2); // x^2 coefficient

        }
        
        for (int i = 0; i < itm_indices.size(); i++ ){
            int p = itm_indices[i];
            double intrinsic = fmax(K-X[p],0);
            double expectedContinuance;

            if (intrinsic ==0 & Y[p] ==0) {
                continue;
            }

            if (EV == 0) {
                expectedContinuance = c_coeff + (b_coeff * X[p]) + (a_coeff * X[p] * X[p]);
            } else {
                expectedContinuance = EV;
            }

            if (intrinsic > expectedContinuance) {
                C[p*N+n] = intrinsic;
            }
            
        }
    }

    double price = 0;
    for (int p=0; p<P; p++) {
        for (int n=0; n<N; n++) {
            if (C[p*N+n] > 0) {
                price += C[p*N+n] * exp(-r * n * dt);
                break;
            }
        }
    }

    return price/P;
}



