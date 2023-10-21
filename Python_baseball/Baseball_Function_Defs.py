#Define functions for baseball study

import copy
import os
from Baseball_Constants import *
from Baseball_Class_Defs import *

def PrintGlEntry(entrynum, GL_ENTRY):
    print("Entry#", entrynum, "...", GL_ENTRY.date, GL_ENTRY.outs,      \
          GL_ENTRY.visitor, GL_ENTRY.vstr_score, GL_ENTRY.vstr_runs_by_inn,   \
          GL_ENTRY.hometeam, GL_ENTRY.home_score, GL_ENTRY.home_runs_by_inn)

# Discard a set of records in a CSV object  
# workobj = csv.reader(csvworkfile)
def DiscardEarlyEntrys(Work_Obj):
    lim = NUM_GAMES_PRE_NORMALIZE * NUM_TEAMS / 2
    numentrys = 0
    for item in Work_Obj:
        numentrys += 1
        if (numentrys == lim): break

    #print ("Early entrys discarded =", numentrys)
    return numentrys

# Accumulate enough data for an N game history for all teams
# workobj = csv.reader(csvworkfile)
# Unused
def LoadTeamHistories(Ssn_Data, Work_Obj):
    numentrys = 0
    for item in Work_Obj:
        game_log_entry = GameLogEntryWork(item)
        #PrintGlEntry(numentrys, game_log_entry)
        ExtractGameData(Ssn_Data, game_log_entry)
        numentrys += 1
        if AllHistorysLoaded(Ssn_Data) : break

    print ("Num entrys processed for team histories =", numentrys)
    return numentrys

# For all teams - if any team's game history is incomplete then return False
# Unused
def AllHistorysLoaded(Ssn_Data):
    for team_data in Ssn_Data.team_data:
        if (team_data.game_hist[0] == 0): return False
    return True

# data from one game for both teams
def ExtractGameData(Ssn_Data, GL_Entry):

    # Weed out noise - the Federal League
    vstr_dx = AssignTeamIndx(GL_Entry.visitor)
    home_dx = AssignTeamIndx(GL_Entry.hometeam) 
    if ((home_dx == -1) or (vstr_dx == -1)): # team not recognized
         return False

    home_score = int(GL_Entry.home_score)
    vstr_score = int(GL_Entry.vstr_score)
    Ssn_Data.total_winning_runs += max(home_score, vstr_score)
    Ssn_Data.total_losing_runs  += min(home_score, vstr_score)

    home_win  = (home_score > vstr_score)
    xtra_inns = (int(GL_Entry.outs) > 54)

    vstr_data = Ssn_Data.team_data[vstr_dx]
    home_data = Ssn_Data.team_data[home_dx]

    if (GL_Entry.home_stolen_bases != ''):
        if (int(GL_Entry.home_stolen_bases) > -1):
            home_data.stolen_bases += int(GL_Entry.home_stolen_bases)

    if (GL_Entry.vstr_stolen_bases != ''):
        if (int(GL_Entry.vstr_stolen_bases) > -1):
            vstr_data.stolen_bases += int(GL_Entry.vstr_stolen_bases)

    # Determine if relative payroll makes a correct prediction
    payrolls = Payroll_Pair.copy()
    payrolls = [home_data.payroll, vstr_data.payroll]
    if (sum(payrolls) > 0):
        if (float(max(payrolls)) >= float(min(payrolls) * RLTV_PAYROLL_THRESHOLD)):
            Ssn_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX] += 1

            if ((home_data.payroll > vstr_data.payroll) and (home_win == True)): 
                Ssn_Data.predictor_stats[Cnfrmd][RLTV_PAYROLL_DX] += 1

            if ((home_data.payroll < vstr_data.payroll) and (home_win == False)):
                Ssn_Data.predictor_stats[Cnfrmd][RLTV_PAYROLL_DX] += 1

    # Determine if relative winning percentage makes a good prediction
    # One value per game - not one per team-game
    # Initialize to equal ratios
    home_win_ratio = Float_500
    vstr_win_ratio = Float_500

    if ((home_data.games_sampled >= RLTV_RECORD_MIN_GAMES) and \
        (vstr_data.games_sampled >= RLTV_RECORD_MIN_GAMES)):
        home_win_ratio = float(home_data.wins / home_data.games_sampled)
        vstr_win_ratio = float(vstr_data.wins / vstr_data.games_sampled)

        win_ratio_diff = abs(home_win_ratio - vstr_win_ratio)
        if (win_ratio_diff >= RLTV_RECORD_THRESHOLD):
            Ssn_Data.predictor_stats[SmplSz][RLTV_RECORD_DX] += 1

            if ((home_win_ratio > vstr_win_ratio) and (home_win == True)): 
                Ssn_Data.predictor_stats[Cnfrmd][RLTV_RECORD_DX] += 1

            if ((home_win_ratio < vstr_win_ratio) and (home_win == False)):
                Ssn_Data.predictor_stats[Cnfrmd][RLTV_RECORD_DX] += 1

    # Process game data and update predictor values for each team
    temp_vstr_data = copy.deepcopy(vstr_data)
    ProcessGameDataOneTeam(Ssn_Data, vstr_dx, home_win, False, xtra_inns,
                           (GL_Entry.vstr_score == 0))

    temp_home_data = copy.deepcopy(home_data) 
    ProcessGameDataOneTeam(Ssn_Data, home_dx, home_win, True, xtra_inns,
                           (GL_Entry.home_score == 0))

    # AdjustSeasonCounters(Ssn_Data,temp_vstr_data,temp_home_data,vstr_data,home_data)
    return True

