# Working constants
import numpy as nmp

LParen = '('
Float0 = float(0.000)
Float1 = float(1.000)
Float_500  = float(0.500)
HALF_MILLION = 500000
ONE_MILLION = 1000000
Two_Pi = 6.2832

MOMENTUM_THRESHOLD = 3
RECENT_HISTORY_LEN = 12
HISTORY_HALF_LEN = (RECENT_HISTORY_LEN / 2) 
RLTV_RECORD_MIN_GAMES = 10
RLTV_RECORD_THRESHOLD = float(0.100)  # Ten percent difference

# Source web site for team payrolls for 2010...2019
# http://www.stevetheump.com/Payrolls.htm#2010payroll
RLTV_PAYROLL_THRESHOLD = float(1.333)

# Predictor efficiencies derived from the current completed decade
# hometeam_win_pred_rate=0.538 win_percent_diff_pred_rate=0.577 team_history_pred_rate=0.597
# relative_payroll_Pred_rate=0.546 team_momentum_pred_rate=0.680
#
TEAM_MOMENTUM_PRED_RATE = float(0.681)  # Momentum is the big dog. Big Mo is a beast
TEAM_HISTORY_PRED_RATE  = float(0.597)
RLTV_RECORD_PRED_RATE   = float(0.577)
RLTV_PAYROLL_PRED_RATE  = float(0.546)
HOMETEAM_WIN_PRED_RATE  = float(0.538)  # When all else fails - always available

EVEN_ODDS               = Float_500
#
# Set the weights proportional to their probabilities (prediction efficiency)
SUM_OF_PREDICTOR_RATES = \
    ((TEAM_MOMENTUM_PRED_RATE) + (TEAM_HISTORY_PRED_RATE) + RLTV_RECORD_PRED_RATE + HOMETEAM_WIN_PRED_RATE)

PREDICTOR_RATES = (HOMETEAM_WIN_PRED_RATE,RLTV_PAYROLL_PRED_RATE, RLTV_RECORD_PRED_RATE,\
                   TEAM_HISTORY_PRED_RATE,TEAM_MOMENTUM_PRED_RATE)

# The margin values are... how much better than even odds for each predictor 
Net_Predictor_Rates = ((HOMETEAM_WIN_PRED_RATE - Float_500),\
                       (RLTV_PAYROLL_PRED_RATE - Float_500),(RLTV_RECORD_PRED_RATE - Float_500),\
                       (TEAM_HISTORY_PRED_RATE - Float_500),(TEAM_MOMENTUM_PRED_RATE - Float_500))
MIN_PREDICTOR_RATE = min(PREDICTOR_RATES)
MAX_PREDICTOR_RATE = max(PREDICTOR_RATES)

# Parameters for the Correlation matrix
HOME_WIN_DX      = 0
RLTV_PAYROLL_DX  = 1
RLTV_RECORD_DX   = 2
TEAM_HISTORY_DX  = 3 
TEAM_MOMENTUM_DX = 4

AR_SIZE_DX       = 0
AR_MIN_DX        = 1
AR_MAX_DX        = 2
AR_AVG_DX        = 3
AR_MDN_DX        = 4
AR_STD_DX        = 5 

# Indeces of an array of 2; a ratio expressed as N/D
Cnfrmd = 0
SmplSz = 1
RatioTuple = (Cnfrmd, SmplSz)
RatioTupleLen = len(RatioTuple)
NUM_PREDICTORS = len(PREDICTOR_RATES)
#
OneD_Zeros_Array = nmp.zeros(NUM_PREDICTORS, dtype=int)
Predictor_Stats_Zeros_Array = nmp.zeros((RatioTupleLen, NUM_PREDICTORS), dtype=int)
Predictor_Zeros_Array = Predictor_Stats_Zeros_Array
RatiosND_Zeros_Array = nmp.zeros((RatioTupleLen, NUM_PREDICTORS, NUM_PREDICTORS), dtype=int)
Ratios_Zeros_Array = nmp.zeros((NUM_PREDICTORS, NUM_PREDICTORS), dtype=nmp.float32)
Payroll_Pair = nmp.zeros(2, dtype=int)

Correlation_Labels = ("home_win", "rltv_pay", "rltv_rec", "history.", "momentum")

