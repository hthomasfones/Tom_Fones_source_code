#Define classes for baseball study solution

from Baseball_Constants import *
from Baseball_Function_Defs import *

# 'The working data object for one game - extracted from the raw data file'
class GameLogEntry: 
    'The working data object for one game - extracted from the raw data file'

    def __init__(self, GL_Entry_Raw):
        self.date             = GL_Entry_Raw[0]
        self.league           = GL_Entry_Raw[7]
        self.duration         = GL_Entry_Raw[18]
        self.outs             = GL_Entry_Raw[11]
        self.visitor          = GL_Entry_Raw[3]
        self.vstr_score       = GL_Entry_Raw[9]
        self.vstr_runs_by_inn = GL_Entry_Raw[19]
        self.vstr_game_num    = GL_Entry_Raw[5]
        self.vstr_stolen_bases = GL_Entry_Raw[34]
        self.hometeam         = GL_Entry_Raw[6]
        self.home_score       = GL_Entry_Raw[10]
        self.home_runs_by_inn = GL_Entry_Raw[20]
        self.home_game_num    = GL_Entry_Raw[8]
        self.home_stolen_bases = GL_Entry_Raw[62]

# 'The working data object for one game - retrieved from the saved file'
class GameLogWorkEntry(GameLogEntry): 
    'The working data object for one game - retrieved from the saved file'

    def __init__(self, GL_Entry_Work):
        self.date             = GL_Entry_Work[0]
        self.league           = GL_Entry_Work[1]
        self.duration         = GL_Entry_Work[2]
        self.outs             = GL_Entry_Work[3]
        self.visitor          = GL_Entry_Work[4]
        self.vstr_score       = GL_Entry_Work[5]
        self.vstr_runs_by_inn = GL_Entry_Work[6]
        self.vstr_game_num    = GL_Entry_Work[7]
        self.vstr_stolen_bases = GL_Entry_Work[8]
        self.hometeam         = GL_Entry_Work[9]
        self.home_score       = GL_Entry_Work[10]
        self.home_runs_by_inn = GL_Entry_Work[11]
        self.home_game_num    = GL_Entry_Work[12]
        self.home_stolen_bases = GL_Entry_Work[13]

#  The current state for one team'
class TeamData:
    'The current state for one team'

    def __init__(self, Num, Name, City):
        self.dx          = Num
        self.name        = Name
        self.city        = City
        self.payroll     = 0
        self.games_sampled = 0
        self.wins        = 0
        self.win_ratio   = Float0
        self.xtra_inn_gms= 0
        self.hm_xtra_inn_gms = 0 
        self.hm_xtra_inn_wins = 0
        self.max_win_streak  = 0
        self.max_loss_streak = 0
        self.shutouts     = 0    #offense - not this teams pitching
        self.stolen_bases = 0

        self.game_hist = []
        for x in range(0,RECENT_HISTORY_LEN): self.game_hist.append(0)

        # This record-keeping makes matrix arithmetic easier in the season summary
        # We won't track relative record for one team
        self.predictor_stats = Predictor_Stats_Zeros_Array.copy()
        self.curr_momentum   = 0
        self.num_momentum_win_strks  = 0
        self.num_momentum_loss_strks  = 0

# The current/final state for all teams - one season'
class SeasonData:
    'The current/final state for all teams - one season'
    'Initialize the season object...' 
    def __init__(self, year):
        self.year                = year
        self.total_duration      = 0
        self.predictor_stats     = Predictor_Stats_Zeros_Array.copy()

        self.total_winning_runs  = 0
        self.total_losing_runs   = 0
        self.total_stolen_bases  = 0
        self.weaver_method_gms   = 0
        self.xtra_inn_gms        = 0
        self.hm_xtra_inn_wins    = 0 
        self.avg_winning_score   = float(0)
        self.avg_losing_score    = float(0)

        'Initialize the team object for all teams...' 
        self.Team_Payrolls = []
        for x in range(0, len(Teams)):
            self.Team_Payrolls.append(Teams[x])
        AssignPayrolls(self.Team_Payrolls, self.year)

        self.hometeam_win_pred_rate = float(0)
        self.rltv_payroll_pred_rate = float(0)
        self.rltv_record_pred_rate  = float(0)
        self.history_pred_rate      = float(0)
        self.momentum_pred_rate     = float(0)
        self.combined_pred_rate     = float(0)
        self.pred_rate_increase     = float(0)

        self.team_data = []
        for x in range(0, len(Teams)):
            self.team_data.append(TeamData(x, Teams[x][0], Teams[x][1]))

        for x in range(0, len(Teams)):
            self.team_data[x].payroll = self.Team_Payrolls[x][2]
            # print(self.team_data[x].name, self.team_data[x].city, self.team_data[x].payroll)

# 'The summary data for multiple seasons'
class PeriodData:
    'Initialize the period/epoch object...' 
    def __init__(self):
        self.num_seasons          = 0
        #self.total_duration       = 0
        self.total_games_sampled  = 0
        self.predictor_stats      = Predictor_Stats_Zeros_Array.copy()

        self.total_winning_runs   = 0
        self.total_losing_runs    = 0
        self.total_stolen_bases   = 0
        self.weaver_method_gms    = 0
        self.xtra_inn_gms         = 0
        self.hm_xtra_inn_wins     = 0 

        self.avg_winning_score      = float(0)
        self.avg_losing_score       = float(0)
        self.hometeam_win_pred_rate = float(0)
        self.rltv_payroll_pred_rate = float(0)
        self.rltv_record_pred_rate  = float(0)
        self.history_pred_rate      = float(0)
        self.momentum_pred_rate     = float(0)
        self.combined_pred_rate     = float(0)
        self.pred_rate_increase     = float(0)

        'Initialize the season object for all seasons...' 
        self.season_data = []

class SeasonSummary:
    'The summary data for one season'

    def __init__(self, year):
        self.year  = year
        self.team_data = []

class PeriodSummary:
    def __init__(self, year):
        self.year  = year
        self.team_data = []

# The matrix for tracking correlations between predictors
# N x N x 2(num,denom)
# 
class CorrelationsND_Matrix:
    'The matrix for tracking correlations between predictors'
    def __init__(self):
        self.matrix = RatiosND_Zeros_Array.copy()
      
# 'The current/final state of the predictors for one season' 
class PredictorData:
    'The current/final state of the predictors for one season' 
    'Initialize the object...' 
    def __init__(self, year):
        self.year                    = year
        self.total_games_sampled     = 0
        self.PredictionRatios        = Predictor_Zeros_Array.copy()
        self.num_history_weeded      = 0
        self.num_momentum_weeded     = 0

        # Can we improve upon the most efficient predictor ?
        self.combined_sample_size  = 0
        self.num_combined_correct  = 0
        self.combined_rate_sum      = Float0
        self.rate_increase_sum      = Float0
        self.combined_rate_avg      = Float0
        self.rate_increase_avg      = Float0
        #
        self.correlations_nd       = CorrelationsND_Matrix()

