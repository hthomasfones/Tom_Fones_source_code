# Extract data from a large set (160+ fields) of comma-separated-variables
# and save the useful survey data   

import csv
from Baseball_Constants import *
from Baseball_Class_Defs import *
from Baseball_Function_Defs import *

for datafiles in RawWorkDataFiles:
    RawDataFile = datafiles[0]
    WorkDataFile = datafiles[1]

    rawcsvfile = open(RawDataFile, 'r', newline='')   # Our input data
    csvworkfile = open(WorkDataFile, 'w', ) #The data set we will reference many times
    rawobj = csv.reader(rawcsvfile)

    entrynum = 0
    for item in rawobj:
        game_log_entry = GameLogEntry(item)
        if (game_log_entry.league == "FL"): continue

        #print("Entry# ", entrynum, \
        #      "Date=", game_log_entry.date, "Location=", game_log_entry.hometeam)
        entrynum += 1
        csvworkfile.writelines([game_log_entry.date,",",game_log_entry.league,",", \
                                game_log_entry.duration,",",game_log_entry.outs,",",\
                                game_log_entry.visitor, ",", game_log_entry.vstr_score,",", \
                                game_log_entry.vstr_runs_by_inn,",",game_log_entry.vstr_game_num,",",\
                                game_log_entry.vstr_stolen_bases,",", \
                                game_log_entry.hometeam, ",", game_log_entry.home_score,",", \
                                game_log_entry.home_runs_by_inn,",", game_log_entry.home_game_num,",",
                                game_log_entry.home_stolen_bases, "\n"])

    rawcsvfile.close()
    csvworkfile.close()
    print("Extract_Season_Data...",RawDataFile, "-->", WorkDataFile, \
          " entries processed", entrynum)
