#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cstring>

using namespace std;

static const int N = 9;

enum MessageTag {
    TAG_SEND_TASK,
    TAG_NO_MORE_TASK,
    TAG_SOLUTION_FOUND,
    TAG_SOLUTION_FAIL,
    TAG_TERMINATE
};

bool isValid(const int M[81], int row, int col, int val) {
    // 检查行
    for (int j = 0; j < N; j++) {
        if (M[row * 9 + j] == val) return false;
    }
    // 检查列
    for (int i = 0; i < N; i++) {
        if (M[i * 9 + col] == val) return false;
    }
    // 检查3x3格子
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = startRow; i < startRow + 3; i++) {
        for (int j = startCol; j < startCol + 3; j++) {
            if (M[i * 9 + j] == val) return false;
        }
    }
    return true;
}

bool findEmpty(const int M[81], int &row, int &col) {
    for (int i = 0; i < 81; i++) {
        if (M[i] == 0) {
            row = i / 9;
            col = i % 9;
            return true;
        }
    }
    return false;
}

bool solveSudokuDFS(int M[81]) {
    int row, col;
    if (!findEmpty(M, row, col)) {
        return true;
    }
    for (int num = 1; num <= 9; num++) {
        if (isValid(M, row, col, num)) {
            M[row * 9 + col] = num;
            if (solveSudokuDFS(M)) return true;
            M[row * 9 + col] = 0;
        }
    }
    return false;
}

vector<vector<int>> expandNode(const vector<int> &board) {
    vector<vector<int>> result;
    int pos = -1;
    for (int i = 0; i < 81; i++) {
        if (board[i] == 0) {
            pos = i;
            break;
        }
    }
    // this means that we have found the result
    if (pos == -1) return result;
    int row = pos / 9, col = pos % 9;

    int Mtmp[81];
    for (int i = 0; i < 81; i++) {
        Mtmp[i] = board[i];
    }

    for (int num = 1; num <= 9; num++) {
        if (isValid(Mtmp, row, col, num)) {
            vector<int> newBoard = board;
            newBoard[pos] = num;
            result.push_back(newBoard);
        }
    }
    return result;
}

void distributeTasks(queue<vector<int>> &tasks, int p, double *end) {
    int activeWorkers = p;
    while (activeWorkers > 0) {
        MPI_Status status;
        for (int w = 1; w <= p; w++) {
            if (tasks.empty()) {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_NO_MORE_TASK, MPI_COMM_WORLD);
                activeWorkers--;
            } else {
                vector<int> task = tasks.front(); tasks.pop();
                MPI_Send(&task[0], 81, MPI_INT, w, TAG_SEND_TASK, MPI_COMM_WORLD);
            }
        }

        while (activeWorkers > 0) {
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int source = status.MPI_SOURCE;
            int tag = status.MPI_TAG;

            if (tag == TAG_SOLUTION_FOUND) {
                *end = MPI_Wtime();
                vector<int> solutionBoard(81);
                MPI_Recv(&solutionBoard[0], 81, MPI_INT, source, TAG_SOLUTION_FOUND, MPI_COMM_WORLD, &status);
                // 记录结果，发通知终止
                for (int w = 1; w <= p; w++) {
                    if (w != source) {
                        MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
                    }
                }
                // 输出结果
                for (int i = 0; i < 81; i++) {
                    cout << solutionBoard[i];
                }
                cout << ' ';
                return;
            } else if (tag == TAG_SOLUTION_FAIL) {
                MPI_Recv(NULL, 1, MPI_INT, source, TAG_SOLUTION_FAIL, MPI_COMM_WORLD, &status);
                // 继续分配新的任务
                if (!tasks.empty()) {
                    vector<int> task = tasks.front(); tasks.pop();
                    MPI_Send(&task[0], 81, MPI_INT, source, TAG_SEND_TASK, MPI_COMM_WORLD);
                }
            }
        }
    }
}

void clearMessageQueue() {
    int flag = 1;
    MPI_Status status;
    while (flag) {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            int dummy;
            MPI_Recv(&dummy, 0, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    double end = 0;
    double start = 0;

    int p = size - 1;
    // ensure p>0
    if (p <= 0) {
        cout << "At least 1 worker is required." << endl;
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        // Master
        string puzzle;
        cin >> puzzle;

        vector<int> initBoard(81);
        for (int i = 0; i < 81; i++) {
            initBoard[i] = puzzle[i] - '0';
        }
        start = MPI_Wtime();
        queue<vector<int>> tasks;
        tasks.push(initBoard);

        distributeTasks(tasks, p, &end); // 动态任务分配
        cout << (end - start) * 1000 << endl;
    } else {
        // Worker
        bool running = true;
        while (running) {
            MPI_Status status;
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int tag = status.MPI_TAG;

            if (tag == TAG_SEND_TASK) {
                vector<int> task(81, 0);
                MPI_Recv(&task[0], 81, MPI_INT, 0, TAG_SEND_TASK, MPI_COMM_WORLD, &status);

                int M_task[81];
                for (int i = 0; i < 81; i++) {
                    M_task[i] = task[i];
                }

                if (solveSudokuDFS(M_task)) {
                    vector<int> sol(81);
                    for (int i = 0; i < 81; i++) {
                        sol[i] = M_task[i];
                    }
                    MPI_Send(&sol[0], 81, MPI_INT, 0, TAG_SOLUTION_FOUND, MPI_COMM_WORLD);
                    running = false;
                } else {
                    int dummy = 0;
                    MPI_Send(&dummy, 1, MPI_INT, 0, TAG_SOLUTION_FAIL, MPI_COMM_WORLD);
                }

            } else if (tag == TAG_NO_MORE_TASK) {
                MPI_Recv(NULL, 0, MPI_INT, 0, TAG_NO_MORE_TASK, MPI_COMM_WORLD, &status);
                running = false;
            } else if (tag == TAG_TERMINATE) {
                MPI_Recv(NULL, 0, MPI_INT, 0, TAG_TERMINATE, MPI_COMM_WORLD, &status);
                running = false;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    if (rank == 0) {
        end = MPI_Wtime();
        cout << "Final: ";
        cout << (end - start) * 1000 << 'ms'<< endl;
    }
    return 0;
}
