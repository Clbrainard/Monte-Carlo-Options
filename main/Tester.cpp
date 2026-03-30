#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <string>
#include <cmath>
#include <vector>
#include <iterator>
#include <Eigen/Dense>
#include <omp.h>
#include <ctime>
#include <chrono>
#include <thread>

// Matrix A stored as flat vector in row-major order
// A[p][n] is stored at A[p * N + n]
// P = number of rows, N = number of columns

#include <chrono>

long long current_milliseconds() {
    using namespace std::chrono;

    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

int current_minute() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* local = std::localtime(&t);
        return local->tm_min;
}

/*
TYPES
1 = Polynomial Deg 2
2 = Polynomial Deg 3
3 = Leandre Deg 2
4 = Leandre Deg 3
5 = Hermite Deg 2
5 = Hermite Deg 3
6 = Laguerre Deg 2
7 = Laguerre Deg 3
*/
#include <vector>
#include <cmath>
#include <stdexcept>

#include <vector>
#include <functional>
#include <cmath>
#include <stdexcept>

std::vector<std::function<double(double)>> basisSet(int type) {
    using f = std::function<double(double)>;

    switch(type) {

        // Polynomial Deg 2
        case 1:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return x*x; },
                [](double x){ return 0.0; }
            };

        // Polynomial Deg 3
        case 2:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return x*x; },
                [](double x){ return x*x*x; }
            };

        // Legendre Deg 2
        case 3:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return (3*x*x - 1)/2.0; },
                [](double x){ return 0.0; }
            };

        // Legendre Deg 3
        case 4:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return (3*x*x - 1)/2.0; },
                [](double x){ return (5*x*x*x - 3*x)/2.0; }
            };

        // Hermite Deg 2
        case 5:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return x*x - 1.0; },
                [](double x){ return 0.0; }
            };

        // Hermite Deg 3
        case 6:
            return {
                [](double x){ return 1.0; },
                [](double x){ return x; },
                [](double x){ return x*x - 1.0; },
                [](double x){ return x*x*x - 3*x; }
            };

        // Laguerre Deg 2
        case 7:
            return {
                [](double x){ return std::exp(-x/2.0); },
                [](double x){ return std::exp(-x/2.0)*(1 - x); },
                [](double x){ return std::exp(-x/2.0)*(1 - 2*x + x*x/2.0); },
                [](double x){ return 0.0; }
            };

        // Laguerre Deg 3
        case 8:
            return {
                [](double x){ return std::exp(-x/2.0); },
                [](double x){ return std::exp(-x/2.0)*(1 - x); },
                [](double x){ return std::exp(-x/2.0)*(1 - 2*x + x*x*0.5); },
                [](double x){ return std::exp(-x/2.0)*(1 - 3*x + 1.5*x*x - x*x*x/6.0); }
            };

        default:
            throw std::invalid_argument("Invalid basis type");
    }
}

std::vector<double> regress(const std::vector<double>& X, const std::vector<double>& Y, int regType) {
    const int n = X.size();

    auto basis = basisSet(regType);

    double B11 = 0.0;
    double B12 = 0.0;
    double B13 = 0.0;
    double B14 = 0.0;
    double B22 = 0.0;
    double B23 = 0.0;
    double B24 = 0.0;
    double B33 = 0.0;
    double B34 = 0.0;
    double B44 = 0.0;
    double S0 = 0.0;
    double S1 = 0.0;
    double S2 = 0.0;
    double S3 = 0.0;

    for (int i = 0; i < n; i++) {
        double x = X[i];
        double y = Y[i];
        double bas0 = basis[0](x);
        double bas1 = basis[1](x);
        double bas2 = basis[2](x);
        double bas3 = basis[3](x);
        B11 += bas0 * bas0;
        B12 += bas0 * bas1;
        B13 += bas0 * bas2;
        B14 += bas0 * bas3;
        B22 += bas1 * bas1;
        B23 += bas1 * bas2;
        B24 += bas1 * bas3;
        B33 += bas2 * bas2;
        B34 += bas2 * bas3;
        B44 += bas3 * bas3;
        S0 += bas0 * y;
        S1 += bas1 * y;
        S2 += bas2 * y;
        S3 += bas3 * y;
    }

    Eigen::Matrix4d A;
    A << B11,   B12,  B13,  B14,
         B12,   B22,  B23,  B24,
         B13,   B23,  B33,  B34,
         B14,   B24,  B34,  B44;

    Eigen::Vector4d b(S0, S1, S2, S3);

    Eigen::Vector4d c = A.colPivHouseholderQr().solve(b);

    return {c(0), c(1), c(2), c(3)};
}

void generatePathBlock(
    std::vector<double>& paths,
    int pStart,
    int pEnd,
    double So,
    int N,
    double drift,
    double vol,
    unsigned int seed
) {
    std::mt19937 gen(seed);
    std::normal_distribution<double> d(0.0, 1.0);

    for (int p = pStart; p < pEnd; ++p) {
        double last = So;
        int rowStart = p * N;

        for (int n = 0; n < N; ++n) {
            last *= std::exp(drift + vol * d(gen));
            paths[rowStart + n] = last;
        }
    }
}

