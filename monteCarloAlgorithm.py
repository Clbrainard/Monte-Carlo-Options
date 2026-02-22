
import numpy as np
import math
import matplotlib.pyplot as plt

class GBM:
    
    @staticmethod 
    def generatePricePath(So,dt,N,r,v,rng):
        steps = [So]
        for n in range(N):
            a = (r - (0.5 * (v**2))) * dt
            b = v * math.sqrt(dt) * GBM.Z(rng)
            Sn = steps[n] * math.exp(a+b)
            steps.append(Sn)
        return steps[1:]

    @staticmethod
    def generatePricePathMatrix(P,So,dt,N,r,v):
        paths = []
        rng = np.random.default_rng()
        for p in range(P):
            paths.append(GBM.generatePricePath(So,dt,N,r,v,rng))
        return np.array(paths)

    @staticmethod
    def Z(rng):
        return rng.normal(loc=0.0, scale=1.0)
    
    @staticmethod
    def displayPaths(P,So,dt,N,r,v):
        paths = GBM.generatePricePathMatrix(P,So,dt,N,r,v)

        t = np.arange(paths.shape[1])

        plt.plot(t, paths.T, linewidth=0.8, alpha=0.6)  # transpose so each column becomes a line
        plt.xlabel("Step")
        plt.ylabel("Value")
        plt.title(f"{paths.shape[0]} paths")
        plt.grid(True, alpha=0.3)
        plt.show()

class MonteCarloAlgorithm:
    
    @staticmethod
    def priceOption(So,T,N,P,r,v,K,side="call"):
        
        if side == "call":
            def intrinsic(s):
                return max(s-K,0)
        elif side == "put":
            def intrinsic(s):
                return max(K-s,0)
            
        dt = T/N
        R = math.exp(-r * dt)

        S = GBM.generatePricePathMatrix(P,So,dt,N,r,v)

        C = np.zeros((P, N))

        for p in range(P):
            C[p,N-1] = intrinsic(S[p,N-1])
        

        CF = MonteCarloAlgorithm.fillCashflowTable(S,C,R,P,N,intrinsic)
        return MonteCarloAlgorithm.solvePriceFromCF(CF,R,P,N)


    @staticmethod
    def fillCashflowTable(S,C,R,P,N,intrinsic):
        CF = C.copy()
        for n in range(N-2, -1, -1):
            CF[:,n] = MonteCarloAlgorithm.inductiveStep(S,CF,R,P,n,intrinsic)
        return CF

    @staticmethod
    def inductiveStep(S,CF,R,P,n,intrinsic):
        output = np.zeros(P)
        X = S[:,n]
        Y = CF[:,n+1] * R

        #pop rows where x and y are zero
        #poplist = []
        #for p in range(P):
        #    if intrinsic(X[p]) == 0 and Y[p] == 0:
        #        poplist.append(p)
        #X =  np.delete(X,poplist)
        #Y =  np.delete(Y,poplist)

        # Create a mask where it's NOT the case that both are zero
        # The ~ is the 'NOT' operator, and & is 'AND'
        mask = ~((X == 0) & (Y == 0))

        # Apply the mask to both arrays
        X_filtered = X[mask]
        Y_filtered = Y[mask]

        if X.size == 0 and Y.size ==0:
            return output

        a,b,c = MonteCarloAlgorithm.LSM(X_filtered,Y_filtered)

        def EV(x):
            return a*(x**2) + b*x + c
       
        for p in range(P):
            ES = 0
            if not (X[p]==0 and Y[p]==0):
                ES = EV(S[p,n])
                IS = intrinsic(S[p,n])
                if IS >= ES:
                    output[p] = IS
        return output


    @staticmethod
    def LSM(X,Y):
        if X.size == 1 and Y.size == 1:
            a= 0
            b=0
            c = Y[0]
        if X.size == 2 and Y.size == 2:
            a = 0
            b, c = np.polyfit(X, Y, deg=1)
        else:
            a, b, c = np.polyfit(X, Y, deg=2)
        return a,b,c
    
    @staticmethod
    def solvePriceFromCF(CF,R,P,N):
        discountedSum = 0
        for p in range(P):
            for n in range(N):
                entry = CF[p,n]
                if entry != 0.00:
                    discountedSum += (entry * (R ** (n+1)))
                    break
        return discountedSum / P




