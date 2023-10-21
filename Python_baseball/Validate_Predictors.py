# Validate the derived predictors 

import csv
import numpy as nmp
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

print ("Predictor efficiencies", SUM_OF_PREDICTOR_RATES,\
       MIN_PREDICTOR_RATE, MAX_PREDICTOR_RATE,"\n")
print ("Prediction weights", PREDICTION_WEIGHTS, FltFormat(SUM_OF_PREDICTION_WEIGHTS))
assert (round(SUM_OF_PREDICTION_WEIGHTS, 3) == Float1), "Weights don't sum to 1.000"

csvworkfile = open(WorkDataFile, 'r', newline='') #The data set we will reference many times
workobj = csv.reader(csvworkfile)

year = YearFromFileName(WorkDataFile)
season_data    = SeasonData(year)
predictors     = PredictorData(year)
num_processed  = 0
num_examined   = 0

num_processed += DiscardEarlyEntrys(workobj)

for item in workobj:
    game_log_entry = GameLogEntryWork(item)
    num_processed += 1
    if (ExtractPredictorData(season_data, predictors, game_log_entry) == True):
        ExtractGameData(season_data, game_log_entry)
        num_examined += 1

csvworkfile.close()

print("\nWorkDataFile=", WorkDataFile, "records scanned=", num_processed,\
      "records processed=", num_examined)

EvaluatePredictors(season_data, predictors, num_examined)

correlations = Ratios_Zeros_Array.copy()
EvaluateCorrelations(predictors.correlations_nd, correlations)
PrintMatrixRatios(correlations)
