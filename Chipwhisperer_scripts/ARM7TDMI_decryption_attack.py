import chipwhisperer as cw
from chipwhisperer.analyzer.attacks.cpa import CPA
from chipwhisperer.analyzer.attacks.cpa_algorithms.progressive import CPAProgressive
from chipwhisperer.analyzer.attacks.models.AES128_8bit import *
from chipwhisperer.analyzer.preprocessing.add_noise_random import AddNoiseRandom

traces = self.project.traceManager()

attack = CPA()
leak_model = AES128_8bit(InvSBox_output) # inv sbox works on T-Box Decryption as well
attack.setAnalysisAlgorithm(CPAProgressive, leak_model)
#attack.setTraceSource(traces)
attack.setTraceSource(self.ppmod[0])

attack.setTraceStart(0)
attack.setTracesPerAttack(-1)
attack.setIterations(1)
attack.setReportingInterval(10)
attack.setTargetSubkeys([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
attack.setPointRange((48000,50400)) # attack last round which occurs 1st, for decryption

self.results_table.setAnalysisSource(attack)
self.correlation_plot.setAnalysisSource(attack)
self.output_plot.setAnalysisSource(attack)
self.pge_plot.setAnalysisSource(attack)
attack.processTraces()