# Update stats in a team's record  based on Win/Loss, Home/Away, Xtra_Innings?, shutout
# momentum continued?, history
def ProcessGameDataOneTeam(Ssn_Data, Team_Dx, HomeWin, Home, Xtra_Inns, Shutout):

    team_data = Ssn_Data.team_data[Team_Dx]
    team_data.games_sampled += 1
    if (Home == True): team_data.predictor_stats[SmplSz][HOME_WIN_DX] += 1
    if (Shutout == True): team_data.shutouts += 1

    if (Xtra_Inns == True):
        team_data.xtra_inn_gms += 1
        if (Home == True): team_data.hm_xtra_inn_gms += 1

    # Is this game a win or loss ?
    # If away & home_loss or if home & home_win - its a win
    if (Home == HomeWin): this_game = 1
    else: this_game = -1

    if (this_game == 1):
        team_data.wins += 1
        if (Home == True):
            team_data.predictor_stats[Cnfrmd][HOME_WIN_DX] += 1
            if (Xtra_Inns == True): team_data.hm_xtra_inn_wins += 1

    prev_game = team_data.game_hist[RECENT_HISTORY_LEN-1]
    if (prev_game == 0): prev_game = -this_game

    # Update the current momentum. Compare this game with the previous game
    # if this game equals the previous game increment momentum
    if (this_game == prev_game): team_data.curr_momentum += 1

    # Do we have momentum to accumulate as a predictor?
    momentum_actv = (abs(team_data.curr_momentum) >= MOMENTUM_THRESHOLD)
    if momentum_actv:
        team_data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX] += 1
        if (this_game == prev_game): team_data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX] += 1

    if (this_game != prev_game): 
        team_data.curr_momentum = 0 # reset momentum
        if momentum_actv: 
            if (prev_game == 1): team_data.num_momentum_win_strks += 1
            if (prev_game == -1): team_data.num_momentum_loss_strks += 1

    # Do we have sufficient history to test - is the oldest entry non-zero?
    # If so can we use history as a predictor?
    hist_val = GetHistoryValue(team_data.game_hist)
    if (hist_val != 0): team_data.predictor_stats[SmplSz][TEAM_HISTORY_DX] += 1
    if (this_game == hist_val): team_data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX] += 1

    # This game becomes the most recent history
    del team_data.game_hist[0]
    team_data.game_hist.append(this_game)
    return

# Adjust for over-counting history and momentum
# For all four counters...
# if they were incremented for both teams then decrement the season total
def AdjustSeasonCounters(Ssn_Data,temp_vstr_data,temp_home_data,vstr_data,home_data):
    if ((vstr_data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX] > temp_vstr_data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX])\
        and 
        (home_data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX] > temp_home_data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX])):
            Ssn_Data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX] -= 1

    if ((vstr_data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX] > temp_vstr_data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX])\
        and
        (home_data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX] > temp_home_data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX])):
            Ssn_Data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX] -= 1

    if ((vstr_data.predictor_stats[SmplSz][TEAM_HISTORY_DX] > temp_vstr_data.predictor_stats[SmplSz][TEAM_HISTORY_DX])\
        and 
        (home_data.predictor_stats[SmplSz][TEAM_HISTORY_DX] > temp_home_data.predictor_stats[SmplSz][TEAM_HISTORY_DX])):
            Ssn_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX] -= 1

    if ((vstr_data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX] > temp_vstr_data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX])\
        and
        (home_data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX] > temp_home_data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX])):
            Ssn_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX] -= 1

    return

