#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX 50

// ================= RESULT STRUCT =================
typedef struct {
    int faults;
    float fault_rate;
    float hit_rate;
    char eviction_log[MAX][64];
    int log_count;
} SimResult;

// ================= SEARCH =================
int search(int key, int frame[], int frame_size) {
    for (int i = 0; i < frame_size; i++)
        if (frame[i] == key) return 1;
    return 0;
}

// ================= PRINT MATRIX TABLE =================
void printMatrix(int matrix[MAX][MAX], char miss[], int frame_size, int n) {
    printf("\n");
    for (int i = frame_size - 1; i >= 0; i--) {
        printf("  Frame %d   ", i + 1);
        for (int j = 0; j < n; j++) {
            if (matrix[i][j] == -1)
                printf("%-4s", "-");
            else
                printf("%-4d", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n           ");
    for (int i = 0; i < n; i++)
        printf("%-4c", miss[i]);
    printf("\n");
}

// ================= INPUT VALIDATION =================
int getValidInt(const char *prompt, int min, int max_val) {
    int val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &val) == 1 && val >= min && val <= max_val)
            return val;
        printf("  Invalid input. Enter a number between %d and %d.\n", min, max_val);
        while (getchar() != '\n');
    }
}

// ================= PRINT FRAME STATE =================
void printFrameState(int frame[], int frame_size, int current_page, int step, int n,
                     int is_fault, const char *evicted, int hits, int faults) {
    printf("\n  Step %d/%d  ->  Page: %d  |  %s",
           step, n, current_page, is_fault ? "FAULT" : "HIT");
    if (evicted[0] != '\0')
        printf("  (evicted: %s)", evicted);
    printf("\n  Frames: [ ");
    for (int i = 0; i < frame_size; i++) {
        if (frame[i] == -1) printf("-  ");
        else printf("%d  ", frame[i]);
    }
    int total = hits + faults;
    printf("]  Hits: %d  Faults: %d  Hit Rate: %.1f%%\n",
           hits, faults, total == 0 ? 0.0f : 100.0f * hits / total);
}

// ================= PAUSE FOR STEP MODE =================
void pauseStep() {
    printf("  [Press ENTER to continue...]");
    while (getchar() != '\n');
}

// ================= SAVE TO FILE =================
void saveResultsToFile(const char *algo, int pages[], int n, int frame_size, SimResult *r) {
    FILE *f = fopen("simulation_results.txt", "a");
    if (!f) { printf("  Could not open file.\n"); return; }

    time_t t = time(NULL);
    fprintf(f, "\n========================================\n");
    fprintf(f, "Algorithm  : %s\n", algo);
    fprintf(f, "Date/Time  : %s", ctime(&t));
    fprintf(f, "Frame Size : %d\n", frame_size);
    fprintf(f, "Page String: ");
    for (int i = 0; i < n; i++) fprintf(f, "%d ", pages[i]);
    fprintf(f, "\nPage Faults: %d\n", r->faults);
    fprintf(f, "Fault Rate : %.2f\n", r->fault_rate);
    fprintf(f, "Hit Rate   : %.2f\n", r->hit_rate);
    fprintf(f, "Eviction Log:\n");
    for (int i = 0; i < r->log_count; i++)
        fprintf(f, "  %s\n", r->eviction_log[i]);
    fprintf(f, "========================================\n");
    fclose(f);
    printf("  Results saved to simulation_results.txt\n");
}

// ================= FIFO =================
SimResult FIFO(int pages[], int n, int frame_size, int display, int step_mode) {
    SimResult res = {0, 0.0f, 0.0f, {""}, 0};
    int frame[MAX], matrix[MAX][MAX], index = 0, page_faults = 0, hits = 0;
    char miss[MAX];

    for (int i = 0; i < frame_size; i++) frame[i] = -1;

    if (display) {
        printf("\n--- FIFO ---\n");
        printf("First-In First-Out: Replaces the page that was loaded earliest.\n");
    }

    for (int i = 0; i < n; i++) {
        char evicted[32] = "";
        int is_fault = 0;

        if (!search(pages[i], frame, frame_size)) {
            if (frame[index] != -1)
                sprintf(evicted, "page %d", frame[index]);
            frame[index] = pages[i];
            index = (index + 1) % frame_size;
            page_faults++;
            miss[i] = '*';
            is_fault = 1;
        } else {
            miss[i] = ' ';
            hits++;
        }

        for (int j = 0; j < frame_size; j++)
            matrix[j][i] = frame[j];

        if (display)
            printFrameState(frame, frame_size, pages[i], i + 1, n,
                            is_fault, evicted, hits, page_faults);

        if (step_mode) pauseStep();

        if (is_fault && res.log_count < MAX) {
            if (evicted[0] != '\0')
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d replaced %s", i+1, pages[i], evicted);
            else
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d loaded into empty frame", i+1, pages[i]);
        }
    }

    res.faults = page_faults;
    res.fault_rate = (float)page_faults / n;
    res.hit_rate = (float)hits / n;

    if (display) {
        printf("\n  Reference String: ");
        for (int i = 0; i < n; i++) printf("%-4d", pages[i]);
        printf("\n");
        printMatrix(matrix, miss, frame_size, n);
        printf("\n  FIFO Page Faults = %d\n", page_faults);
        printf("  Hit Rate         = %.2f\n", res.hit_rate);
        printf("  Fault Rate       = %.2f\n", res.fault_rate);
    }

    return res;
}

