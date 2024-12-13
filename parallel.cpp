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

bool isValid(int M[N][N], int row, int col, int val) {
    for (int j = 0; j < N; j++) {
        if (M[row][j] == val) return false;
    }
    for (int i = 0; i < N; i++) {
        if (M[i][col] == val) return false;
    }
    int startRow = (row / 3)*3;
    int startCol = (col / 3)*3;
    for (int i = startRow; i < startRow + 3; i++) {
        for (int j = startCol; j < startCol + 3; j++) {
            if (M[i][j] == val) return false;
        }
    }
    return true;
}

bool findEmpty(int M[N][N], int &row, int &col) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (M[i][j] == 0) {
                row = i; col = j;
                return true;
            }
        }
    }
    return false;
}

bool solveSudokuDFS(int M[N][N]) {
    int row, col;
    if (!findEmpty(M, row, col)) {
        return true;
    }
    for (int num = 1; num <= 9; num++) {
        if (isValid(M, row, col, num)) {
            M[row][col] = num;
            if (solveSudokuDFS(M)) return true;
            M[row][col] = 0;
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
    if (pos == -1) return result;
    int row = pos / 9, col = pos % 9;
    int Mtmp[9][9];
    for (int i = 0; i < 81; i++) {
        Mtmp[i/9][i%9] = board[i];
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

void ensureEnoughTasks(queue<vector<int>> &tasks, int p) {
    while ((int)tasks.size() < p+1) {
        if (tasks.empty()) return;
        vector<int> front = tasks.front();
        tasks.pop();
        auto newNodes = expandNode(front);
        for (auto &node : newNodes) {
            tasks.push(node);
        }
    }
}

void intArrayToBoard(const int arr[N][N], vector<int> &board) {
    board.resize(81);
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i*9 + j] = arr[i][j];
        }
    }
}

void boardToIntArray(const vector<int> &board, int arr[N][N]) {
    for (int i = 0; i < 81; i++) {
        arr[i/9][i%9] = board[i];
    }
}

// 清空消息队列中未消费的消息
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
    bool hasprinted = false;
    double end = 0;

    int p = size - 1;
    static int M_global[N][N];
    if (rank == 0) {
        // Master
        string puzzle;
        cin >> puzzle;

        vector<int> initBoard(81);
        for (int i = 0; i < 81; i++) {
            initBoard[i] = puzzle[i] - '0';
        }
        double start = MPI_Wtime();
        queue<vector<int>> tasks;
        tasks.push(initBoard);
        ensureEnoughTasks(tasks, p);
        end = MPI_Wtime();
        cout << "Time BFS: " << (end - start)*1000 << "ms" << endl;

        int activeWorkers = p;
        bool solutionFound = false;
        vector<int> solutionBoard(81,0);
        // 分配初始任务
        for (int w = 1; w <= p; w++) {
            if (tasks.empty()) {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_NO_MORE_TASK, MPI_COMM_WORLD);
                activeWorkers--;
            } else {
                auto task = tasks.front(); tasks.pop();
                MPI_Send(&task[0], 81, MPI_INT, w, TAG_SEND_TASK, MPI_COMM_WORLD);
            }
        }
        // 主循环等待反馈
        while (activeWorkers > 0 && !solutionFound) {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int source = status.MPI_SOURCE;
            int tag = status.MPI_TAG;

            if (tag == TAG_SOLUTION_FOUND) {
                end = MPI_Wtime();
                cout << "Time DFS: " << (end - start)*1000 << "ms" << endl;
                MPI_Recv(&solutionBoard[0], 81, MPI_INT, source, TAG_SOLUTION_FOUND, MPI_COMM_WORLD, &status);
                solutionFound = true;
                // 通知其他worker终止
                for (int w = 1; w <= p; w++) {
                    if (w != source) {
                        MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
                    }
                }
            } else if (tag == TAG_SOLUTION_FAIL) {
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, source, TAG_SOLUTION_FAIL, MPI_COMM_WORLD, &status);
                ensureEnoughTasks(tasks, p);
                if (tasks.empty()) {
                    MPI_Send(NULL, 0, MPI_INT, source, TAG_NO_MORE_TASK, MPI_COMM_WORLD);
                    activeWorkers--;
                } else {
                    auto task = tasks.front(); tasks.pop();
                    MPI_Send(&task[0], 81, MPI_INT, source, TAG_SEND_TASK, MPI_COMM_WORLD);
                }
            } else {
                // 不期望的消息
                MPI_Recv(NULL, 0, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
                MPI_Send(NULL, 0, MPI_INT, source, TAG_TERMINATE, MPI_COMM_WORLD);
                activeWorkers--;
            }
        }

        // 只能打印一次，防止有多个解

        if (solutionFound && !hasprinted) {
            hasprinted = true;
            int SolM[9][9];
            boardToIntArray(solutionBoard, SolM);
            for (int i = 0; i < 9; i++) 
                for (int j = 0; j < 9; j++) 
                    cout << SolM[i][j];
            cout << ' ';
            cout << (end - start)*1000<<endl; 
        } else {
            cout << "No solution found or no more tasks." << endl;
        }

        // Master结束前清理剩余消息
        clearMessageQueue();

    } else {
        // Worker
        bool running = true;
        while (running) {
            MPI_Status status;
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int tag = status.MPI_TAG;

            if (tag == TAG_SEND_TASK) {
                vector<int> task(81,0);
                MPI_Recv(&task[0], 81, MPI_INT, 0, TAG_SEND_TASK, MPI_COMM_WORLD, &status);

                int M_task[9][9];
                for (int i = 0; i < 81; i++) {
                    M_task[i/9][i%9] = task[i];
                }

                if (solveSudokuDFS(M_task)) {
                    vector<int> sol(81);
                    for (int i = 0; i < 81; i++) {
                        sol[i] = M_task[i/9][i%9];
                    }
                    MPI_Send(&sol[0], 81, MPI_INT, 0, TAG_SOLUTION_FOUND, MPI_COMM_WORLD);
                    running = false;
                } else {
                    int dummy=0;
                    MPI_Send(&dummy, 1, MPI_INT, 0, TAG_SOLUTION_FAIL, MPI_COMM_WORLD);
                    // 等待下一个消息
                }

            } else if (tag == TAG_NO_MORE_TASK) {
                MPI_Recv(NULL, 0, MPI_INT, 0, TAG_NO_MORE_TASK, MPI_COMM_WORLD, &status);
                running = false;
            } else if (tag == TAG_TERMINATE) {
                MPI_Recv(NULL, 0, MPI_INT, 0, TAG_TERMINATE, MPI_COMM_WORLD, &status);
                running = false;
            } else {
                // Unexpected message, consume and exit
                MPI_Recv(NULL, 0, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
                running = false;
            }
        }
    }

    // 全局同步，确保所有消息处理完毕再结束
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