# data from one game for both teams
def ExtractPredictorData(Ssn_Data, Predictors, GL_Entry):

    # Weed out noise - the Federal League
    vstr_dx = AssignTeamIndx(GL_Entry.visitor)
    home_dx = AssignTeamIndx(GL_Entry.hometeam) 
    if ((home_dx == -1) or (vstr_dx == -1)): # team not recognized
         return False

    Predictors.total_games_sampled += 1
    one_game_results = Predictor_Zeros_Array.copy()
 
    home_score = int(GL_Entry.home_score)
    vstr_score = int(GL_Entry.vstr_score)
    home_win   = (home_score > vstr_score)

    vstr_data = Ssn_Data.team_data[vstr_dx]
    home_data = Ssn_Data.team_data[home_dx]

    # We always know location so we can always use home_team as a predictor
    # (sample_size = total_games)
    one_game_results[SmplSz][HOME_WIN_DX] = 1
    if (home_win): 
        one_game_results[Cnfrmd][HOME_WIN_DX] = 1

    #Can we use relative payroll as a predictor
    payrolls = [home_data.payroll, vstr_data.payroll]
    if (float(max(payrolls)) >= float(min(payrolls) * RLTV_PAYROLL_THRESHOLD)):
        one_game_results[SmplSz][RLTV_PAYROLL_DX] = 1

        if ((home_data.payroll > vstr_data.payroll) and (home_win == True)): 
            one_game_results[Cnfrmd][RLTV_PAYROLL_DX] = 1

        if ((home_data.payroll < vstr_data.payroll) and (home_win == False)): 
             one_game_results[Cnfrmd][RLTV_PAYROLL_DX] = 1

    #Can we use relative win-loss as a predictor
    rltv_record_trend = 0
    if ((home_data.games_sampled >= RLTV_RECORD_MIN_GAMES) and \
        (vstr_data.games_sampled >= RLTV_RECORD_MIN_GAMES)):
        home_winpc = float(home_data.wins / home_data.games_sampled)
        vstr_winpc = float(vstr_data.wins / vstr_data.games_sampled)

        rltv_record = (home_winpc - vstr_winpc)
        if (abs(rltv_record) >= RLTV_RECORD_THRESHOLD):
            one_game_results[SmplSz][RLTV_RECORD_DX] = 1

            if (rltv_record > Float0):
                if (home_win == True): 
                    one_game_results[Cnfrmd][RLTV_RECORD_DX] = 1
            else: # (rltv_record < Float0)
                if (home_win == False): 
                    one_game_results[Cnfrmd][RLTV_RECORD_DX] = 1

    # Can we use either/both recent team history(s) as a predictor
    # We may have one predictor from either team... 
    # conflicting predictors or complimentary predictors
    home_history_trend = GetHistoryValue(home_data.game_hist)
    vstr_history_trend = GetHistoryValue(vstr_data.game_hist)

    # Do we have one useful team history, complementary history or
    # conflicting history? (net history == 0)
    net_history_trend = sign(home_history_trend - vstr_history_trend)
    if (net_history_trend == 0):
        Predictors.num_history_weeded += 1
    else:
        one_game_results[SmplSz][TEAM_HISTORY_DX] = 1

        if (home_history_trend == 1):
            if (home_win == True): one_game_results[Cnfrmd][TEAM_HISTORY_DX] = 1

        if (home_history_trend == -1):
            if (home_win == False): one_game_results[Cnfrmd][TEAM_HISTORY_DX] = 1

        if (home_history_trend == 0):
            if (vstr_history_trend == 1):
               if (home_win == False): one_game_results[Cnfrmd][TEAM_HISTORY_DX] = 1

            if (vstr_history_trend == -1):
                if (home_win == True): one_game_results[Cnfrmd][TEAM_HISTORY_DX] = 1

    # Can we use either/both team momentum(s) as a predictor
    # We may have one predictor from either team or complimentary predictors or 
    # conflicting predictors (net momentum == 0) 
    home_momentum_trend = 0
    if (home_data.curr_momentum >= MOMENTUM_THRESHOLD):
        home_momentum_trend = home_data.game_hist[RECENT_HISTORY_LEN-1]

    vstr_momentum_trend = 0
    if (vstr_data.curr_momentum >= MOMENTUM_THRESHOLD):
        vstr_momentum_trend = vstr_data.game_hist[RECENT_HISTORY_LEN-1]

    # Do we have conflicting momentum ? (net momentum == 0) 
    # or complementary momentum
    net_momentum_trend = sign(home_momentum_trend - vstr_momentum_trend)
    if (net_momentum_trend == 0):
        Predictors.num_momentum_weeded += 1
    else:
        one_game_results[SmplSz][TEAM_MOMENTUM_DX] = 1
        if (home_momentum_trend == 1):
            if (home_win == True): one_game_results[Cnfrmd][TEAM_MOMENTUM_DX] = 1

        if (home_momentum_trend == -1):
            if (home_win == False): one_game_results[Cnfrmd][TEAM_MOMENTUM_DX] = 1

        if (vstr_momentum_trend == 1):
            if (home_win == False): one_game_results[Cnfrmd][TEAM_MOMENTUM_DX] += 1
        if (vstr_momentum_trend == -1):
            if (home_win == True): one_game_results[Cnfrmd][TEAM_MOMENTUM_DX] += 1

    # Collate the predictor data
    Predictors.PredictionRatios = nmp.add(Predictors.PredictionRatios,one_game_results)
    UpdateCorrelationValues(Predictors.correlations_nd.matrix, one_game_results)

    # Can the combined predictor make a prediction ?
    prediction_set = one_game_results[SmplSz]
    if (sum(prediction_set) > (NUM_PREDICTORS / 2)):
        prediction_results = one_game_results[Cnfrmd]
        combined_values = \
        ComputeCombinedPredictor(Net_Predictor_Rates, prediction_set)
        if (combined_values[0] > Float0):
            Predictors.combined_sample_size += 1

            #If a majority of predictiors were correct - declare victory
            if (sum(prediction_results) > (sum(prediction_set) / 2)):
                #print(prediction_set, prediction_results)
                Predictors.num_combined_correct += 1    

            Predictors.combined_rate_sum += combined_values[0]
            Predictors.rate_increase_sum += combined_values[1]

    return True

# Update predictor correlations for every pair of predictors in a square matrix
# Shape should be 2(N,D):num_pred:num_pred
def UpdateCorrelationValues(Corr_Matrix, One_Game_Results):

    cor_shape     = nmp.shape(Corr_Matrix)
    results_shape = nmp.shape(One_Game_Results)
    mtrx_dim = results_shape[1]
    assert((mtrx_dim == cor_shape[1]) and (mtrx_dim == cor_shape[2])),\
           "UpdateCorrelationValues... incompatible matrices"

    for row in range(0, mtrx_dim):
        col0 = row

        for col in range(0, mtrx_dim):
            # Did both predictors make a prediction ?
            if ((One_Game_Results[SmplSz][col0] == 1) and (One_Game_Results[SmplSz][col] == 1)):
                Corr_Matrix[SmplSz][row][col] += 1

                # If so - did both predictors make the same prediction ?  0 or 1
                if (One_Game_Results[Cnfrmd][col0] == One_Game_Results[Cnfrmd][col]):
                    Corr_Matrix[Cnfrmd][row][col] += 1
    return     

