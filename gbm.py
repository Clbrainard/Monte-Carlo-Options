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