# These values are the determined correlation values for all pairs of predictors
# Correlation values.. home_win, rltv_record, team_history,  team_momentum
# Matrix[row:col] == Matrix[col:row]
# Row......... home_win    1.000        0.493        0.489           0.565
# Row......... rltv_rec    0.493        1.000        0.942           0.897
# Row......... history.    0.489        0.942        1.000           0.955
# Row......... momentum    0.565        0.897        0.955           1.000

# These values are the one's complements of the correlation values above for  predictor pairs
# They will be used as relative weights applied to pairs of predictors while computing
# a composite/combined predictor
Predictor_Divergence_Matrix = ((0.000, 0.495, 0.507, 0.511, 0.435),
                               (0.495, 0.000, 0.276, 0.363, 0.437),
                               (0.507, 0.276, 0.000, 0.058, 0.103),
                               (0.511, 0.363, 0.058, 0.000, 0.045),
                               (0.435, 0.437, 0.103, 0.045, 0.000))

#Source web site for game logs
# https://www.retrosheet.org/gamelogs/

# One season starter data
#RawDataFile  = "..\\data\\Baseball_Season_2019.csv" #Our input file from the above URL
#WorkDataFile = "..\\data\\BB_GL_2019.csv"          #Our reusable working data set

# The dead ball era
Dead_Ball_Era = [# Raw data....................     extracted work data
                 ("..\\data\\Baseball_Season_1910.csv", "..\\data\\BB_GL_1910.csv"),
                 ("..\\data\\Baseball_Season_1911.csv", "..\\data\\BB_GL_1911.csv"),
                 ("..\\data\\Baseball_Season_1912.csv", "..\\data\\BB_GL_1912.csv"),
                 ("..\\data\\Baseball_Season_1913.csv", "..\\data\\BB_GL_1913.csv"),
                 ("..\\data\\Baseball_Season_1914.csv", "..\\data\\BB_GL_1914.csv"),
                 ("..\\data\\Baseball_Season_1915.csv", "..\\data\\BB_GL_1915.csv"),
                 ("..\\data\\Baseball_Season_1916.csv", "..\\data\\BB_GL_1916.csv"),
                 ("..\\data\\Baseball_Season_1917.csv", "..\\data\\BB_GL_1917.csv"),
                 ("..\\data\\Baseball_Season_1918.csv", "..\\data\\BB_GL_1918.csv"),
                 ("..\\data\\Baseball_Season_1919.csv", "..\\data\\BB_GL_1919.csv")
                 ]

NineteenTens_Decade = Dead_Ball_Era

Babe_Ruth_Era = [# Raw data....................     extracted work data
                 ("..\\data\\Baseball_Season_1910.csv", "..\\data\\BB_GL_1910.csv"),
                 ("..\\data\\Baseball_Season_1911.csv", "..\\data\\BB_GL_1911.csv"),
                 ("..\\data\\Baseball_Season_1912.csv", "..\\data\\BB_GL_1912.csv"),
                 ("..\\data\\Baseball_Season_1913.csv", "..\\data\\BB_GL_1913.csv"),
                 #("..\\data\\Baseball_Season_1914.csv", "..\\data\\BB_GL_1914.csv"),
                 #("..\\data\\Baseball_Season_1915.csv", "..\\data\\BB_GL_1915.csv"),
                 ("..\\data\\Baseball_Season_1916.csv", "..\\data\\BB_GL_1916.csv"),
                 ("..\\data\\Baseball_Season_1917.csv", "..\\data\\BB_GL_1917.csv"),
                 ("..\\data\\Baseball_Season_1918.csv", "..\\data\\BB_GL_1918.csv"),
                 ("..\\data\\Baseball_Season_1919.csv", "..\\data\\BB_GL_1919.csv"),
                 ("..\\data\\Baseball_Season_1920.csv", "..\\data\\BB_GL_1920.csv"),
                 ("..\\data\\Baseball_Season_1921.csv", "..\\data\\BB_GL_1921.csv"),
                 ("..\\data\\Baseball_Season_1922.csv", "..\\data\\BB_GL_1922.csv"),
                 ("..\\data\\Baseball_Season_1923.csv", "..\\data\\BB_GL_1923.csv"),
                 ("..\\data\\Baseball_Season_1924.csv", "..\\data\\BB_GL_1924.csv"),
                 ("..\\data\\Baseball_Season_1925.csv", "..\\data\\BB_GL_1925.csv"),
                 ("..\\data\\Baseball_Season_1926.csv", "..\\data\\BB_GL_1926.csv"),
                 ("..\\data\\Baseball_Season_1927.csv", "..\\data\\BB_GL_1927.csv"),
                 ("..\\data\\Baseball_Season_1928.csv", "..\\data\\BB_GL_1928.csv"),
                 ("..\\data\\Baseball_Season_1929.csv", "..\\data\\BB_GL_1929.csv"),
                 ("..\\data\\Baseball_Season_1930.csv", "..\\data\\BB_GL_1930.csv"),
                ]

