"""
Example usage of the carlo_pricer module.
Make sure to build the extension first using:
    python setup.py build_ext --inplace
"""

import carlo_pricer

# Example: Price a European Put Option
# Parameters:
# So = 100          # Initial stock price
# T = 1.0           # 1 year to expiration
# N = 50            # 50 time steps
# P = 10000         # 10,000 price paths
# r = 0.05          # 5% risk-free rate
# v = 0.2           # 20% volatility
# K = 100           # Strike price

put_price = carlo_pricer.price_put_option(
    So=100,     # Initial stock price
    T=1.0,      # Time to expiration
    N=50,       # Number of time steps
    P=10000,    # Number of price paths
    r=0.05,     # Risk-free rate
    v=0.2,      # Volatility
    K=100       # Strike price
)

print(f"Put Option Price: ${put_price:.2f}")

# Try with different parameters
atm_put = carlo_pricer.price_put_option(100, 0.5, 50, 5000, 0.03, 0.25, 100)
print(f"ATM Put (6 months, 25% vol): ${atm_put:.2f}")

otm_put = carlo_pricer.price_put_option(100, 1.0, 100, 10000, 0.05, 0.15, 95)
print(f"OTM Put (1 year, 15% vol): ${otm_put:.2f}")
