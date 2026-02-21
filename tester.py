from monteCarloAlgorithm import MonteCarloAlgorithm
import matplotlib.pyplot as plt
import pandas as pd


class Tester:

    def runTest(paths,stepsPerYear,riskFreeRate,numSamples,dataSet,parity,ITMonly=False,OTMonly=False):
        samples = Tester.getSamples(numSamples,dataSet,ITMonly,OTMonly)

        if parity not in ["call","put"]:
            print("Parity must be call or put")
            return None
        
        results = pd.DataFrame(columns=["prediction","target","error","percentError","moneyness","time"])
        for row in samples.itertuples():
            prediction, target, error, moneyness, time = Tester.testRow(row,paths,stepsPerYear,riskFreeRate,parity)
            results.loc[len(results)] = [prediction, target, error, error/target, moneyness, time]
        
        return results, {"paths":paths,
                         "stepsPerYear":stepsPerYear,
                         "riskFreeRate":riskFreeRate,
                         "numSamples":numSamples,
                         "dataSet":dataSet,
                         "parity":parity}
        
    
    def testRow(row,paths,stepsPerYear,riskFreeRate,parity):
        T = row.minutesUntilExpiration / (365*24*60)
        steps = int(T * stepsPerYear)
        prediction = MonteCarloAlgorithm.priceOption(row.stockPrice,T,steps,paths,riskFreeRate,row.impliedVolatility,row.strike,parity)
        return prediction, row.optionPrice, abs(prediction - row.optionPrice), row.moneyness, T

    #implement ability to save analysis in csv
    def analyzeResults(results,params,showGraph=False,saveStatistics=False):
        ### STATISTICS ###

        meanPercentError = results["percentError"].mean()
        medianPercentError = results["percentError"].median()
        correlationOfMoneynessAndError = results["moneyness"].corr(results["percentError"])
        correlationOfTimeAndError = results["time"].corr(results["percentError"])

        ###TERMINAL RESPONSE###
        results_summary = f"""
        ### TEST RESULTS ###

        ## PARAMETERS ##
        The test used {params['paths']} paths for each of the {params['numSamples']} samples
        A risk free rate of {params['riskFreeRate']} was used
        There were {params['stepsPerYear']} steps per year for each simulation
        The test was run on the dataset {params['dataSet']} of {params['parity']}s

        ## STATISTICS ##
        The percent error had a mean of {meanPercentError} and a median of {medianPercentError}
        The correlation of moneyness to percent error was {correlationOfMoneynessAndError}
        The correlation of time to expiration to percent error was {correlationOfTimeAndError}
        """

        if not showGraph:
            return results_summary

        ### PLOT DATA ###

        # 1. Create the figure and a 1x2 grid of axes
        # sharey=True links the Y-axes together
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5), sharey=True)

        # 2. Plot on the first axis (left)
        ax1.scatter(results['moneyness'], results['percentError'], color='blue', alpha=0.5)
        ax1.set_title('Plot 1: Moneyness vs Percent Error')
        ax1.set_xlabel('Moneyness in Dollars')
        ax1.set_ylabel('Percent Error') # This label will appear on the far left

        # 3. Plot on the second axis (right)
        ax2.scatter(results['time'], results['percentError'], color='red', alpha=0.5)
        ax2.set_title('Plot 2: Time to Expiration vs Percent Error')
        ax2.set_xlabel('Time in Years')

        # 4. Clean up layout
        plt.tight_layout()
        plt.show()

        return results_summary


    def getSamples(numSamples,dataSet,ITMonly,OTMonly):
        df = pd.read_csv('TestData/' + dataSet + '.csv')
        if ITMonly:
            return df[df["moneyness"]>1].sample(n=numSamples)
        if OTMonly:
            return df[df["moneyness"]<1].sample(n=numSamples)
        samples = df.sample(n=numSamples)#, random_state=42)
        return samples

    def launchAnalystTerminal():
        ITMonly = False
        OTMonly = False
        print("How many paths?")
        paths = int(input())
        print("How many samples?")
        samples = int(input())
        print("ITM or OTM only? (ITM,OTM,No)")
        choice = input()
        if choice == "ITM":
            ITMonly = True
        elif choice == "OTM":
            OTMonly = True
        print("How many steps per hour?")
        stepsPerHour = int(input())

        results, params = Tester.runTest(paths,365*24*stepsPerHour,0.04,samples,"FridayPutSpread_MSFT","put",ITMonly,OTMonly)
        print(Tester.analyzeResults(results,params))

    def runAnalysis(paths,samples,stepsPerHour,ITMonly,OTMonly):
            results, params = Tester.runTest(paths,365*24*stepsPerHour,0.04,samples,"FridayPutSpread_MSFT","put",ITMonly,OTMonly)
            return Tester.analyzeResults(results,params)
    
    def analyzeVariedPathCount(testPathCounts,samples,stepsPerHour,ITMonly,OTMonly):
        results = dict.fromkeys(testPathCounts)
        for paths in testPathCounts:
            sampleResults, params = Tester.runTest(paths,365*24*stepsPerHour,0.04,samples,"FridayPutSpread_MSFT","put",ITMonly,OTMonly)
            print(sampleResults["percentError"])
            results[paths] = sampleResults
        
        data = []
        for paths in testPathCounts:
            newData = [paths,results[paths]["percentError"].mean(),results[paths]["percentError"].median()]


        # Extract x, y1, and y2 using list comprehensions
        x_vals = [row[0] for row in data]
        y1_vals = [row[1] for row in data]
        y2_vals = [row[2] for row in data]

        # Plot both lines on the same graph
        plt.plot(x_vals, y1_vals, label='$y_1$', marker='o', linestyle='-')
        plt.plot(x_vals, y2_vals, label='$y_2$', marker='s', linestyle='--')

        # Add labels and legend
        plt.xlabel('$x$')
        plt.ylabel('$y$ Values')
        plt.title('Comparison of $y_1$ and $y_2$')
        plt.legend()
        plt.grid(True)

        # Save the plot
        plt.savefig('plot.png')
        plt.show()
        print(data)

    def analyzeVariedStepCount(paths,samples,testStepsPerHour,ITMonly,OTMonly):
        pass

Tester.analyzeVariedPathCount([25,50,100,150,200,250],50,60,False,False)