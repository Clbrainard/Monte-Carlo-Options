import QuantLib as ql
import pandas as pd
import numpy as np

df = pd.read_csv("putTestSet.csv")

today = ql.Date.todaysDate()
ql.Settings.instance().evaluationDate = today

calendar = ql.NullCalendar()
day_count = ql.Actual365Fixed()

STEPS_PER_YEAR = 365
NUM_PATHS = 500
SEED = 42
POLYNOM_ORDER = 2          # second-degree polynomial
POLYNOM_TYPE = 0           # 0 = Monomial (1, x, x^2)
CALIBRATION_PATHS = 500

pct_errors = []
rows = []

for _, row in df.iterrows():
    S0 = row["Stock_Price"]
    r  = row["Risk_Free_Rate"]
    K  = row["Strike"]
    T  = row["Years_to_exp"]
    v  = row["Volatility"]
    days = int(round(row["Days_to_exp"]))
    actual = row["Actual_Price"]

    maturity = today + ql.Period(days, ql.Days)
    exercise = ql.AmericanExercise(today, maturity)
    payoff   = ql.PlainVanillaPayoff(ql.Option.Put, K)
    option   = ql.VanillaOption(payoff, exercise)

    spot_handle = ql.QuoteHandle(ql.SimpleQuote(S0))
    rate_ts     = ql.YieldTermStructureHandle(
                      ql.FlatForward(today, ql.QuoteHandle(ql.SimpleQuote(r)), day_count))
    div_ts      = ql.YieldTermStructureHandle(
                      ql.FlatForward(today, ql.QuoteHandle(ql.SimpleQuote(0.0)), day_count))
    vol_ts      = ql.BlackVolTermStructureHandle(
                      ql.BlackConstantVol(today, calendar, ql.QuoteHandle(ql.SimpleQuote(v)), day_count))

    process = ql.BlackScholesMertonProcess(spot_handle, div_ts, rate_ts, vol_ts)

    time_steps = max(1, round(T * STEPS_PER_YEAR))

    engine = ql.MCAmericanEngine(
        process,
        "pseudorandom",
        timeSteps=time_steps,
        requiredSamples=NUM_PATHS,
        seed=SEED,
        polynomOrder=POLYNOM_ORDER,
        polynomType=POLYNOM_TYPE,
        nCalibrationSamples=CALIBRATION_PATHS,
    )
    option.setPricingEngine(engine)

    lsm_price = option.NPV()
    pct_error = abs(lsm_price - actual) / actual * 100
    pct_errors.append(pct_error)

    rows.append({
        "S0": S0,
        "K": K,
        "T": T,
        "Vol": v,
        "Actual":    round(actual,    6),
        "LSM":       round(lsm_price, 6),
        "PctError%": round(pct_error,  4),
    })

results = pd.DataFrame(rows)
pd.set_option("display.max_rows", None)
pd.set_option("display.float_format", "{:.6f}".format)
print(results.to_string(index=False))
print(f"\nMean Percent Error: {np.mean(pct_errors):.4f}%")
