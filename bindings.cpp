#include <pybind11/pybind11.h>
#include "CarloPricer.cpp"

PYBIND11_MODULE(carlo_pricer, m) {
    m.doc() = "Monte Carlo Option Pricing Module";
    
    m.def("price_put_option", &pricePutOption, 
        pybind11::arg("So"),
        pybind11::arg("T"),
        pybind11::arg("N"),
        pybind11::arg("P"),
        pybind11::arg("r"),
        pybind11::arg("v"),
        pybind11::arg("K"),
        R"pbdoc(
            Price a European Put Option using Monte Carlo simulation with Longstaff-Schwartz algorithm.
            
            Args:
                So (float): Initial stock price
                T (float): Time to expiration (in years)
                N (int): Number of time steps
                P (int): Number of price paths
                r (float): Risk-free interest rate
                v (float): Volatility (annualized)
                K (float): Strike price
                
            Returns:
                float: The estimated put option price
        )pbdoc"
    );
}
