#include "mynet.h"
#include <stdlib.h>
#include <time.h>

#define QUESTION_LEN 100
#define MAX_VALUE 100.0

static char Question[QUESTION_LEN];
static int Right_answer;

/* 問題文作成関数 */
char *make_question()
{
    int x, y;

    srand(time(NULL));

    x = 1 + (int) (MAX_VALUE * rand() / (RAND_MAX + 1.0));
    y = 1 + (int) (MAX_VALUE * rand() / (RAND_MAX + 1.0));

    snprintf(Question, QUESTION_LEN, "Question: %d * %d = ? ", x, y);

    Right_answer = x * y;

    return (Question);
}

/* 解答チェック関数 */
int check_answer(char *answer) { return (atoi(answer) == Right_answer); }
