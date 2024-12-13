#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip> // For setting precision
#include <cstdlib> // For atoi
#include <cmath> // For pow
#include <algorithm> // For min, max

using namespace std;

static const int N = 9;

// Function to check if placing val at grid[row][col] is valid
bool isValid(int grid[N][N], int row, int col, int val) {
    // Row check
    for (int j = 0; j < N; j++) {
        if (grid[row][j] == val) return false;
    }
    // Column check
    for (int i = 0; i < N; i++) {
        if (grid[i][col] == val) return false;
    }
    // 3x3 subgrid check
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[startRow + i][startCol + j] == val) return false;
        }
    }
    return true;
}

// Backtracking Sudoku solver
bool solveSudoku(int grid[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (grid[i][j] == 0) { // Find an empty cell
                for (int val = 1; val <= 9; val++) { // Try values 1-9
                    if (isValid(grid, i, j, val)) {
                        grid[i][j] = val; // Tentatively assign
                        if (solveSudoku(grid)) { // Recurse
                            return true;
                        }
                        grid[i][j] = 0; // Backtrack
                    }
                }
                return false; // Trigger backtracking
            }
        }
    }
    return true; // Solved
}

int main() {
    // Expected CSV input: puzzle,solution,clues,difficulty,difficulty_range
    // Output: puzzle,solution,clues,difficulty,difficulty_range,result,time
    // 'result' is 'Solved' or 'No Solution', 'time' is in milliseconds

    string line;
    bool isFirstLine = true;

    // Read input line by line
    while (getline(cin, line)) {
        // Trim whitespace from both ends
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == string::npos) {
            // Line contains only whitespace
            continue;
        }
        line = line.substr(start, end - start + 1);

        // Handle 'exit' command
        if (line == "exit") {
            break;
        }

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Parse CSV fields
        stringstream ss(line);
        string field;
        vector<string> fields;

        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // If it's the first line (header), process accordingly
        if (isFirstLine) {
            isFirstLine = false;
            // Optionally, you can validate the header here
            // For simplicity, we'll include the new fields in the output header
            cout << "puzzle,solution,clues,difficulty,difficulty_range,result,time\n";
            continue;
        }

        // Ensure there are at least 5 fields as per the header
        if (fields.size() < 5) {
            cerr << "Invalid input format. Expected at least 5 fields, got " << fields.size() << ".\n";
            continue;
        }

        string puzzle = fields[0];
        string solution = fields[1];
        string clues = fields[2];
        string difficulty = fields[3];
        string difficulty_range = fields[4];

        // Validate puzzle length
        if (puzzle.length() != 81) {
            cerr << "Invalid puzzle length. Expected 81 characters, got " << puzzle.length() << ".\n";
            // Output with 'No Solution' and zero time
            cout << puzzle << "," << solution << "," << clues << "," 
                 << difficulty << "," << difficulty_range 
                 << ",Invalid Puzzle Length,0\n";
            continue;
        }

        // Initialize the grid to zero
        int grid[N][N] = {0};

        // Populate the grid
        bool invalidChar = false;
        for (int i = 0; i < 81; i++) {
            char c = puzzle[i];
            if (c >= '1' && c <= '9') {
                grid[i / 9][i % 9] = c - '0';
            } else if (c == '0' || c == '.') { // Allow '0' or '.' for empty cells
                grid[i / 9][i % 9] = 0;
            } else {
                // Invalid character encountered
                cerr << "Invalid character '" << c << "' in puzzle at position " << i+1 << ".\n";
                grid[i / 9][i % 9] = 0; // Treat as empty
                invalidChar = true;
            }
        }

        // Measure solving time
        auto start_time = std::chrono::steady_clock::now();
        bool solved = solveSudoku(grid);
        auto end_time = std::chrono::steady_clock::now();
        chrono::duration<double> elapsed_seconds = end_time - start_time;
        double elapsed_ms = elapsed_seconds.count() * 1000.0;

        // Prepare result string
        string result;
        if (!solved) {
            result = "No Solution";
        } else {
            result = "Solved";
        }

        // Convert solved grid back to string if solved
        string solvedPuzzle = puzzle; // Default to original puzzle
        if (solved) {
            solvedPuzzle = "";
            solvedPuzzle.reserve(81);
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    solvedPuzzle += to_string(grid[i][j]);
                }
            }
        }

        // If there were invalid characters, mark as 'Invalid Puzzle'
        if (invalidChar) {
            result = "Invalid Puzzle";
        }

        // Output in CSV format
        // Fields: puzzle,solution,clues,difficulty,difficulty_range,result,time
        // Time is set to zero if puzzle is invalid
        if (result == "Invalid Puzzle") {
            cout << puzzle << "," << solution << "," << clues << "," 
                 << difficulty << "," << difficulty_range 
                 << "," << result << "," << "0\n";
        } else {
            cout << puzzle << ",";
            if (solved) {
                cout << solvedPuzzle;
            } else {
                cout << solution; // If not solved, retain the original solution field
            }
            cout << "," << clues << "," << difficulty << "," << difficulty_range 
                 << "," << result << "," 
                 << fixed << setprecision(3) << elapsed_ms << "\n";
        }
    }

    return 0;
}
