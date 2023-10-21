# Examine the data for one season and deriver predictor values

import sys
import csv

sys.path.append( "..\\")
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

print("Derive_Predictors assumed_constants: Relative_payroll_threshold=",RLTV_PAYROLL_THRESHOLD,\
      " relative_record_min_games=",RLTV_RECORD_MIN_GAMES," relative_record_threshold=",RLTV_RECORD_THRESHOLD,\
      " recent_history_len=",RECENT_HISTORY_LEN," momentum_threshold=", MOMENTUM_THRESHOLD) 

period_data = PeriodData()
period_data.season_data  = []
RawWorkDataFiles = Current_Era

for datafiles in RawWorkDataFiles:
    WorkDataFile = datafiles[1]
    csvworkfile = open(WorkDataFile, 'r', newline='') #The data set we will reference many times
    workobj = csv.reader(csvworkfile)

    year = YearFromFileName(WorkDataFile)
    season_data = SeasonData(year)
    num_processed = 0
    num_examined = 0
    num_processed += DiscardEarlyEntrys(workobj)

    for item in workobj:
        game_log_entry = GameLogWorkEntry(item)
        num_processed += 1
        # print("Record#", num_examined, "Gl_Entry=", item)
        if (ExtractGameData(season_data, game_log_entry) == True):
            num_examined += 1

        # PrintGlEntry(num_processed, game_log_entry)

    csvworkfile.close()

    print("\nWorkDataFile=", WorkDataFile, "records scanned=", num_processed,\
          "records processed=", num_examined)

    # PrintSeasonStats(season_data)
    CollateSeasonData(season_data, num_examined)
    DerivePredictors(season_data, num_examined)
    print("======================================")

    season_data.team_data = [] # Not needed going forward
    period_data.season_data.append(season_data)

CollatePeriodData(period_data)
PrintPeriodSummary(period_data)