// ================= LRU =================
SimResult LRU(int pages[], int n, int frame_size, int display, int step_mode) {
    SimResult res = {0, 0.0f, 0.0f, {""}, 0};
    int frame[MAX], matrix[MAX][MAX], lru_time[MAX], page_faults = 0, hits = 0, counter = 0;
    char miss[MAX];

    for (int i = 0; i < frame_size; i++) { frame[i] = -1; lru_time[i] = 0; }

    if (display) {
        printf("\n--- LRU ---\n");
        printf("Least Recently Used: Replaces the page that has not been used for the longest time.\n");
    }

    for (int i = 0; i < n; i++) {
        counter++;
        char evicted[32] = "";
        int is_fault = 0, found = 0;

        for (int j = 0; j < frame_size; j++) {
            if (frame[j] == pages[i]) {
                lru_time[j] = counter;
                found = 1;
                miss[i] = ' ';
                hits++;
                break;
            }
        }

        if (!found) {
            int lru = 0;
            for (int j = 1; j < frame_size; j++)
                if (lru_time[j] < lru_time[lru]) lru = j;
            if (frame[lru] != -1)
                sprintf(evicted, "page %d", frame[lru]);
            frame[lru] = pages[i];
            lru_time[lru] = counter;
            page_faults++;
            miss[i] = '*';
            is_fault = 1;
        }

        for (int j = 0; j < frame_size; j++)
            matrix[j][i] = frame[j];

        if (display)
            printFrameState(frame, frame_size, pages[i], i + 1, n,
                            is_fault, evicted, hits, page_faults);

        if (step_mode) pauseStep();

        if (is_fault && res.log_count < MAX) {
            if (evicted[0] != '\0')
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d replaced %s (LRU)", i+1, pages[i], evicted);
            else
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d loaded into empty frame", i+1, pages[i]);
        }
    }

    res.faults = page_faults;
    res.fault_rate = (float)page_faults / n;
    res.hit_rate = (float)hits / n;

    if (display) {
        printf("\n  Reference String: ");
        for (int i = 0; i < n; i++) printf("%-4d", pages[i]);
        printf("\n");
        printMatrix(matrix, miss, frame_size, n);
        printf("\n  LRU Page Faults = %d\n", page_faults);
        printf("  Hit Rate        = %.2f\n", res.hit_rate);
        printf("  Fault Rate      = %.2f\n", res.fault_rate);
    }

    return res;
}

// ================= OPTIMAL - predict helper =================
int predict(int pages[], int frame[], int n, int index, int frame_size) {
    int farthest = index, pos = -1;
    for (int i = 0; i < frame_size; i++) {
        int j;
        for (j = index; j < n; j++) {
            if (frame[i] == pages[j]) {
                if (j > farthest) { farthest = j; pos = i; }
                break;
            }
        }
        if (j == n) return i;
    }
    return (pos == -1) ? 0 : pos;
}

