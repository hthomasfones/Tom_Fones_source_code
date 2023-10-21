# Look at the runs-per-game data
# This program studies the 'Earl Weaver' method... 
# that the winning team scores more runs in one inning than the losing team
# scores in the whole game (much) of the time
#  
import csv
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

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
        game_log_entry = GameLogEntryWork(item)
        #print("Record#", num_examined, "Gl_Entry=", item)
        ExtractWeaverData(season_data, game_log_entry)
        num_processed += 1
        num_examined += 1

    csvworkfile.close()

    PrintSummary(WorkDataFile, num_processed, num_examined)
    avg_wscore     = season_data.total_winning_runs / num_examined
    avg_lscore     = season_data.total_losing_runs / num_examined
    weaver_ratio   = season_data.weaver_method_gms / num_examined
    steals_per_run = \
    season_data.total_stolen_bases / (season_data.total_winning_runs + season_data.total_losing_runs)
    avg_duration = int(float(season_data.total_duration / num_examined)) 

    print("Average winning score,losing score, runs_per_game, duration... ", \
          FltFormat(avg_wscore),FltFormat(avg_lscore),FltFormat(avg_wscore + avg_lscore), avg_duration, \
          ";  Weaver_premise_games,ratio, steals_per_run...", \
          season_data.weaver_method_gms,FltFormat(weaver_ratio),FltFormat(steals_per_run)) 
