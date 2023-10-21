# Survey predictors correlations

import csv
import numpy as nmp
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

print("Derived_Predictor_Correlations predictors: hometeam_win, relative_payroll, relative_record, team_history, team_momentum",\
      HOMETEAM_WIN_PRED_RATE, RLTV_PAYROLL_PRED_RATE, RLTV_RECORD_PRED_RATE,\
      TEAM_HISTORY_PRED_RATE, TEAM_MOMENTUM_PRED_RATE)

print("Derived_Predictor_Correlations predictor rates sum, min, max:",
      SUM_OF_PREDICTOR_RATES,MIN_PREDICTOR_RATE,MAX_PREDICTOR_RATE)

period_data = PeriodData()
period_data.season_data = []

for datafiles in RawWorkDataFiles:
    print("\n")
    WorkDataFile = datafiles[1]
    csvworkfile = open(WorkDataFile, 'r', newline='') #The data set we will reference many times
    workobj = csv.reader(csvworkfile)

    year = YearFromFileName(WorkDataFile)
    season_data   = SeasonData(year)
    predictors    = PredictorData(year)
    num_processed = 0
    num_examined  = 0

    num_processed += DiscardEarlyEntrys(workobj)

    for item in workobj:
        game_log_entry = GameLogEntryWork(item)
        num_processed += 1
        if (ExtractPredictorData(season_data, predictors, game_log_entry) == True):
            ExtractGameData(season_data, game_log_entry)
            num_examined += 1

    csvworkfile.close()

    print("WorkDataFile=", WorkDataFile, "records scanned=", num_processed,\
          "records processed=", num_examined)

    EvaluatePredictors(season_data, predictors, num_examined)

    period_data.season_data.append(season_data)
    # 

CollatePeriodPredictorResults(period_data)

print("\nEvaluateCorrelations(...)")
correlations = Ratios_Zeros_Array.copy()
EvaluateCorrelations(predictors.correlations_nd, correlations)
PrintMatrixRatios("Correlation", correlations)

print("\nInvertMatrixRatios(...)")
InvertMatrixRatios(correlations)
# PrintMatrixRatios("Correlation",correlations)

print("\nConvertCor2Ind(...)")
divergences = Ratios_Zeros_Array.copy()
ConvertCor2Ind(correlations, divergences)
PrintMatrixRatios("Divergence", divergences)