NineteenTwentys_Decade = [#
                    ("..\\data\\Baseball_Season_1920.csv", "..\\data\\BB_GL_1920.csv"),
                    ("..\\data\\Baseball_Season_1921.csv", "..\\data\\BB_GL_1921.csv"),
                    ("..\\data\\Baseball_Season_1922.csv", "..\\data\\BB_GL_1922.csv"),
                    ("..\\data\\Baseball_Season_1923.csv", "..\\data\\BB_GL_1923.csv"),
                    ("..\\data\\Baseball_Season_1924.csv", "..\\data\\BB_GL_1924.csv"),
                    ("..\\data\\Baseball_Season_1925.csv", "..\\data\\BB_GL_1925.csv"),
                    ("..\\data\\Baseball_Season_1926.csv", "..\\data\\BB_GL_1926.csv"),
                    ("..\\data\\Baseball_Season_1927.csv", "..\\data\\BB_GL_1927.csv"),
                    ("..\\data\\Baseball_Season_1928.csv", "..\\data\\BB_GL_1928.csv"),
                    ("..\\data\\Baseball_Season_1929.csv", "..\\data\\BB_GL_1929.csv"),
                   ]

Golden_Era =     [
                 ("..\\data\\Baseball_Season_1926.csv", "..\\data\\BB_GL_1926.csv"),
                 ("..\\data\\Baseball_Season_1927.csv", "..\\data\\BB_GL_1927.csv"),
                 ("..\\data\\Baseball_Season_1928.csv", "..\\data\\BB_GL_1928.csv"),
                 ("..\\data\\Baseball_Season_1929.csv", "..\\data\\BB_GL_1929.csv"),
                 ("..\\data\\Baseball_Season_1930.csv", "..\\data\\BB_GL_1930.csv"),
                 ("..\\data\\Baseball_Season_1931.csv", "..\\data\\BB_GL_1931.csv"),
                 ("..\\data\\Baseball_Season_1932.csv", "..\\data\\BB_GL_1932.csv"),
                 ("..\\data\\Baseball_Season_1933.csv", "..\\data\\BB_GL_1933.csv"),
                 ("..\\data\\Baseball_Season_1934.csv", "..\\data\\BB_GL_1934.csv"),
                 ("..\\data\\Baseball_Season_1935.csv", "..\\data\\BB_GL_1935.csv"),
                 ("..\\data\\Baseball_Season_1936.csv", "..\\data\\BB_GL_1936.csv"),
                 ("..\\data\\Baseball_Season_1937.csv", "..\\data\\BB_GL_1937.csv"),
                 ("..\\data\\Baseball_Season_1938.csv", "..\\data\\BB_GL_1938.csv"),
                 ("..\\data\\Baseball_Season_1939.csv", "..\\data\\BB_GL_1939.csv"),
                 ("..\\data\\Baseball_Season_1940.csv", "..\\data\\BB_GL_1940.csv"),
                 ]