# Determine a predictor that is a reasonable combination of the known predictors.
# Attempt to produce an efficiency greater than the best input predictor
def ComputeCombinedPredictor(Net_Predictor_Rates, Active_Pred_Set):
    combined_values = (Float0, Float0)

    # Detemine the best input predictor and its' location in the set
    net_pred_rates = nmp.multiply(Net_Predictor_Rates, Active_Pred_Set)
    max_net_pred_rate = max(net_pred_rates)
    if (max_net_pred_rate == Float0):
        #print(max_net_pred_rate, Active_Pred_Set, net_pred_rates)
        return combined_values

    # Start out equal to the best input predictor
    if (max_net_pred_rate not in net_pred_rates): #Net_Predictor_Rates):
        #print(Active_Pred_Set, net_pred_rates)
        return combined_values

    max_pred_dx = Net_Predictor_Rates.index(max_net_pred_rate)
    combined_predictor = max_net_pred_rate

    for x in range(0, len(Net_Predictor_Rates)):
        if (x == max_pred_dx): continue # Skip the row:col pair which has an independence of zero

        weights = Predictor_Divergence_Matrix[x] # The row specific to this predictor
        weight = weights[max_pred_dx]
        combined_predictor += net_pred_rates[x] * weight

    if (combined_predictor <= max_net_pred_rate):
        return combined_values

    combined_predictor += Float_500
    max_pred_rate = max_net_pred_rate + Float_500
    increase = combined_predictor - max_pred_rate
    #print("ComputeCombinedPredictor: max_input_rate, combined_rate, increase...", \
    #      FltFormat(max_pred_rate), FltFormat(combined_predictor), FltFormat(increase))

    combined_values = (combined_predictor, increase)
    return combined_values

# These city codes are 2019-specific and have changed over time
# Older codes must be converted
def AssignTeamIndx(CityCode):
    if CityCode in CITY_CODES: # No substitution necessary
        return CITY_CODES.index(CityCode)

    if CityCode == "FLO":
        CityCode = "MIA"
    if CityCode == "SLA":
        CityCode = "BAL" # Replace the AL St Louis Browns w/ BAL (Orioles)
    if CityCode == "PHA":
        CityCode = "OAK" # Replace the AL PHil Athletics w/ OAK
    if CityCode == "WS1":
        CityCode = "MIN" # Replace the AL Charter Washington Senators w/ MIN Twins
    if CityCode == "BRO":
        CityCode = "LAN" # Replace NL Brooklyn w/ the Los Angeles Dodgers
    if CityCode == "NY1":
        CityCode = "SFN" # Replace the NL New York team w/ the S.F. NL Giants
    if CityCode == "BSN":
        CityCode = "ATL" # Replace the Boston NL team w/ the Atl Braves

    if CityCode in CITY_CODES:
        return CITY_CODES.index(CityCode)
    else:
        print("AssignTeamIndx error: Federal League?",CityCode)
        return -1

# Return a value for the recent history trend if there is enough data
def GetHistoryValue(game_hist):
    if (game_hist[0] == 0): return 0

    wins = game_hist.count(1) 
    if (wins > (HISTORY_HALF_LEN+1)): return 1   
    if (wins < (HISTORY_HALF_LEN-1)): return -1  
    return 0

# We are looking for examples of the Earl Weaver method succeeding
# that one big inning decides the game (one inning >= losing score)
def ExtractWeaverData(Ssn_Data, GL_Entry):

    home_score = int(GL_Entry.home_score)
    vstr_score = int(GL_Entry.vstr_score)
    Ssn_Data.total_winning_runs += max(home_score, vstr_score)
    Ssn_Data.total_losing_runs  += min(home_score, vstr_score)

    if (GL_Entry.home_stolen_bases != ''):
        if (int(GL_Entry.home_stolen_bases) > -1):
            Ssn_Data.total_stolen_bases += int(GL_Entry.home_stolen_bases)

    if (GL_Entry.vstr_stolen_bases != ''):
        if (int(GL_Entry.vstr_stolen_bases) > -1):
            Ssn_Data.total_stolen_bases += int(GL_Entry.vstr_stolen_bases)

    if (GL_Entry.duration != ''):
        Ssn_Data.total_duration += int(GL_Entry.duration)

    # If they are tied after 9 innings skip this game
    xtra_inns = (int(GL_Entry.outs) > 54)
    if xtra_inns: return

    losing_score = min(home_score, vstr_score)
    if (losing_score == 0):
        losing_score = 1

    home_win  = (home_score > vstr_score)
    if home_win:
        WinRunsPerInnStr = GL_Entry.home_runs_by_inn
    else:
        WinRunsPerInnStr = GL_Entry.vstr_runs_by_inn

    # Save the string into a list
    WinRunsPerInn = [] 
    for i in range(len(WinRunsPerInnStr)): 
        WinRunsPerInn.append(WinRunsPerInnStr[i])

    # Convert inning runs to integer
    WinRunsPerInnInt = []
    for i in range(len(WinRunsPerInn)):
        if (WinRunsPerInn[i] == LParen): # This inning looks like... (dd)
            WinRunsPerInnInt.append(10)  # Well it might be 16 !
            break
        else: 
            if (WinRunsPerInn[i] != 'x'):
                WinRunsPerInnInt.append(int(WinRunsPerInn[i]))

    # Finally - do we have a sufficient big inning
    # Require a 'crooked number' (more than one run)
    big_inning = max(WinRunsPerInnInt)
    if (big_inning == 1):
        return

    #if (big_inning > losing_score): # Require that the big inning wins the game
    if (big_inning >= losing_score): # Count a tie in Weaver's favor
        Ssn_Data.weaver_method_gms += 1
        #print("Weaver inning! winning runs by inning, losing score = ",\
        #      WinRunsPerInnInt, losing_score)

    return

