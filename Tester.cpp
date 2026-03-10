#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <string>
#include <cmath>
#include <vector>
#include <iterator>
#include "Eigen/Dense"
#include <omp.h>
#include <ctime>
#include <chrono>

int current_minute() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* local = std::localtime(&t);
        return local->tm_min;
}

std::vector<double> solveQuadraticLLT(
    double n,
    double Sx,
    double Sx2,
    double Sx3,
    double Sx4,
    double Sy,
    double Sxy,
    double Sx2y
) {
    Eigen::Matrix3d A;
    A << n,   Sx,  Sx2,
         Sx,  Sx2, Sx3,
         Sx2, Sx3, Sx4;

    Eigen::Vector3d b(Sy, Sxy, Sx2y);

    Eigen::LLT<Eigen::Matrix3d> llt(A);
    if (llt.info() != Eigen::Success) {
        throw std::runtime_error("Cholesky failed (matrix not SPD)");
    }

    Eigen::Vector3d c = llt.solve(b);

    return { c(0), c(1), c(2) };
}

std::vector<double> quadRegress(std::vector<double> X, std::vector<double> Y) {
    int n = X.size();
    double Sx = 0;
    double Sx2 = 0;
    double Sx3 = 0;
    double Sx4 = 0;
    double Sx5 = 0;
    double Sy = 0;
    double Sxy = 0;
    double Sx2y = 0;
    for (size_t i = 0; i < n; ++i) {
        const double xi = X[i];
        const double yi = Y[i];

        double x_pow = xi;        // x
        Sx += x_pow;

        x_pow *= xi;              // x^2
        Sx2 += x_pow;
        Sx2y += x_pow * yi;

        x_pow *= xi;              // x^3
        Sx3 += x_pow;

        x_pow *= xi;              // x^4
        Sx4 += x_pow;

        x_pow *= xi;              // x^5
        Sx5 += x_pow;

        Sy  += yi;
        Sxy += xi * yi;
    }
    Eigen::Matrix3d A;
    A << n,   Sx,  Sx2,
         Sx,  Sx2, Sx3,
         Sx2, Sx3, Sx4;

    Eigen::Vector3d b(Sy, Sxy, Sx2y);

    Eigen::LLT<Eigen::Matrix3d> llt(A);
    if (llt.info() != Eigen::Success) {
        throw std::runtime_error("Cholesky failed (matrix not SPD)");
    }

    Eigen::Vector3d c = llt.solve(b);

    return { c(0), c(1), c(2) };

}

std::vector<std::vector<double>> generatePricePathMatrix(int P, double So, double dt, int N, double r, double v, std::mt19937 &gen) {
    std::vector<std::vector<double>> paths(P, std::vector<double>(N, 0.0));

    // Pre-calculate constant terms to save clock cycles inside the loop
    double drift = (r - 0.5 * v * v) * dt;
    double vol = v * std::sqrt(dt);

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

//############################################
//          PUT IMPLEMENTATION
//############################################

double pricePutOption(double So, double T, int N, int P, double r, double v, double K, std::mt19937 &gen) {
    double dt = T / N;
    std::vector<std::vector<double>> S = generatePricePathMatrix(P,So,dt,N,r,v,gen);
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
        C[p][N-1] = fmax(K-S[p][N-1],0);
    }
    
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
                std::vector<double> solution  = quadRegress(X,Y);
                c_coeff = solution[0];
                b_coeff = solution[1];
                a_coeff = solution[2];
            } catch (const std::runtime_error&) {
                useReg = false;
            }
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

double runTestME(int paths, int stepsPerHour,double riskFreeRate,int samples, std::string dataSet, std::mt19937 &gen, std::vector<std::vector<float>> S) {
    double sum = 0;
    
    #pragma omp parallel for
    for (int i = 0; i<S.size(); i++) {
        std::vector<float> row = S[i];
        double T = row[4] / (365*24*60);
        int N = (stepsPerHour * 365 * 24) * T;
        sum += (std::abs(pricePutOption(row[0],T,N,paths,riskFreeRate,row[3],row[1],gen) - row[2]) / row[2])*100;

    }

    return sum/samples;
}

int main() {
    std::cout << "Starting program" << std::endl;
    double riskFreeRate = 0.04;
    /*
    int paths;
    int samples;
    int stepsPerHour;

    std::cout << "How many paths?" << std::endl;
    std::cin >> paths;

    std::cout << "How many samples?" << std::endl;
    std::cin >> samples;

    std::cout << "How many steps per hour?" << std::endl;
    std::cin >> stepsPerHour;
    */
   

    int numTests = 10;
    int numSamples = 30;
    std::vector<double> pathSched = {10,50,100,250,500,1000,5000};
    std::vector<double> stepSched = {5,10,15,30,60};

    //PRESENT DATA SET CHOICE
    std::string dataSet = "TestData/FridayPutSpread_MSFT.csv";

    for (int t = 0; t<numTests; t++) {
        std::mt19937 gen(current_minute());
        std::vector<std::vector<float>> batch = select_random_samples(dataSet,numSamples,gen);
        for (int z = 0; z<8; z++) {
            std::ofstream file("data.csv", std::ios::app);
            for (int n = 0; n<5; n++) {
                auto start = std::chrono::high_resolution_clock::now();
                double result = runTestME(pathSched[z],stepSched[n],riskFreeRate,numSamples,dataSet,gen,batch);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;

                std::cout << "Path count: " << pathSched[z] << "; Step per min: " << stepSched[n] << "; Mean error of: " << result << "; In " << elapsed.count() << " seconds \n";
                file << pathSched[z] << "," << stepSched[n] << "," << result << "," << elapsed.count() << "\n";
            }
        }
    }

    return 0;
}