NineteenThirtys_Decade = [#
                         ("..\\data\\Baseball_Season_1930.csv", "..\\data\\BB_GL_1930.csv"),
                         ("..\\data\\Baseball_Season_1931.csv", "..\\data\\BB_GL_1931.csv"),
                         ("..\\data\\Baseball_Season_1932.csv", "..\\data\\BB_GL_1932.csv"),
                         ("..\\data\\Baseball_Season_1933.csv", "..\\data\\BB_GL_1933.csv"),
                         ("..\\data\\Baseball_Season_1934.csv", "..\\data\\BB_GL_1934.csv"),
                         ("..\\data\\Baseball_Season_1935.csv", "..\\data\\BB_GL_1935.csv"),
                         ("..\\data\\Baseball_Season_1936.csv", "..\\data\\BB_GL_1936.csv"),
                         ("..\\data\\Baseball_Season_1937.csv", "..\\data\\BB_GL_1937.csv"),
                         ("..\\data\\Baseball_Season_1938.csv", "..\\data\\BB_GL_1938.csv"),
                         ("..\\data\\Baseball_Season_1939.csv", "..\\data\\BB_GL_1939.csv"),
                         ]

# 10 years before and after Jackie - the steals era
#
Jackie_Robinson_Era = [# Raw data....................     extracted work data
                       ("..\\data\\Baseball_Season_1937.csv", "..\\data\\BB_GL_1937.csv"),
                       ("..\\data\\Baseball_Season_1938.csv", "..\\data\\BB_GL_1938.csv"),
                       ("..\\data\\Baseball_Season_1939.csv", "..\\data\\BB_GL_1939.csv"),
                       ("..\\data\\Baseball_Season_1940.csv", "..\\data\\BB_GL_1940.csv"),
                       ("..\\data\\Baseball_Season_1941.csv", "..\\data\\BB_GL_1941.csv"),
                       ("..\\data\\Baseball_Season_1942.csv", "..\\data\\BB_GL_1942.csv"),
                       ("..\\data\\Baseball_Season_1943.csv", "..\\data\\BB_GL_1943.csv"),
                       ("..\\data\\Baseball_Season_1944.csv", "..\\data\\BB_GL_1944.csv"),
                       ("..\\data\\Baseball_Season_1945.csv", "..\\data\\BB_GL_1945.csv"),
                       ("..\\data\\Baseball_Season_1946.csv", "..\\data\\BB_GL_1946.csv"),
                       ("..\\data\\Baseball_Season_1947.csv", "..\\data\\BB_GL_1947.csv"),
                       ("..\\data\\Baseball_Season_1948.csv", "..\\data\\BB_GL_1948.csv"),
                       ("..\\data\\Baseball_Season_1949.csv", "..\\data\\BB_GL_1949.csv"),
                       ("..\\data\\Baseball_Season_1950.csv", "..\\data\\BB_GL_1950.csv"),
                       ("..\\data\\Baseball_Season_1951.csv", "..\\data\\BB_GL_1951.csv"),
                       ("..\\data\\Baseball_Season_1952.csv", "..\\data\\BB_GL_1952.csv"),
                       ("..\\data\\Baseball_Season_1953.csv", "..\\data\\BB_GL_1953.csv"),
                       ("..\\data\\Baseball_Season_1954.csv", "..\\data\\BB_GL_1954.csv"),
                       ("..\\data\\Baseball_Season_1955.csv", "..\\data\\BB_GL_1955.csv"),
                       ("..\\data\\Baseball_Season_1956.csv", "..\\data\\BB_GL_1956.csv")
                       ]

NineteenFortys_Decade = [#
                      ("..\\data\\Baseball_Season_1940.csv", "..\\data\\BB_GL_1940.csv"),
                      ("..\\data\\Baseball_Season_1941.csv", "..\\data\\BB_GL_1941.csv"),
                      ("..\\data\\Baseball_Season_1942.csv", "..\\data\\BB_GL_1942.csv"),
                      ("..\\data\\Baseball_Season_1943.csv", "..\\data\\BB_GL_1943.csv"),
                      ("..\\data\\Baseball_Season_1944.csv", "..\\data\\BB_GL_1944.csv"),
                      ("..\\data\\Baseball_Season_1945.csv", "..\\data\\BB_GL_1945.csv"),
                      ("..\\data\\Baseball_Season_1946.csv", "..\\data\\BB_GL_1946.csv"),
                      ("..\\data\\Baseball_Season_1947.csv", "..\\data\\BB_GL_1947.csv"),
                      ("..\\data\\Baseball_Season_1948.csv", "..\\data\\BB_GL_1948.csv"),
                      ("..\\data\\Baseball_Season_1949.csv", "..\\data\\BB_GL_1949.csv"),
                      ]

