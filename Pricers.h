#pragma once

double priceAmericanPut(
    double So, double T, int N, int P, double r, double v, double K, int regType
);

double priceAmericanCall(
    double So, double T, int N, int P, double r, double v, double K, int regType
);

double priceEuropeanCall(
    double So, double T, int P, double r, double v, double K
);

double priceEuropeanPut(
    double So, double T, int P, double r, double v, double K
);