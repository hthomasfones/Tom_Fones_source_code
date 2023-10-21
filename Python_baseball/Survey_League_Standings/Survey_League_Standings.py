# Look at the runs-per-game data
# This program studies the 'Earl Weaver' method... 
# that the winning team scores more runs in one inning than the losing team
# scores in the whole game (much) of the time
#  
import sys
import csv
import matplotlib
import matplotlib.pyplot as plt
import numpy as nmp
import math
import scipy.stats as stats
#
sys.path.append( "..\\")
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

period_summary = PeriodSummary(1900)
num_seasons = 0
RawWorkDataFiles = Current_Era

for datafiles in RawWorkDataFiles: # for each year
    WorkDataFile = datafiles[1]
    csvworkfile = open(WorkDataFile, 'r', newline='') #The data set we will reference many times
    workobj = csv.reader(csvworkfile)

    year = YearFromFileName(WorkDataFile)
    season_summary = SeasonSummary(year)
    season_data = SeasonData(year)
    num_seasons += 1
    num_processed = 0
    num_examined = 0
    #num_processed += DiscardEarlyEntrys(workobj)

    for item in workobj:
        game_log_entry = GameLogWorkEntry(item)
        if (game_log_entry.league == "FL"): continue

        num_processed += 1
        if (ExtractGameData(season_data, game_log_entry) == True):
            num_examined += 1

    csvworkfile.close()

    PrintSummary(WorkDataFile, num_processed, num_examined)

    season_team_data = DeriveSeasonSummary(season_data, year)
    for team_data in season_team_data:
        period_summary.team_data.append(team_data)

sorted_period_list = sorted(period_summary.team_data, key=TeamWinRatio) #reverse=True)

print("\nSummary of multi-year period...")
PrintSeasonSummary(sorted_period_list, year)

# Build a 1D array
Win_Ratios = []
for team_obj in sorted_period_list: Win_Ratios.append(team_obj.win_ratio)

# array_stats = (Size, Min, Max, Avg, Mdn, StdDev, Var)
wr_stats = ComputeArrayStats(Win_Ratios, True)

# Standard Prob-Dist-Func values for Sorted_X, Avg, StdDev
Yvalues = stats.norm.pdf(Win_Ratios, wr_stats[AR_AVG_DX], wr_stats[AR_STD_DX])

plt.xlim = (Float0, Float1)
plt.ylim(0, len(Win_Ratios)/NUM_TEAMS)
plt.yticks(nmp.arange(0, len(Win_Ratios)/NUM_TEAMS, 1))
#plt.xticks(nmp.arange(Float0, Float1, .050))
StdDev = wr_stats[AR_STD_DX]
Avg = wr_stats[AR_AVG_DX]
plt.xticks(nmp.arange(-(Avg - StdDev)*2, (Avg - StdDev)*2, StdDev))
plt.plot(Win_Ratios, Yvalues)
plt.xlabel("Winning percentage")
plt.ylabel("Distribution: over 30 teams X 10 seasons")
plt.title("Winning percentage distribution: 2010 - 2019")
plt.show()
