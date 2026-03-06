#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <cmath>
#include <iterator>
#include <chrono>

#include <omp.h>
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cusolverDn.h>
#include <cublas_v2.h>




// The Kernel: Each thread calculates ONE full path
__global__ void generatePathsKernel(double* d_paths, int P, int N, double So, double drift, double vol, unsigned long seed) {
    int p = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (p < P) {
        // Each thread needs its own random state
        curandState state;
        curand_init(seed, p, 0, &state);

        double last = So;
        for (int n = 0; n < N; n++) {
            double gauss = curand_normal_double(&state);
            last = last * exp(drift + (vol * gauss));
            // Store in flat 1D array (Row-Major: path * total_steps + current_step)
            d_paths[p * N + n] = last;
        }
    }
}

// Host Wrapper Function
std::vector<double> generateCudaPricePathMatrix(int P, double So, double dt, int N, double r, double v) {
    size_t size = P * N * sizeof(double);
    std::vector<double> h_paths(P * N);
    
    double* d_paths;
    cudaMalloc(&d_paths, size);

    double drift = (r - 0.5 * v * v) * dt;
    double vol = v * sqrt(dt);

    // Launch configuration
    int threadsPerBlock = 256;
    int blocksPerGrid = (P + threadsPerBlock - 1) / threadsPerBlock;

    generatePathsKernel<<<blocksPerGrid, threadsPerBlock>>>(d_paths, P, N, So, drift, vol, time(NULL));

    // Copy results back to CPU
    cudaMemcpy(h_paths.data(), d_paths, size, cudaMemcpyDeviceToHost);
    
    cudaFree(d_paths);
    return h_paths;
}
//############################################
//          PUT IMPLEMENTATION
//############################################

double pricePutOption(double So, double T, int N, int P, double r, double v, double K) {
    double dt = T / N;
    std::vector<double> S = generateCudaPricePathMatrix(P,So,dt,N,r,v);
    std::vector<std::vector<double>> C(P, std::vector<double>(N, 0.0));
    std::vector<int> itm_indices;
    std::vector<double> X;
    X.reserve(P/10);
    std::vector<double> Y;
    Y.reserve(P/10);
    double c_coeff;
    double b_coeff;
    double a_coeff;
    double EV;

    #pragma omp parallel for
    for (int p = 0; p<P; p++) {
        C[p][N-1] = fmax(K-S[(p*N)+N-1],0);
    }
    
    for (int n= N-2; n>=0; n--) {
        //get X and Y
        X.clear();
        Y.clear();
        itm_indices.clear();

        #pragma omp parallel for
        for (int p = 0; p<P; p++) {
            if (K-S[(p*N)+n] > 0 && C[p][n+1] > 0) {
                #pragma omp critical
                {
                    X.push_back(S[(p*N)+n]);
                    Y.push_back(C[p][n+1] * exp(-r*dt));
                    itm_indices.push_back(p);
                    }
            }
        }
        //std::cout << X_filtered.size();

        // if it is optimal to exercise nowhere in this step, skip to next step
        if (X.size() == 0) {
            continue;
        }

        
        //here is the part where we determine E() function

        //if there are less that 3 datapoints, assume E() is mean of Y_filtered
        std::vector<double> coefs;
        bool useReg = X.size() > 2;
        if (useReg) {
            // in format {intercept, x coef, x^2 coef}
            coefs = {0,0,0};//solveQuadraticRegression(X, Y);
        }
        
        #pragma omp parallel for
        for (int i = 0; i < itm_indices.size(); i++ ){
            int p = itm_indices[i];
            double intrinsic = fmax(K-S[(p*N)+n],0);
            double expectedContinuance;

            if (useReg) {
                expectedContinuance = coefs[2] + (coefs[1] * S[(p*N)+n]) + (coefs[0]* S[(p*N)+n] * S[(p*N)+n]);
            } else {
                expectedContinuance = 0;
            }

            if (intrinsic > expectedContinuance) {
                C[p][n] = intrinsic;
            }
            
        }
    }

    double price = 0;
    #pragma omp parallel for
    for (int p=0; p<P; p++) {
        for (int n=0; n<N; n++) {
            if (C[p][n] > 0) {
                price += C[p][n] * exp(-r * dt * (n+1));
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
        sum += std::abs(pricePutOption(row[0],T,N,paths,riskFreeRate,row[3],row[1]) - row[2]) / row[2];

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
    /*
    std::cout << "ITM or OTM only? (ITM,OTM,No)" << std::endl;
    std::cin >> choice;

    if (choice == "ITM") {
        ITMonly = true;
    }
    else if (choice == "OTM") {
        OTMonly = true;
    }
    */
    std::cout << "How many steps per hour?" << std::endl;
    std::cin >> stepsPerHour;

    //RISK FREE RATE IS SET TO 0.04
    riskFreeRate = 0.04;
    //std::cout << "Risk Free Rate?" << std::endl;
    //std::cin >> riskFreeRate;

    //PRESENT DATA SET CHOICE
    std::string dataSet = "TestData/FridayPutSpread_MSFT-Simplified.csv";
    /*
    auto start = std::chrono::high_resolution_clock::now();

    double result = runTest(paths,stepsPerHour,riskFreeRate,samples,dataSet,ITMonly,OTMonly);

    auto end = std::chrono::high_resolution_clock::now();

    // Print directly in seconds
    std::cout << "Total runtime: " 
              << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() 
              << " seconds." << std::endl;
    */
    double result = runTest(paths,stepsPerHour,riskFreeRate,samples,dataSet,ITMonly,OTMonly);


    std::cout << "Mean Percent Error:" << result;

    return 0;
}

//C:\msys64\ucrt64\bin\g++.exe -fopenmp Tester.cpp -o Tester.exe
//C:\msys64\ucrt64\bin\g++ -fopenmp Tester.cpp
//nvcc CudaRewrite.cu -o my_sim -lcurand
//nvcc CudaRewrite.cu -o my_sim -lcurand -lcusolver -lcublas