# Including the last year of 16 teams 1960
NineteenFiftys_Decade = [#
                      ("..\\data\\Baseball_Season_1950.csv", "..\\data\\BB_GL_1950.csv"),
                      ("..\\data\\Baseball_Season_1951.csv", "..\\data\\BB_GL_1951.csv"),
                      ("..\\data\\Baseball_Season_1952.csv", "..\\data\\BB_GL_1952.csv"),
                      ("..\\data\\Baseball_Season_1953.csv", "..\\data\\BB_GL_1953.csv"),
                      ("..\\data\\Baseball_Season_1954.csv", "..\\data\\BB_GL_1954.csv"),
                      ("..\\data\\Baseball_Season_1955.csv", "..\\data\\BB_GL_1955.csv"),
                      ("..\\data\\Baseball_Season_1956.csv", "..\\data\\BB_GL_1956.csv"),
                      ("..\\data\\Baseball_Season_1957.csv", "..\\data\\BB_GL_1957.csv"),
                      ("..\\data\\Baseball_Season_1958.csv", "..\\data\\BB_GL_1958.csv"),
                      ("..\\data\\Baseball_Season_1959.csv", "..\\data\\BB_GL_1959.csv"),
                      ("..\\data\\Baseball_Season_1960.csv", "..\\data\\BB_GL_1960.csv"),
                     ]

# Skipping over the one year of 10 AL teams,8 NL teams;
# Stopping before 2nd expansion
NineteenSixtys_Decade = [#
                      ("..\\data\\Baseball_Season_1962.csv", "..\\data\\BB_GL_1962.csv"),
                      ("..\\data\\Baseball_Season_1963.csv", "..\\data\\BB_GL_1963.csv"),
                      ("..\\data\\Baseball_Season_1964.csv", "..\\data\\BB_GL_1964.csv"),
                      ("..\\data\\Baseball_Season_1965.csv", "..\\data\\BB_GL_1965.csv"),
                      ("..\\data\\Baseball_Season_1966.csv", "..\\data\\BB_GL_1966.csv"),
                      ("..\\data\\Baseball_Season_1967.csv", "..\\data\\BB_GL_1967.csv"),
                      ("..\\data\\Baseball_Season_1968.csv", "..\\data\\BB_GL_1968.csv"),
                     ]
Twenty_Team_Era = NineteenSixtys_Decade

TwentyFour_Team_Era =   [#
                        ("..\\data\\Baseball_Season_1969.csv", "..\\data\\BB_GL_1969.csv"),
                        ("..\\data\\Baseball_Season_1970.csv", "..\\data\\BB_GL_1970.csv"),
                        ("..\\data\\Baseball_Season_1971.csv", "..\\data\\BB_GL_1971.csv"),
                        ("..\\data\\Baseball_Season_1972.csv", "..\\data\\BB_GL_1972.csv"),
                        ("..\\data\\Baseball_Season_1973.csv", "..\\data\\BB_GL_1973.csv"),
                        ("..\\data\\Baseball_Season_1974.csv", "..\\data\\BB_GL_1974.csv"),
                        ("..\\data\\Baseball_Season_1975.csv", "..\\data\\BB_GL_1975.csv"),
                        ("..\\data\\Baseball_Season_1976.csv", "..\\data\\BB_GL_1976.csv"),
                        ]

#Only until the next expansion
NineteenSeventys_Decade = [#
                        ("..\\data\\Baseball_Season_1970.csv", "..\\data\\BB_GL_1970.csv"),
                        ("..\\data\\Baseball_Season_1971.csv", "..\\data\\BB_GL_1971.csv"),
                        ("..\\data\\Baseball_Season_1972.csv", "..\\data\\BB_GL_1972.csv"),
                        ("..\\data\\Baseball_Season_1973.csv", "..\\data\\BB_GL_1973.csv"),
                        ("..\\data\\Baseball_Season_1974.csv", "..\\data\\BB_GL_1974.csv"),
                        ("..\\data\\Baseball_Season_1975.csv", "..\\data\\BB_GL_1975.csv"),
                        ("..\\data\\Baseball_Season_1976.csv", "..\\data\\BB_GL_1976.csv"),
                        ]