// ================= OPTIMAL =================
SimResult Optimal(int pages[], int n, int frame_size, int display, int step_mode) {
    SimResult res = {0, 0.0f, 0.0f, {""}, 0};
    int frame[MAX], matrix[MAX][MAX], page_faults = 0, hits = 0;
    char miss[MAX];

    for (int i = 0; i < frame_size; i++) frame[i] = -1;

    if (display) {
        printf("\n--- Optimal ---\n");
        printf("Optimal: Replaces the page that will not be used for the longest time in the future.\n");
    }

    for (int i = 0; i < n; i++) {
        char evicted[32] = "";
        int is_fault = 0;

        if (!search(pages[i], frame, frame_size)) {
            int pos = -1;
            for (int j = 0; j < frame_size; j++)
                if (frame[j] == -1) { pos = j; break; }
            if (pos == -1) pos = predict(pages, frame, n, i + 1, frame_size);
            if (frame[pos] != -1)
                sprintf(evicted, "page %d", frame[pos]);
            frame[pos] = pages[i];
            page_faults++;
            miss[i] = '*';
            is_fault = 1;
        } else {
            miss[i] = ' ';
            hits++;
        }

        for (int j = 0; j < frame_size; j++)
            matrix[j][i] = frame[j];

        if (display)
            printFrameState(frame, frame_size, pages[i], i + 1, n,
                            is_fault, evicted, hits, page_faults);

        if (step_mode) pauseStep();

        if (is_fault && res.log_count < MAX) {
            if (evicted[0] != '\0')
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d replaced %s (OPT)", i+1, pages[i], evicted);
            else
                sprintf(res.eviction_log[res.log_count++],
                        "Step %d: page %d loaded into empty frame", i+1, pages[i]);
        }
    }

    res.faults = page_faults;
    res.fault_rate = (float)page_faults / n;
    res.hit_rate = (float)hits / n;

    if (display) {
        printf("\n  Reference String: ");
        for (int i = 0; i < n; i++) printf("%-4d", pages[i]);
        printf("\n");
        printMatrix(matrix, miss, frame_size, n);
        printf("\n  Optimal Page Faults = %d\n", page_faults);
        printf("  Hit Rate            = %.2f\n", res.hit_rate);
        printf("  Fault Rate          = %.2f\n", res.fault_rate);
    }

    return res;
}

// ================= GENERATE RANDOM STRING =================
void generateRandom(int pages[], int *n) {
    int range;
    *n = getValidInt("  Enter number of pages to generate (1-50): ", 1, MAX);
    range = getValidInt("  Enter page range (pages will be 0 to N-1): ", 2, 20);

    srand((unsigned int)time(NULL));
    printf("  Generated Reference String: ");
    for (int i = 0; i < *n; i++) {
        pages[i] = rand() % range;
        printf("%d ", pages[i]);
    }
    printf("\n");
}

// ================= FRAME SWEEP =================
void frameSweep(int pages[], int n, int algo) {
    printf("\n--- Frame Size Sweep (frames 1 to 10) ---\n\n");
    printf("  %-8s  %-8s  %-12s  %s\n", "Frames", "Faults", "Fault Rate", "Bar");
    printf("  -----------------------------------------------\n");

    int prev_faults = -1, max_faults = 0;

    for (int fs = 1; fs <= 10; fs++) {
        SimResult r;
        if (algo == 1)      r = FIFO(pages, n, fs, 0, 0);
        else if (algo == 2) r = LRU(pages, n, fs, 0, 0);
        else                r = Optimal(pages, n, fs, 0, 0);
        if (r.faults > max_faults) max_faults = r.faults;
    }

    for (int fs = 1; fs <= 10; fs++) {
        SimResult r;
        if (algo == 1)      r = FIFO(pages, n, fs, 0, 0);
        else if (algo == 2) r = LRU(pages, n, fs, 0, 0);
        else                r = Optimal(pages, n, fs, 0, 0);

        int anomaly = (prev_faults != -1 && r.faults > prev_faults && algo == 1);
        int bar_len = max_faults == 0 ? 0 : (r.faults * 20) / max_faults;

        printf("  %-8d  %-8d  %-12.2f  ", fs, r.faults, r.fault_rate);
        for (int b = 0; b < bar_len; b++) printf("#");
        if (anomaly) printf("  <- Belady's Anomaly!");
        printf("\n");

        prev_faults = r.faults;
    }

    printf("\n  Note: Belady's Anomaly = more frames causes MORE faults (FIFO only)\n");
}