###
def PrintSummary(WorkDataFile, num_processed, num_examined):
    print("Input_file=",WorkDataFile, " records_scanned=", num_processed, \
          " records_examined_for_stats=", num_examined)
    return

###
def PrintSeasonStats(Ssn_Data):
    print("Season Stats...")
    for team_data in Ssn_Data.team_data:
        print(team_data.name, "#wins=", team_data.wins, " #games=", team_data.games_sampled,      \
              #" #home_wins=", team_data.home_wins, " #home_games=", team_data.home_games, \
              #" #home_xtra_inns_wins=", team_data.hm_xtra_inn_wins, \
              #" #home_xtra_inn_gms=", team_data.hm_xtra_inn_gms, \
              #" #xtra_inns_gms=", team_data.xtra_inn_gms, \
              #" momentum=", team_data.curr_momentum, " history=", team_data.game_hist) 
              " momentum: #predicted #games streaks=", \
              team_data.num_momentum_correct, team_data.momentum_sample_size,\
              team_data.num_momentum_win_strks, team_data.num_momentum_loss_strks)
              #str(float(team_data.num_momentum_correct/team_data.momentum_sample_size))[0:5]
    return

#
def DeriveSeasonSummary(Ssn_Data, Year):
    temp_team_objs = []
    for team_data in Ssn_Data.team_data:
        if (team_data.games_sampled == 0): continue

        team_data.win_ratio = float(team_data.wins / team_data.games_sampled)
        temp_team_objs.append(team_data)

    sorted_team_objs = sorted(temp_team_objs, key=TeamWinRatio, reverse=True)
    #PrintSeasonSummary(sorted_team_objs, Year)
 
    return sorted_team_objs

#
def PrintSeasonSummary(Team_List, Year):
    print("Season_Summary...", Year)

    team_num = 0
    for team_data in Team_List:
        team_num += 1
        print(team_num, team_data.name, "#wins, #games, win%...",
              team_data.wins, team_data.games_sampled, FltFormat(team_data.win_ratio))
    return

# This returns a sort key field
def TeamName(Team_Data):
    return Team_Data.name

# This returns a sort key field
def TeamWinRatio(Team_Data):
    return Team_Data.win_ratio

###
def CollateSeasonData(Ssn_Data, Num_Examined):
    Ssn_Data.total_games_sampled = Num_Examined

    for team_data in Ssn_Data.team_data:
        Ssn_Data.predictor_stats = nmp.add(Ssn_Data.predictor_stats, team_data.predictor_stats)

        Ssn_Data.hm_xtra_inn_wins   += team_data.hm_xtra_inn_wins
        Ssn_Data.xtra_inn_gms       += team_data.hm_xtra_inn_gms
        Ssn_Data.total_stolen_bases += team_data.stolen_bases

    return

# Determine the percentage for home field, relative payroll, relative record, N game history, momentum 
def DerivePredictors(Ssn_Data, Num_Games_Examined):

    Num_Team_Games = Num_Games_Examined * 2

    Ssn_Data.hometeam_win_pred_rate = \
        float(Ssn_Data.predictor_stats[Cnfrmd][HOME_WIN_DX]/Ssn_Data.predictor_stats[SmplSz][HOME_WIN_DX])
    Ssn_Data.rltv_payroll_pred_rate = \
        float(Ssn_Data.predictor_stats[Cnfrmd][RLTV_PAYROLL_DX]/Ssn_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX])
    Ssn_Data.rltv_record_pred_rate = \
        float(Ssn_Data.predictor_stats[Cnfrmd][RLTV_RECORD_DX]/Ssn_Data.predictor_stats[SmplSz][RLTV_RECORD_DX])
    Ssn_Data.history_pred_rate   = \
        float(Ssn_Data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX]/Ssn_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX])
    Ssn_Data.momentum_pred_rate = \
        float(Ssn_Data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX]/Ssn_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX])

    print("DerivePredictors... year=", Ssn_Data.year, \
          " games_per_season=", NUM_GAMES_PER_SEASON, \
          " total_gms_examined=",Ssn_Data.total_games_sampled,\
          " total_teams =",NUM_TEAMS," extra_inning_gms =",Ssn_Data.xtra_inn_gms)

    print("Home_extra_innings: #wins #games ratio...", \
          Ssn_Data.hm_xtra_inn_wins,Ssn_Data.xtra_inn_gms, \
          RtoFormat(Ssn_Data.hm_xtra_inn_wins, Ssn_Data.xtra_inn_gms))

    print("Home_team: wins games pred_eff...", \
          Ssn_Data.predictor_stats[Cnfrmd][HOME_WIN_DX],Ssn_Data.predictor_stats[SmplSz][HOME_WIN_DX],\
          FltFormat(Ssn_Data.hometeam_win_pred_rate))

    print("Relative_Payroll: #correct #samples pred_rate...", \
          Ssn_Data.predictor_stats[Cnfrmd][RLTV_PAYROLL_DX],Ssn_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX], \
          FltFormat(Ssn_Data.rltv_payroll_pred_rate),\
          " sample_size_ratio=", RtoFormat(Ssn_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX], Num_Games_Examined))

    print("Relative_Record: #correct #samples pred_rate...", \
          Ssn_Data.predictor_stats[Cnfrmd][RLTV_RECORD_DX],Ssn_Data.predictor_stats[SmplSz][RLTV_RECORD_DX], \
          FltFormat(Ssn_Data.rltv_record_pred_rate),\
          " sample_size_ratio=", RtoFormat(Ssn_Data.predictor_stats[SmplSz][RLTV_RECORD_DX], Num_Games_Examined))

    print("Recent_history: #correct #samples, pred_rate...", \
          Ssn_Data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX],Ssn_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX], \
          FltFormat(Ssn_Data.history_pred_rate),\
          " sample_size_ratio=", RtoFormat(Ssn_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX], Num_Games_Examined))

    print("Momentum: #correct  #samples pred_rate...",      \
          Ssn_Data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX],Ssn_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX], \
          FltFormat(Ssn_Data.momentum_pred_rate), \
          " sample_size_ratio=", RtoFormat(Ssn_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX], Num_Games_Examined))

    avg_wscore = Ssn_Data.total_winning_runs / Ssn_Data.total_games_sampled
    avg_lscore = Ssn_Data.total_losing_runs / Ssn_Data.total_games_sampled
    print("Averages: winning score  losing score  runs_per_game...", \
          FltFormat(avg_wscore), FltFormat(avg_lscore), FltFormat(avg_wscore+avg_lscore))

    return