# Why not Weaver's entire mngr career (1969...1982) ?
# Please see this essay
# https://sabr.org/journal/article/1977-when-earl-weaver-became-earl-weaver/
#
# This defined period overlaps the DH-Pre-Inter-league play Period (1973...1996)
Earl_Weaver_Era = [# Raw data....................     extracted work data
                   ("..\\data\\Baseball_Season_1977.csv", "..\\data\\BB_GL_1977.csv"),
                   ("..\\data\\Baseball_Season_1978.csv", "..\\data\\BB_GL_1978.csv"),
                   ("..\\data\\Baseball_Season_1979.csv", "..\\data\\BB_GL_1979.csv"),
                   ("..\\data\\Baseball_Season_1980.csv", "..\\data\\BB_GL_1980.csv"),
                   ("..\\data\\Baseball_Season_1981.csv", "..\\data\\BB_GL_1981.csv"),
                   ("..\\data\\Baseball_Season_1982.csv", "..\\data\\BB_GL_1982.csv"),
                   ("..\\data\\Baseball_Season_1983.csv", "..\\data\\BB_GL_1983.csv"),
                   ("..\\data\\Baseball_Season_1984.csv", "..\\data\\BB_GL_1984.csv"),
                   ("..\\data\\Baseball_Season_1985.csv", "..\\data\\BB_GL_1985.csv"),
                   ("..\\data\\Baseball_Season_1986.csv", "..\\data\\BB_GL_1986.csv"),
                  ]

# This Period begins with A.L expansion to 14 teams and ends w/ inter-league play
# four years after the DH starts
DH_Pre_Interleague_Era =  [# Raw data....................     extracted work data
                           ("..\\data\\Baseball_Season_1977.csv", "..\\data\\BB_GL_1977.csv"),
                           ("..\\data\\Baseball_Season_1978.csv", "..\\data\\BB_GL_1978.csv"),
                           ("..\\data\\Baseball_Season_1979.csv", "..\\data\\BB_GL_1979.csv"),
                           ("..\\data\\Baseball_Season_1980.csv", "..\\data\\BB_GL_1980.csv"),
                           ("..\\data\\Baseball_Season_1981.csv", "..\\data\\BB_GL_1981.csv"),
                           ("..\\data\\Baseball_Season_1982.csv", "..\\data\\BB_GL_1982.csv"),
                           ("..\\data\\Baseball_Season_1983.csv", "..\\data\\BB_GL_1983.csv"),
                           ("..\\data\\Baseball_Season_1984.csv", "..\\data\\BB_GL_1984.csv"),
                           ("..\\data\\Baseball_Season_1985.csv", "..\\data\\BB_GL_1985.csv"),
                           ("..\\data\\Baseball_Season_1986.csv", "..\\data\\BB_GL_1986.csv"),
                           ("..\\data\\Baseball_Season_1987.csv", "..\\data\\BB_GL_1987.csv"),
                           ("..\\data\\Baseball_Season_1988.csv", "..\\data\\BB_GL_1988.csv"),
                           ("..\\data\\Baseball_Season_1989.csv", "..\\data\\BB_GL_1989.csv"),
                           ("..\\data\\Baseball_Season_1990.csv", "..\\data\\BB_GL_1990.csv"),
                           ("..\\data\\Baseball_Season_1991.csv", "..\\data\\BB_GL_1991.csv"),
                           ("..\\data\\Baseball_Season_1992.csv", "..\\data\\BB_GL_1992.csv"),
                           ("..\\data\\Baseball_Season_1993.csv", "..\\data\\BB_GL_1993.csv"),
                           ("..\\data\\Baseball_Season_1994.csv", "..\\data\\BB_GL_1994.csv"),
                           ("..\\data\\Baseball_Season_1995.csv", "..\\data\\BB_GL_1995.csv"),
                           ("..\\data\\Baseball_Season_1996.csv", "..\\data\\BB_GL_1996.csv"),
                           ]

