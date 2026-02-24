#include <iostream>
#include <string>
//#include "CarloPricer"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <random>

#include <string>
#include <cmath>
#include <random>
#include <vector>
#include <iterator>
#include "Eigen/Dense"

std::vector<double> generateFlatPricePathMatrix(int P, double So, double dt, int N, double r, double v) {
    std::vector<double> flatArray(P * N);
    
    static std::mt19937 gen(std::random_device{}());
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
    std::vector<double> X;
    std::vector<double> Y;
    std::vector<double> X_filtered;
    std::vector<double> Y_filtered;
    X_filtered.reserve(P);
    Y_filtered.reserve(P);

    for (int p = 0; p<P; p++) {
        C[(p*N)+(N-1)] = fmax(K-S[(p*N)+(N-1)],0);
    }
    
    for (int n= N-2; n>=0; n--) {
        //get X and Y
        X.clear();
        Y.clear();
        X_filtered.clear();
        Y_filtered.clear();
        itm_indices.clear();

        for (int p = 0; p<P; p++) {
            X.push_back(S[p*N+n]);
            Y.push_back(C[p*N+n+1] * exp(-r*dt));
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
            A(i, 2) = X_filtered[i] * X_filtered[i];  // x^2 term
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

            if (intrinsic ==0 && Y[p] ==0) {
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

std::vector<std::vector<float>> select_random_samples(const std::string& filename, int s) {
    std::ifstream file(filename);
    std::string line;
    std::vector<std::vector<float>> all_data;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return {};
    }

    // 1. Read first non-empty line and detect header vs data
    std::string first_line;
    while (std::getline(file, first_line)) {
        if (!first_line.empty()) break;
    }

    if (first_line.empty()) return {};

    auto try_parse_line = [](const std::string &ln, std::vector<float> &out) {
        std::stringstream ss(ln);
        std::string cell;
        out.clear();
        while (std::getline(ss, cell, ',')) {
            try {
                out.push_back(std::stof(cell));
            } catch (...) {
                return false;
            }
        }
        return !out.empty();
    };

    std::vector<float> parsed;
    bool first_is_data = try_parse_line(first_line, parsed);
    if (first_is_data) {
        all_data.push_back(parsed);
    }

    // 2. Parse the rest of the file
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::vector<float> row_values;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            try {
                row_values.push_back(std::stof(cell));
            } catch (const std::exception& e) {
                // skip invalid tokens in this line
                row_values.clear();
                break;
            }
        }

        if (!row_values.empty()) {
            all_data.push_back(row_values);
        }
    }
    // 3. Shuffle and Sample
    std::random_device rd;
    std::mt19937 g(rd());

    // Ensure we don't try to sample more rows than exist
    size_t sample_size = std::min(static_cast<size_t>(s), all_data.size());

    // We shuffle the whole vector and then slice the first 's' elements
    std::shuffle(all_data.begin(), all_data.end(), g);

    // Return the subset
    return std::vector<std::vector<float>>(all_data.begin(), all_data.begin() + sample_size);
}

double runTest(int paths, int stepsPerHour,double riskFreeRate,int samples, std::string dataSet, bool ITMonly,bool OTMonly) {
    std::vector<std::vector<float>> S = select_random_samples(dataSet,samples);
    double sum = 0;

    for (int i = 0; i<S.size(); i++) {
        std::vector<float> row = S[i];
        double T = row[4] / (365*24*60);
        int N = (stepsPerHour * 365 * 24) * T;
        sum += std::abs(pricePutOption(row[0],T,N,paths,riskFreeRate,row[3],row[1]) - row[2]);
    }

    return sum/samples;
}

int main() {
    bool ITMonly = false;
    bool OTMonly = false;

    int paths;
    int samples;
    int stepsPerHour;
    double riskFreeRate;

    std::string choice;

    std::cout << "How many paths?" << std::endl;
    std::cin >> paths;

    std::cout << "How many samples?" << std::endl;
    std::cin >> samples;

    std::cout << "ITM or OTM only? (ITM,OTM,No)" << std::endl;
    std::cin >> choice;

    if (choice == "ITM") {
        ITMonly = true;
    }
    else if (choice == "OTM") {
        OTMonly = true;
    }

    std::cout << "How many steps per hour?" << std::endl;
    std::cin >> stepsPerHour;

    std::cout << "Risk Free Rate?" << std::endl;
    std::cin >> riskFreeRate;

    //PRESENT DATA SET CHOICE
    std::string dataSet = "TestData/FridayPutSpread_MSFT-Simplified.csv";

    double result = runTest(paths,stepsPerHour,riskFreeRate,samples,dataSet,ITMonly,OTMonly);

    std::cout << "Mean Percent Error:" << result;

    return 0;
}