#
def EvaluatePredictors(Ssn_Data, Predictors, num_examined):
    pred_ratios = Predictors.PredictionRatios
    sample_size = Predictors.total_games_sampled

    print("EvaluatePredictors: home_team=",HOMETEAM_WIN_PRED_RATE,\
          "relative_payroll=",RLTV_PAYROLL_PRED_RATE, "relative_payroll=",RLTV_RECORD_PRED_RATE,\
          "team_history=",TEAM_HISTORY_PRED_RATE,"team_momentum=",TEAM_MOMENTUM_PRED_RATE)

    Ssn_Data.hometeam_win_pred_rate = \
        float(pred_ratios[Cnfrmd][HOME_WIN_DX]/pred_ratios[SmplSz][HOME_WIN_DX])
    print("Home_team: sample_size_ratio, predictor_rate...", \
          RtoFormat(pred_ratios[SmplSz][HOME_WIN_DX], num_examined), \
          FltFormat(Ssn_Data.hometeam_win_pred_rate))  

    Ssn_Data.rltv_payroll_pred_rate = \
        float(pred_ratios[Cnfrmd][RLTV_PAYROLL_DX]/pred_ratios[SmplSz][RLTV_PAYROLL_DX])
    print("Relative_Payroll: sample_size_ratio, predictor_rate...", \
          RtoFormat(pred_ratios[SmplSz][RLTV_PAYROLL_DX], sample_size), \
          FltFormat(Ssn_Data.rltv_payroll_pred_rate)) 

    Ssn_Data.rltv_record_pred_rate = \
        float(pred_ratios[Cnfrmd][RLTV_RECORD_DX]/pred_ratios[SmplSz][RLTV_RECORD_DX])
    print("Relative_Record: sample_size_ratio, predictor_rate...", \
          RtoFormat(pred_ratios[SmplSz][RLTV_RECORD_DX], sample_size), \
          FltFormat(Ssn_Data.rltv_record_pred_rate))  

    Ssn_Data.history_pred_rate = \
        float(pred_ratios[Cnfrmd][TEAM_HISTORY_DX]/pred_ratios[SmplSz][TEAM_HISTORY_DX])
    print("Team_history: sample_size_ratio, #weeded, predictor_rate...", \
          RtoFormat(pred_ratios[SmplSz][TEAM_HISTORY_DX], sample_size), \
          Predictors.num_history_weeded, FltFormat(Ssn_Data.history_pred_rate)) 

    Ssn_Data.momentum_pred_rate = \
        float(pred_ratios[Cnfrmd][TEAM_MOMENTUM_DX]/pred_ratios[SmplSz][TEAM_MOMENTUM_DX])
    print("Team_momentum: sample_size_ratio, #x, #weeded, predictor_rate...", \
          RtoFormat(pred_ratios[SmplSz][TEAM_MOMENTUM_DX], sample_size), \
          sample_size, Predictors.num_momentum_weeded,\
          RtoFormat(pred_ratios[Cnfrmd][TEAM_MOMENTUM_DX], pred_ratios[SmplSz][TEAM_MOMENTUM_DX])) 

    print("Combined: sample_size_ratio, #confirmed, sample_size, predictor_rate...",\
          RtoFormat(Predictors.combined_sample_size, sample_size), \
          Predictors.num_combined_correct, Predictors.combined_sample_size, \
          RtoFormat(Predictors.num_combined_correct, Predictors.combined_sample_size))

    Ssn_Data.combined_pred_rate = Predictors.combined_rate_sum / float(Predictors.combined_sample_size)
    Ssn_Data.pred_rate_increase = Predictors.rate_increase_sum / float(Predictors.combined_sample_size)
    print("Combined computed averages: predictor_rate, increase...", \
          FltFormat(Ssn_Data.combined_pred_rate), FltFormat(Ssn_Data.pred_rate_increase))

    return