NineteenEightys_Decade = [#
                        ("..\\data\\Baseball_Season_1980.csv", "..\\data\\BB_GL_1980.csv"),
                        ("..\\data\\Baseball_Season_1981.csv", "..\\data\\BB_GL_1981.csv"),
                        ("..\\data\\Baseball_Season_1982.csv", "..\\data\\BB_GL_1982.csv"),
                        ("..\\data\\Baseball_Season_1983.csv", "..\\data\\BB_GL_1983.csv"),
                        ("..\\data\\Baseball_Season_1984.csv", "..\\data\\BB_GL_1984.csv"),
                        ("..\\data\\Baseball_Season_1985.csv", "..\\data\\BB_GL_1985.csv"),
                        ("..\\data\\Baseball_Season_1986.csv", "..\\data\\BB_GL_1986.csv"),
                        ("..\\data\\Baseball_Season_1987.csv", "..\\data\\BB_GL_1987.csv"),
                        ("..\\data\\Baseball_Season_1988.csv", "..\\data\\BB_GL_1988.csv"),
                        ("..\\data\\Baseball_Season_1989.csv", "..\\data\\BB_GL_1989.csv"),
                        ]

# The few years of 28 teams
NineteenNinetys_Decade = [#
                        ("..\\data\\Baseball_Season_1983.csv", "..\\data\\BB_GL_1983.csv"),
                        ("..\\data\\Baseball_Season_1984.csv", "..\\data\\BB_GL_1984.csv"),
                        ("..\\data\\Baseball_Season_1985.csv", "..\\data\\BB_GL_1985.csv"),
                        ("..\\data\\Baseball_Season_1986.csv", "..\\data\\BB_GL_1986.csv"),
                        ("..\\data\\Baseball_Season_1987.csv", "..\\data\\BB_GL_1987.csv"),
                        ]

TwentySix_Team_Era  =  [# Raw data....................     extracted work data
                        ("..\\data\\Baseball_Season_1977.csv", "..\\data\\BB_GL_1977.csv"),
                        ("..\\data\\Baseball_Season_1978.csv", "..\\data\\BB_GL_1978.csv"),
                        ("..\\data\\Baseball_Season_1979.csv", "..\\data\\BB_GL_1979.csv"),
                        ("..\\data\\Baseball_Season_1980.csv", "..\\data\\BB_GL_1980.csv"),
                        ("..\\data\\Baseball_Season_1981.csv", "..\\data\\BB_GL_1981.csv"),
                        ("..\\data\\Baseball_Season_1982.csv", "..\\data\\BB_GL_1982.csv"),
                        ("..\\data\\Baseball_Season_1983.csv", "..\\data\\BB_GL_1983.csv"),
                        ("..\\data\\Baseball_Season_1984.csv", "..\\data\\BB_GL_1984.csv"),
                        ("..\\data\\Baseball_Season_1985.csv", "..\\data\\BB_GL_1985.csv"),
                        ("..\\data\\Baseball_Season_1986.csv", "..\\data\\BB_GL_1986.csv"),
                        ("..\\data\\Baseball_Season_1987.csv", "..\\data\\BB_GL_1987.csv"),
                        ("..\\data\\Baseball_Season_1988.csv", "..\\data\\BB_GL_1988.csv"),
                        ("..\\data\\Baseball_Season_1989.csv", "..\\data\\BB_GL_1989.csv"),
                        ("..\\data\\Baseball_Season_1990.csv", "..\\data\\BB_GL_1990.csv"),
                        ("..\\data\\Baseball_Season_1991.csv", "..\\data\\BB_GL_1991.csv"),
                        ("..\\data\\Baseball_Season_1992.csv", "..\\data\\BB_GL_1992.csv"),
                       ]

Twenty0X_Decade = [# Raw data....................     extracted work data
                   ("..\\data\\Baseball_Season_2000.csv", "..\\data\\BB_GL_2000.csv"),
                   ("..\\data\\Baseball_Season_2001.csv", "..\\data\\BB_GL_2001.csv"),
                   ("..\\data\\Baseball_Season_2002.csv", "..\\data\\BB_GL_2002.csv"),
                   ("..\\data\\Baseball_Season_2003.csv", "..\\data\\BB_GL_2003.csv"),
                   ("..\\data\\Baseball_Season_2004.csv", "..\\data\\BB_GL_2004.csv"),
                   ("..\\data\\Baseball_Season_2005.csv", "..\\data\\BB_GL_2005.csv"),
                   ("..\\data\\Baseball_Season_2006.csv", "..\\data\\BB_GL_2006.csv"),
                   ("..\\data\\Baseball_Season_2007.csv", "..\\data\\BB_GL_2007.csv"),
                   ("..\\data\\Baseball_Season_2008.csv", "..\\data\\BB_GL_2008.csv"),
                   ("..\\data\\Baseball_Season_2009.csv", "..\\data\\BB_GL_2009.csv")
                  ]