// ================= COMPARE ALL =================
void compareAll(int pages[], int n, int frame_size) {
    SimResult r1 = FIFO(pages, n, frame_size, 0, 0);
    SimResult r2 = LRU(pages, n, frame_size, 0, 0);
    SimResult r3 = Optimal(pages, n, frame_size, 0, 0);

    printf("\n  Reference String: ");
    for (int i = 0; i < n; i++) printf("%-3d", pages[i]);

    printf("\n\n  -------------------------------------------\n");
    printf("  %-10s  %-8s  %-12s  %-10s\n", "Algorithm", "Faults", "Fault Rate", "Hit Rate");
    printf("  -------------------------------------------\n");
    printf("  %-10s  %-8d  %-12.2f  %-10.2f\n", "FIFO",    r1.faults, r1.fault_rate, r1.hit_rate);
    printf("  %-10s  %-8d  %-12.2f  %-10.2f\n", "LRU",     r2.faults, r2.fault_rate, r2.hit_rate);
    printf("  %-10s  %-8d  %-12.2f  %-10.2f\n", "Optimal", r3.faults, r3.fault_rate, r3.hit_rate);
    printf("  -------------------------------------------\n");

    int min_f = r1.faults;
    const char *best = "FIFO";
    if (r2.faults < min_f) { min_f = r2.faults; best = "LRU"; }
    if (r3.faults < min_f) { min_f = r3.faults; best = "Optimal"; }

    printf("  Best Algorithm: %s (Least Faults = %d)\n", best, min_f);

    printf("\n  Fault Comparison (# = faults):\n");
    int mx = r1.faults > r2.faults ? r1.faults : r2.faults;
    if (r3.faults > mx) mx = r3.faults;

    SimResult *results[3];
    results[0] = &r1; results[1] = &r2; results[2] = &r3;
    const char *names[] = {"FIFO", "LRU", "Optimal"};
    for (int i = 0; i < 3; i++) {
        int bar = mx == 0 ? 0 : (results[i]->faults * 20) / mx;
        printf("  %-10s  ", names[i]);
        for (int b = 0; b < bar; b++) printf("#");
        printf("  %d\n", results[i]->faults);
    }
}

// ================= MAIN =================
int main() {
    int pages[MAX], num = 0, frame_size, repeat, choice;

    do {
        printf("\n=====================================\n");
        printf("   PAGE REPLACEMENT SIMULATOR\n");
        printf("=====================================\n");
        printf("1. FIFO\n");
        printf("2. LRU\n");
        printf("3. Optimal\n");
        printf("4. Compare All\n");
        printf("5. Frame Sweep (Belady's Anomaly Detector)\n");
        printf("6. Generate Random Reference String\n");
        printf("-------------------------------------\n");

        choice = getValidInt("Enter choice (1-6): ", 1, 6);

        if (choice == 6) {
            generateRandom(pages, &num);
            printf("Random string ready.\n");
            printf("\nRun again? (1=Yes / 0=Exit): ");
            scanf("%d", &repeat);
            continue;
        }

        if (num > 0) {
            printf("\nExisting string: ");
            for (int i = 0; i < num; i++) printf("%d ", pages[i]);
            printf("\nUse existing string? (1=Yes / 0=Enter new): ");
            int use; scanf("%d", &use);
            if (!use) num = 0;
        }

        if (num == 0) {
            num = getValidInt("\nEnter number of pages (1-50): ", 1, MAX);
            printf("Enter page reference string:\n> ");
            for (int i = 0; i < num; i++) scanf("%d", &pages[i]);
        }

        if (choice >= 1 && choice <= 4)
            frame_size = getValidInt("Enter number of frames (1-20): ", 1, 20);

        int step_mode = 0;
        if (choice >= 1 && choice <= 3) {
            printf("Step-by-step mode? (1=Yes / 0=No): ");
            scanf("%d", &step_mode);
            if (step_mode)
                while (getchar() != '\n');
        }

        SimResult res;
        res.faults = 0; res.fault_rate = 0; res.hit_rate = 0; res.log_count = 0;

        if (choice == 1) {
            res = FIFO(pages, num, frame_size, 1, step_mode);
            printf("\nSave results to file? (1=Yes / 0=No): ");
            int sv; scanf("%d", &sv);
            if (sv) saveResultsToFile("FIFO", pages, num, frame_size, &res);

        } else if (choice == 2) {
            res = LRU(pages, num, frame_size, 1, step_mode);
            printf("\nSave results to file? (1=Yes / 0=No): ");
            int sv; scanf("%d", &sv);
            if (sv) saveResultsToFile("LRU", pages, num, frame_size, &res);

        } else if (choice == 3) {
            res = Optimal(pages, num, frame_size, 1, step_mode);
            printf("\nSave results to file? (1=Yes / 0=No): ");
            int sv; scanf("%d", &sv);
            if (sv) saveResultsToFile("Optimal", pages, num, frame_size, &res);

        } else if (choice == 4) {
            compareAll(pages, num, frame_size);

        } else if (choice == 5) {
            int algo;
            printf("\nSelect Algorithm for Sweep:\n1. FIFO\n2. LRU\n3. Optimal\n");
            algo = getValidInt("Choice: ", 1, 3);
            frameSweep(pages, num, algo);
        }

        printf("\nRun again? (1=Yes / 0=Exit): ");
        scanf("%d", &repeat);

    } while (repeat == 1);

    printf("\nProgram Ended.\n");
    return 0;
}