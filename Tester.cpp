#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <omp.h>

#include "Pricers.h"

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

//##########################################################
    //    TESTING
//###########################################################

double runTest(int numPaths, int stepsPerYear, std::string dataSet, int regType, int optType) {
    std::vector<std::vector<float>> S = read_all_samples(dataSet);

    double sum = 0; 
    
    for (int i = 0; i<S.size(); i++) {
        std::vector<float> row = S[i];

        double T = row[4];
        int N = T * stepsPerYear;
        double Stock_Price = row[0];
        double r = row[1];
        double vol = row[6];
        double K = row[2];
        double real_price = row[7];

        double price = 0;
        switch (optType) {
            case 1: price = priceAmericanCall(Stock_Price, T, N, numPaths, r, vol, K, regType); break;
            case 2: price = priceAmericanPut(Stock_Price, T, N, numPaths, r, vol, K, regType);  break;
            case 3: price = priceEuropeanCall(Stock_Price, T, numPaths, r, vol, K);    break;
            case 4: price = priceEuropeanPut(Stock_Price, T, numPaths, r, vol, K);     break;
        }

        sum += (std::abs(price - real_price) / real_price) * 100;
    }

    return sum/S.size();
}


int main() {

    /*
    REGRESSION TYPES
    1 = Polynomial Deg 2
    2 = Polynomial Deg 3
    3 = Leandre Deg 2
    4 = Leandre Deg 3
    5 = Hermite Deg 2
    5 = Hermite Deg 3
    6 = Laguerre Deg 2
    7 = Laguerre Deg 3
    */
    int regType = 1;

    /*
    OPTION TYPES
    1 = American Call
    2 = American Put
    3 = European Call
    4 = European Put
    */

    int optType = 4;

    // Price simulation settings:

    int numPaths = 100000;
    int stepsPerYear = 365;

    // Location of test data csv dataframe with collumns:
    // Stock_Price,Risk_Free_Rate,Strike,Days_to_exp,Years_to_exp,Moneyness,Volatility,Actual_Price

    std::string testSet = "Validation/EuropeanPuts.csv";

    // Run Validation test: returns avg error

    std::cout  << runTest(numPaths, stepsPerYear, testSet, regType, optType);
    

}