TwentyTeens_Decade = [# Raw data....................     extracted work data
                 ("..\\data\\Baseball_Season_2010.csv", "..\\data\\BB_GL_2010.csv"),
                 ("..\\data\\Baseball_Season_2011.csv", "..\\data\\BB_GL_2011.csv"),
                 ("..\\data\\Baseball_Season_2012.csv", "..\\data\\BB_GL_2012.csv"),
                 ("..\\data\\Baseball_Season_2013.csv", "..\\data\\BB_GL_2013.csv"),
                 ("..\\data\\Baseball_Season_2014.csv", "..\\data\\BB_GL_2014.csv"),
                 ("..\\data\\Baseball_Season_2015.csv", "..\\data\\BB_GL_2015.csv"),
                 ("..\\data\\Baseball_Season_2016.csv", "..\\data\\BB_GL_2016.csv"),
                 ("..\\data\\Baseball_Season_2017.csv", "..\\data\\BB_GL_2017.csv"),
                 ("..\\data\\Baseball_Season_2018.csv", "..\\data\\BB_GL_2018.csv"),
                 ("..\\data\\Baseball_Season_2019.csv", "..\\data\\BB_GL_2019.csv")
                 ]
Current_Era = TwentyTeens_Decade
#RawWorkDataFiles = Current_Era

NUM_TEAMS = 30
NUM_GAMES_PER_SEASON = 162
NUM_GAMES_PRE_NORMALIZE = (NUM_GAMES_PER_SEASON - 130)

# This is the list of city codes which are used in the input data set.
# It is ordered so that indeces for this list match indeces for the
# alphabetizied team names in the 2-D list below.
# I.e. ARI(zona) should be the same index as Diamondbacks,Phoenix below 
# CITY_CODES.index("ARI") == 8
CITY_CODES = ("ANA", "HOU", "OAK", "ATL", "TOR", "MIL", "SLN", "CHN", "ARI", "LAN", \
             "SFN", "CLE", "SEA", "MIA", "NYN", "WAS", "BAL", "SDN", "PHI", "PIT", \
             "TBA", "TEX", "BOS", "CIN", "COL", "KCA", "DET", "MIN", "CHA", "NYA"  \
             ) 

# List of teams and cities sorted by team name
# strings are immutable, this table is a 2D-tuple used for printing
Teams = (
         ('Angels', 'Anaheim', 0),       \
         ('Astros', 'Houston', 0),       \
         ('Athletics', 'Oakland', 0),    \
         ('Braves', 'Atlanta', 0),       \
         ('Blue Jays', 'Toronto', 0),    \
         ('Brewers', 'Milwaukee', 0),    \
         ('Cardinals', 'St. Louis', 0),  \
         ('Cubs',   'Chicago', 0),       \
         ('Diamondbacks', 'Phoenix', 0), \
         ('Dodgers', 'Los Angeles', 0),   \
         ('Giants', 'San Francisco', 0), \
         ('Indians', 'Cleveland', 0),    \
         ('Mariners', 'Seattle', 0),     \
         ('Marlins', 'Miami', 0),        \
         ('Mets', 'New York', 0),        \
         ('Nationals', 'Washington', 0), \
         ('Orioles', 'Baltimore', 0),     \
         ('Padres', 'San Diego', 0),     \
         ('Phillies', 'Philadelphia', 0), \
         ('Pirates', 'Pittsburgh', 0),    \
         ('Rays', 'Tampa Bay', 0),       \
         ('Rangers', 'Arlingtion', 0),   \
         ('Red Sox', 'Boston', 0),       \
         ('Reds', 'Cincinnati', 0),      \
         ('Rockies', 'Denver', 0),       \
         ('Royals', 'Kansas City', 0),   \
         ('Tigers', 'Detroit', 0),       \
         ('Twins',  'Minneapolis', 0),   \
         ('White Sox', 'Chicago', 0),    \
         ('Yankees', 'New York', 0)       \
         )