# Collect summary data for the whole decade/Period - determine long-term averages, ratios
def CollatePeriodData(Period_Data):

    for season_data in Period_Data.season_data:
        Period_Data.num_seasons += 1
        Period_Data.total_games_sampled += season_data.total_games_sampled

        Period_Data.predictor_stats     = nmp.add(Period_Data.predictor_stats, season_data.predictor_stats)   
         
        Period_Data.xtra_inn_gms       += season_data.xtra_inn_gms
        Period_Data.hm_xtra_inn_wins   += season_data.hm_xtra_inn_wins 
        Period_Data.total_winning_runs += season_data.total_winning_runs
        Period_Data.total_losing_runs  += season_data.total_losing_runs
        Period_Data.total_stolen_bases += season_data.total_stolen_bases
        Period_Data.weaver_method_gms  += season_data.weaver_method_gms

    return

#
def CollatePeriodPredictorResults(Period_Data):

    hometeam_win_pred_rate  = Float0
    rltv_payroll_pred_rate  = Float0
    rltv_record_pred_rate   = Float0
    history_pred_rate       = Float0
    momentum_pred_rate      = Float0
    combined_pred_rate      = Float0
    pred_rate_increase      = Float0 

    for season_data in Period_Data.season_data:
        Period_Data.num_seasons += 1
        hometeam_win_pred_rate  += season_data.hometeam_win_pred_rate
        rltv_payroll_pred_rate  += season_data.rltv_payroll_pred_rate
        rltv_record_pred_rate   += season_data.rltv_record_pred_rate
        history_pred_rate       += season_data.history_pred_rate
        momentum_pred_rate      += season_data.momentum_pred_rate
        combined_pred_rate      += season_data.combined_pred_rate
        pred_rate_increase      += season_data.pred_rate_increase 

    Period_Data.hometeam_win_pred_rate = float(hometeam_win_pred_rate / Period_Data.num_seasons)
    Period_Data.rltv_payroll_pred_rate = float(rltv_payroll_pred_rate / Period_Data.num_seasons)
    Period_Data.rltv_record_pred_rate  = float(rltv_record_pred_rate / Period_Data.num_seasons)
    Period_Data.history_pred_rate      = float(history_pred_rate / Period_Data.num_seasons)
    Period_Data.momentum_pred_rate     = float(momentum_pred_rate / Period_Data.num_seasons)
    Period_Data.combined_pred_rate     = float(combined_pred_rate / Period_Data.num_seasons)
    Period_Data.pred_rate_increase     = float(pred_rate_increase / Period_Data.num_seasons)

    print()
    print("Period data: #seasons=",Period_Data.num_seasons,"\n",\
          "             hometeam_win_pred_rate=", FltFormat(Period_Data.hometeam_win_pred_rate), \
          "relative_payroll_pred_rate=", FltFormat(Period_Data.rltv_payroll_pred_rate), \
          "relative_record_pred_rate=", FltFormat(Period_Data.rltv_record_pred_rate), "\n,"\
          "             team_history_pred_rate=", FltFormat(Period_Data.history_pred_rate), \
          "team_momentum_pred_rate=", FltFormat(Period_Data.momentum_pred_rate), \
          "combined_pred_rate=", FltFormat(Period_Data.combined_pred_rate),
          "pred_rate_increase=", FltFormat(Period_Data.pred_rate_increase))

    return

###
def PrintPeriodSummary(Period_Data):
    hometeam_win_pred_rate = float(Period_Data.predictor_stats[Cnfrmd][HOME_WIN_DX] / Period_Data.predictor_stats[SmplSz][HOME_WIN_DX])
    rltv_payroll_pred_rate = float(Period_Data.predictor_stats[Cnfrmd][RLTV_PAYROLL_DX] / Period_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX])
    rltv_record_pred_rate  = float(Period_Data.predictor_stats[Cnfrmd][RLTV_RECORD_DX] / Period_Data.predictor_stats[SmplSz][RLTV_RECORD_DX])
    history_pred_rate      = float(Period_Data.predictor_stats[Cnfrmd][TEAM_HISTORY_DX] / Period_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX])
    momentum_pred_rate     = float(Period_Data.predictor_stats[Cnfrmd][TEAM_MOMENTUM_DX] / Period_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX])

    print("Period data: #seasons=",Period_Data.num_seasons,\
          "hometeam_win_pred_rate=", FltFormat(hometeam_win_pred_rate), \
          "relative_payroll_pred_rate=", FltFormat(rltv_payroll_pred_rate), \
          "relative_record_pred_rate=", FltFormat(rltv_record_pred_rate),"\n", \
          "team_history_pred_rate=", FltFormat(history_pred_rate), \
          "team_momentum_pred_rate=", FltFormat(momentum_pred_rate))

    print("Period data sample_sizes: home_win, rltv_payroll, rltv_record, history, momentum...", \
          Period_Data.total_games_sampled,\
          Period_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX], \
          Period_Data.predictor_stats[SmplSz][RLTV_RECORD_DX], \
          Period_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX],\
          Period_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX])

    print("Period data sample_size_ratios: home_win, rltv_payroll, rltv_record, history, momentum...", \
          RtoFormat(Period_Data.total_games_sampled, Period_Data.total_games_sampled), \
          RtoFormat(Period_Data.predictor_stats[SmplSz][RLTV_PAYROLL_DX], Period_Data.total_games_sampled), \
          RtoFormat(Period_Data.predictor_stats[SmplSz][RLTV_RECORD_DX], Period_Data.total_games_sampled), \
          RtoFormat(Period_Data.predictor_stats[SmplSz][TEAM_HISTORY_DX], Period_Data.total_games_sampled), \
          RtoFormat(Period_Data.predictor_stats[SmplSz][TEAM_MOMENTUM_DX], Period_Data.total_games_sampled))

    return