std::vector<double> generatePricePathMatrix(
    int P, double So, double dt, int N, double r, double v
) {
    std::vector<double> paths(P * N);

    double drift = (r - 0.5 * v * v) * dt;
    double vol = v * std::sqrt(dt);

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 4;
    }
    numThreads = std::min<unsigned int>(numThreads, P);

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    int baseBlock = P / numThreads;
    int remainder = P % numThreads;

    int start = 0;
    for (unsigned int t = 0; t < numThreads; ++t) {
        int blockSize = baseBlock + (t < remainder ? 1 : 0);
        int end = start + blockSize;

        threads.emplace_back(
            generatePathBlock,
            std::ref(paths),
            start,
            end,
            So,
            N,
            drift,
            vol,
            12345u + t
        );

        start = end;
    }

    for (auto& th : threads) {
        th.join();
    }

    return paths;
}

//############################################
//          PUT IMPLEMENTATION
//############################################

double pricePutOption(
    double So, double T, int N, int P, double r, double v, double K, std::mt19937 &gen, int regType
) {
    double dt = T / N;
    std::vector<double> S = generatePricePathMatrix(P,So,dt,N,r,v);
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


    for (int p = 0; p<P; p++) {
        C[p][N-1] = fmax(K-S[(p*N)+N-1],0);
    }
    
    for (int n= N-2; n>=0; n--) {
        //get X and Y
        X.clear();
        Y.clear();
        itm_indices.clear();

        for (int p = 0; p<P; p++) {
            if (K-S[(p*N)+n] > 0 && C[p][n+1] > 0) {
                    X.push_back(S[(p*N)+n]);
                    Y.push_back(C[p][n+1] * exp(-r*dt));
                    itm_indices.push_back(p);
            }
        }
        //std::cout << X_filtered.size();

        // if it is optimal to exercise nowhere in this step, skip to next step
        if (X.size() == 0) {
            continue;
        }

        
        //here is the part where we determine E() function

        //if there are less that 3 datapoints, assume E() is mean of Y_filtered
        bool useReg = X.size() >2;
        if (useReg) {
            try {
                std::vector<double> solution  = regress(X,Y,regType);
                c_coeff = solution[0];
                b_coeff = solution[1];
                a_coeff = solution[2];
            } catch (const std::runtime_error&) {
                useReg = false;
            }
        }
        
        for (int i = 0; i < itm_indices.size(); i++ ){
            int p = itm_indices[i];
            double intrinsic = fmax(K-S[(p*N)+n],0);
            double expectedContinuance;

            if (useReg) {
                expectedContinuance = c_coeff + (b_coeff * S[(p*N)+n]) + (a_coeff * S[(p*N)+n] * S[(p*N)+n]);
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

std::vector<std::vector<float>> select_random_samples(const std::string& filename, int s, std::mt19937 &gen) {
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

    // Ensure we don't try to sample more rows than exist
    size_t sample_size = std::min(static_cast<size_t>(s), all_data.size());

    // We shuffle the whole vector and then slice the first 's' elements
    std::shuffle(all_data.begin(), all_data.end(), gen);

    // Return the subset
    return std::vector<std::vector<float>>(all_data.begin(), all_data.begin() + sample_size);
}

std::vector<std::vector<float>> read_all_samples(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<std::vector<float>> all_data;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return {};
    }

    // Read first non-empty line and detect whether it's header or data
    std::string first_line;
    while (std::getline(file, first_line)) {
        if (!first_line.empty()) break;
    }

    if (first_line.empty()) return {};

    auto try_parse_line = [](const std::string& ln, std::vector<float>& out) {
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

    // Parse remaining lines
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<float> row_values;
        if (try_parse_line(line, row_values)) {
            all_data.push_back(row_values);
        }
    }

    return all_data;
}

double runTestME(int paths, double stepsPerHour,double riskFreeRate,int samples, std::string dataSet, int seed, int regType) {
    std::mt19937 gen(seed);
    std::vector<std::vector<float>> S = select_random_samples(dataSet,samples,gen);

    double sum = 0;
    
    #pragma omp parallel for
    for (int i = 0; i<S.size(); i++) {
        std::vector<float> row = S[i];
        double T = row[4] / (365*24*60);
        int N = (stepsPerHour * 365 * 24) * T;
        sum += (std::abs(pricePutOption(row[0],T,N,paths,riskFreeRate,row[3],row[1],gen,regType) - row[2]) / row[2])*100;

    }

    return sum/samples;
}

int main() {
    double riskFreeRate = 0.04;

    /*
    TYPES
    1 = Polynomial Deg 2
    2 = Polynomial Deg 3
    3 = Leandre Deg 2
    4 = Leandre Deg 3
    5 = Hermite Deg 2
    5 = Hermite Deg 3
    6 = Laguerre Deg 2
    7 = Laguerre Deg 3
    */
    int numSamples = 5;
    std::vector<double> regSched = {1,2,3,4,5,6,7};
    std::vector<double> pathSched = {1000,10000,100000,1000000};
    std::vector<double> stepSched = {1};
    int seed = 10;//current_minute();

    //PRESENT DATA SET CHOICE
    std::string dataSet = "TestData/FridayPutSpread_MSFT-ITM.csv";

    std::ofstream file("tests/ITM_T6.csv", std::ios::app);
    for (int z = 0; z<pathSched.size(); z++) {
        for (int n = 0; n < stepSched.size(); n++) {
            for (int a = 0; a < regSched.size(); a++) {
                    
                    
                auto start = std::chrono::high_resolution_clock::now();
                double result = runTestME(pathSched[z],stepSched[n],riskFreeRate,numSamples,dataSet,seed,regSched[a]);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;

                file << pathSched[z] << "," << stepSched[n] << "," << result << "," << elapsed.count()/numSamples << "," << regSched[a] << "\n";
                file.flush();
            }
        }
    }
     
    file.close();
    return 0;
}








