def getWrongAnswers(N: int, C: str) -> str:
    R = []
    for i in range(0, len(C)):
        if C[i] == 'A':
            r = 'B'
        else:
            r = 'A'
        R.append(r)

    Rstr = "".join(str(i) for i in R)
    return Rstr
#================================================