# Number crunch predictor correlations
def EvaluateCorrelations(CorrelationsND, Correlations):
    for row in range(0, NUM_PREDICTORS): #(0, )
        for col in range(0, NUM_PREDICTORS):
            Correlations[row][col] = \
            float(CorrelationsND.matrix[Cnfrmd][row][col] / CorrelationsND.matrix[SmplSz][row][col])
    return

#Print a matrix of ratios
def PrintMatrixRatios(Title, Correlations):
    print("\nValues.................. home_win      rltv_payroll rltv_record  team_history    team_momentum")
    for row in range(0, NUM_PREDICTORS):
        print("Row.........",Correlation_Labels[row],"  ",\
              FltFormat(Correlations[row][HOME_WIN_DX]),"       ",\
              FltFormat(Correlations[row][RLTV_PAYROLL_DX]),"      ",\
              FltFormat(Correlations[row][RLTV_RECORD_DX]),\
              "      ",FltFormat(Correlations[row][TEAM_HISTORY_DX]),
              "         ",FltFormat(Correlations[row][TEAM_MOMENTUM_DX]))
    return

# This functions assumes that its input is ratios where sometimes
# there is a value > 1 because the ratio is the reciprocal of probability/correlation
def InvertMatrixRatios(Correlations):
    shape_tuple = nmp.shape(Correlations)

    for row in range(0, shape_tuple[0]):
        for col in range(0, shape_tuple[1]):
            Correlations[row][col] = RatioLimit(Correlations[row][col])
    return

# This functions converts ratios < 1 to their OnesComplement
# Converting correlation to independence (75% correlation == 25% indepencence)
def ConvertCor2Ind(Correlations, Independencies):
    cor_shape = nmp.shape(Correlations)
    ind_shape = nmp.shape(Independencies)
    assert(cor_shape == ind_shape), "Mis-matched matrix shapes"

    for row in range(0, cor_shape[0]):
        for col in range(0, cor_shape[1]):
            Independencies[row][col] = OnesComplement(Correlations[row][col])
    return

# Convert correlation to independence ratio
def OnesComplement(X):
    assert (X <= Float1), "Invalid ratio > 1.000"
    if (X >= Float1): return Float0
    else: return (Float1 - X)

# Return the reciprocal of anything > 1
def RatioLimit(X):
    if (X <= Float1): return X
    else: return Float1 / X

# return -1, 0, 1
def sign(n):
    if (n < 0): return -1
    if (n==0): return 0
    return 1

#
def ComputeArrayStats(Array, qprint):
    Size = len(Array)
    Min = min(Array)
    Max = max(Array)
    Avg = nmp.average(Array)
    Mdn = nmp.median(Array)
    StdDev = nmp.std(Array)
    Var = nmp.var(Array)
    array_stats = (Size, Min, Max, Avg, Mdn, StdDev, Var)

    if (qprint == True):
        print("Array size, Min, Max, Avg, median, stddev, Var...\n", \
              Size, Min, Max, Avg, Mdn, StdDev, Var)
    return array_stats

# Format an array of two elements for printing
def Ar2Format(Array):
    return RtoFormat(Array[0],Array[1])

# Format a ratio for printing
def RtoFormat(N, D):
    float_val = float(N / D)
    return FltFormat(float_val)

# Format a floating pt value as  (-)X.xyz (rounded) for printing
def FltFormat(Float_Val):
    if (Float_Val < Float0): return str(Float_Val-.0005)[0:6]
    else:                    return str(Float_Val+.0005)[0:5]

# Return the year segment from a file name formatted... data\\BB_GL_yyyy.csv
def YearFromFileName(Filename):
    year_ext_segments = Filename.split('_') 
    segments = year_ext_segments[2].split('.')
    #numsegs = len(segments) 
    year    = segments[0]
    #print("YearFromFileName", year)
    return year

def AssignPayrolls(Team_Payrolls, Year):
    Filename = "..\\data\\Team_Payrolls_" + Year + ".txt"
    if not (os.path.exists(Filename)):
        return

    payroll_list = []
    payroll_file = open(Filename, 'r', newline='')
    print("AssignPayrolls file, year...", Filename, Year)

    for x in range(0, len(Team_Payrolls)):
        payroll_line = payroll_file.readline()
        #print(payroll_line)
        payroll_list.append(payroll_line)

    payroll_file.close()

    for x in range(0, len(Team_Payrolls)):
        Team_Payrolls[x] = AssignTeamPayroll(payroll_list, Team_Payrolls[x])
        #print(Team_Payrolls[x])

    return

def AssignTeamPayroll(Payroll_List, Team_Payroll):
    team_payroll = [] 
    team = Team_Payroll[0]
    city = Team_Payroll[1]

    payroll_line = ""
    for x in range(0, len(Payroll_List)):
        if (Payroll_List[x].find(team) > -1):
            payroll_line = Payroll_List[x]
            break

    assert(payroll_line != ""), ("Team not found in payroll list...", team)

    segments = payroll_line.split('$')
    payroll_segment = segments[1]
    payroll_segment = payroll_segment.replace('\t', ' ')
    payroll_segment = payroll_segment.replace('-', ' ')
    payroll_segment = payroll_segment.replace(',', '')
    payroll = int((int(payroll_segment) + HALF_MILLION) / ONE_MILLION)
    #print(payroll_segment, payroll)
    team_payroll = [team, city, payroll]

    return team_payroll
