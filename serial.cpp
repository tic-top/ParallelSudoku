#include <iostream>
#include <chrono>
#include <string>
using namespace std;

static const int N = 9;

// 检查在grid[row][col]处放val是否合法
bool isValid(int grid[N][N], int row, int col, int val) {
    // 行检查
    for (int j = 0; j < N; j++) {
        if (grid[row][j] == val) return false;
    }
    // 列检查
    for (int i = 0; i < N; i++) {
        if (grid[i][col] == val) return false;
    }
    // 3x3宫检查
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[startRow + i][startCol + j] == val) return false;
        }
    }
    return true;
}

// 使用回溯法求解数独
bool solveSudoku(int grid[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (grid[i][j] == 0) {
                // 尝试1-9
                for (int val = 1; val <= 9; val++) {
                    if (isValid(grid, i, j, val)) {
                        grid[i][j] = val;
                        if (solveSudoku(grid)) {
                            return true;
                        }
                        grid[i][j] = 0; // 回溯
                    }
                }
                return false; // 如果1-9都不行，回溯上一层
            }
        }
    }
    return true; // 全部填满则返回true
}

int main() {
    // 读取81位输入字符串
    string puzzle;
    cin >> puzzle;
    // 将其转为9x9整型数组
    int grid[N][N];
    for (int i = 0; i < 81; i++) {
        char c = puzzle[i];
        if (c >= '0' && c <= '9') {
            grid[i/9][i%9] = c - '0';
        } else {
            // 如果不是数字，作为0处理
            grid[i/9][i%9] = 0;
        }
    }

    auto start = std::chrono::steady_clock::now();
    bool solved = solveSudoku(grid);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    if (!solved) {
        // 无解情况，这里根据需要处理，此处简单输出原题和时间
        // 实务中可以按需要改变行为
        cout << puzzle << " " << elapsed_seconds.count() * 1000 << "\n";
        return 0;
    }

    // 输出解答（81位数字）和耗时
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            cout << grid[i][j];
        }
    }
    cout << " " << elapsed_seconds.count() * 1000 << "\n";

    return 0;
}
