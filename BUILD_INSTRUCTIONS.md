# Building the Python Binding for Carlo Pricer

This guide explains how to build and use the pybind11 extension module for the Monte Carlo option pricing library.

## Prerequisites

You'll need the following installed:
- Python 3.6+ 
- C++ compiler (g++ with C++17 support - you already have MinGW)
- pybind11
- setuptools and wheel

## Installation Steps

### 1. Install Dependencies

Open PowerShell and run:

```powershell
pip install pybind11 setuptools wheel
```

### 2. Build the Extension Module

From the project root directory (c:\Users\clbra\Documents\GitHub\Monte-Carlo-Options), run:

```powershell
python setup.py build_ext --inplace
```

This will:
- Compile `bindings.cpp` and `CarloPricer.cpp` with your C++ compiler
- Create `carlo_pricer.cp*.pyd` file (the Python extension module)
- Place it in the current directory so you can import it directly

### 3. Verify Installation

Test that the module works:

```powershell
python -c "import carlo_pricer; print(carlo_pricer.price_put_option(100, 1, 50, 1000, 0.05, 0.2, 100))"
```

You should see a price output (a number).

## Using the Module

Once built, you can import and use it in any Python script:

```python
import carlo_pricer

# Price a put option
price = carlo_pricer.price_put_option(
    So=100,     # Initial stock price
    T=1.0,      # Time to expiration (years)
    N=50,       # Number of time steps
    P=10000,    # Number of price paths
    r=0.05,     # Risk-free rate
    v=0.2,      # Volatility
    K=100       # Strike price
)

print(f"Put Price: ${price:.2f}")
```

## Troubleshooting

### Compiler not found
- Ensure MinGW is in your PATH
- Check: `g++ --version`

### pybind11 not found
- Install it: `pip install pybind11`

### Module import fails
- Make sure you ran `python setup.py build_ext --inplace`
- Check that the `.pyd` file was created in the project directory

### DLL not found errors
- You may need to add MinGW bin to PATH:
  ```powershell
  $env:PATH += ";C:\MinGW\bin"
  ```

## Rebuilding

If you make changes to the C++ code, rebuild with:

```powershell
python setup.py build_ext --inplace --force
```

## Next Steps

See `example_usage.py` for more examples of how to call the pricing function.
