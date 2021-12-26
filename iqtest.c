#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int ROW_LENGTHS[] = {1, 2, 3, 4, 5};
static const int ROW_STARTS[] = {0, 1, 3, 6, 10};

static const int N_ROWS = 5;

static const uint16_t FULL_BOARD = 0x7fff;

typedef struct jump {
    uint16_t start;
    uint16_t middle;
    uint16_t end;
} jump;

typedef struct state {
    int emptyRow;
    int emptyHole;
    uint16_t board;
    uint8_t moveIdx;
    uint8_t moves[13];
} state;

static uint16_t holeMask(int row, int hole) {
    return (1U << (ROW_STARTS[row] + hole));
}

static bool isValidRowHole(int row, int hole) {
    return row >= 0 && row < N_ROWS && hole >= 0 && hole < ROW_LENGTHS[row];
}

static int enumerateJumps(jump *jumps) {
    int jidx = 0;
    for (int row = 0; row < N_ROWS; ++row) {
        for (int hole = 0; hole < ROW_LENGTHS[row]; ++hole) {
            const uint16_t start_mask = holeMask(row, hole);

            static const int middle_rows[] = {-1, -1, 0, 0, 1, 1};
            static const int middle_holes[] = {-1, 1, -1, 1, 0, 1};
            static const int end_rows[] = {-2, -2, 0, 0, 2, 2};
            static const int end_holes[] = {-2, 2, -2, 2, 0, 2};

            for (int i = 0; i < 6; ++i) {
                const int middle_row = row + middle_rows[i];
                const int middle_hole = hole + middle_holes[i];
                const int end_row = row + end_rows[i];
                const int end_hole = hole + end_holes[i];

                if (isValidRowHole(middle_row, middle_hole) &&
                    isValidRowHole(end_row, end_hole)) {
                    jumps[jidx].start = start_mask;
                    jumps[jidx].middle = holeMask(middle_row, middle_hole);
                    jumps[jidx].end = holeMask(end_row, end_hole);
                    ++jidx;
                }
            }
        }
    }
    return jidx;
}

static void step(int *outcomes, int noutcomes, jump *jumps, int njumps,
                 state *s) {
    const int pop = __builtin_popcount(s->board);
    if (pop == 1) {
        outcomes[1]++;
        free(s);
        return;
    }
    bool didJump = false;
    for (int jidx = 0; jidx < njumps; ++jidx) {
        if ((s->board & jumps[jidx].start) && (s->board & jumps[jidx].middle) &&
            (~(s->board) & jumps[jidx].end)) {
            // This is a valid jump.
            didJump = true;
            state *newstate = calloc(1, sizeof(state));
            memcpy(newstate, s, sizeof(state));

            newstate->moves[newstate->moveIdx++] = (uint8_t)jidx;
            newstate->board =
                (((s->board & ~jumps[jidx].start) & ~jumps[jidx].middle) |
                 jumps[jidx].end);
            step(outcomes, noutcomes, jumps, njumps, newstate);
        }
    }
    // Didn't jump, so this is an end state.
    if (!didJump) {
        outcomes[pop]++;
    }

    free(s);
}

int main(int argc, char **argv) {
    int outcomes[15] = {0};
    jump jumps[31];
    const int njumps = enumerateJumps(jumps);
    for (int row = 0; row < N_ROWS; ++row) {
        for (int hole = 0; hole < ROW_LENGTHS[row]; ++hole) {
            state *s = calloc(1, sizeof(state));
            s->emptyRow = row;
            s->emptyHole = hole;
            s->board = FULL_BOARD & ~holeMask(row, hole);

            step(outcomes, 15, jumps, njumps, s);
        }
    }

    int total = 0;
    for (int i = 0; i < 15; i++) {
        printf("%d left: %d\n", i, outcomes[i]);
        total += outcomes[i];
    }
    printf("Total outcomes: %d\n", total);
    return 0;
}
