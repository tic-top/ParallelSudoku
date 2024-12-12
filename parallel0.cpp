#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cstring>

#define N 9  // 数独的大小

// 数独棋盘
int M[N][N];

// 判断M[i][j]填入k是否合法
bool isvalid(int M[N][N], int i, int j, int k) {
    // 检查行
    for (int col = 0; col < N; col++) {
        if (M[i][col] == k) return false;
    }
    
    // 检查列
    for (int row = 0; row < N; row++) {
        if (M[row][j] == k) return false;
    }
    
    // 检查3x3小方块
    int box_row = (i / 3) * 3, box_col = (j / 3) * 3;
    for (int r = box_row; r < box_row + 3; r++) {
        for (int c = box_col; c < box_col + 3; c++) {
            if (M[r][c] == k) return false;
        }
    }
    
    return true;
}

// 广度优先搜索，找到至少p+1个可能的初始解
std::queue<std::vector<std::vector<int>>> bfs(int M[N][N], int p) {
    std::queue<std::vector<std::vector<int>>> queue;
    std::vector<std::vector<int>> initialState(N, std::vector<int>(N, 0));
    std::memcpy(initialState.data(), M, sizeof(int) * N * N);
    queue.push(initialState);
    
    // 这里简单模拟广度优先搜索，只是为了获得至少p+1个初始解
    // 你可以根据实际情况修改
    while (queue.size() < p + 1) {
        auto current = queue.front();
        queue.pop();
        
        // 找到下一个空格子
        bool found = false;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (current[i][j] == 0) {
                    // 尝试填数字1到9
                    for (int k = 1; k <= 9; k++) {
                        if (isvalid(current.data(), i, j, k)) {
                            auto newState = current;
                            newState[i][j] = k;
                            queue.push(newState);
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }
        }
    }

    return queue;
}

// 深度优先搜索（回溯法）填数
bool dfs(int M[N][N], int &solCount) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (M[i][j] == 0) {
                for (int k = 1; k <= 9; k++) {
                    if (isvalid(M, i, j, k)) {
                        M[i][j] = k;
                        if (dfs(M, solCount)) {
                            return true;
                        }
                        M[i][j] = 0;  // 回溯
                    }
                }
                return false;  // 无法填入有效的数字
            }
        }
    }
    // 找到一个解
    solCount++;
    return true;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // 这里是主进程，负责读取数独初始数据
        // 请替换为你实际的输入方式
        int initialPuzzle[N][N] = {
            {5, 3, 0, 0, 7, 0, 0, 0, 0},
            {6, 0, 0, 1, 9, 5, 0, 0, 0},
            {0, 9, 8, 0, 0, 0, 0, 6, 0},
            {8, 0, 0, 0, 6, 0, 0, 0, 3},
            {4, 0, 0, 8, 0, 3, 0, 0, 1},
            {7, 0, 0, 0, 2, 0, 0, 0, 6},
            {0, 6, 0, 0, 0, 0, 2, 8, 0},
            {0, 0, 0, 4, 1, 9, 0, 0, 5},
            {0, 0, 0, 0, 8, 0, 0, 7, 9}
        };

        std::memcpy(M, initialPuzzle, sizeof(initialPuzzle));
    }

    // 广度优先搜索，找到至少p+1个初始解
    int p = size - 1;  // 假设p = 核心数 - 1
    auto initialSolutions = bfs(M, p);

    // 分配解给不同的进程进行DFS
    while (!initialSolutions.empty()) {
        auto currentPuzzle = initialSolutions.front();
        initialSolutions.pop();

        // 每个进程执行深度优先搜索
        int localSolutionCount = 0;
        if (rank != 0) {
            // 进程进行回溯
            dfs(currentPuzzle.data(), localSolutionCount);
        }

        // 主进程收集结果
        int globalSolutionCount = 0;
        MPI_Reduce(&localSolutionCount, &globalSolutionCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            std::cout << "找到的解的数量: " << globalSolutionCount << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}
