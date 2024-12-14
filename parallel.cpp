#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

static const int N = 9;

// Enumeration for message tags
enum MessageTag
{
    TAG_SEND_TASK,
    TAG_NO_MORE_TASK,
    TAG_SOLUTION_FOUND,
    TAG_SOLUTION_FAIL,
    TAG_TERMINATE,
    TAG_CANCEL_TASK
};

// Function to check if placing val at grid[row][col] is valid
bool isValid(const int M[81], int row, int col, int val)
{
    // Check row
    for (int j = 0; j < N; j++)
    {
        if (M[row * 9 + j] == val)
            return false;
    }
    // Check column
    for (int i = 0; i < N; i++)
    {
        if (M[i * 9 + col] == val)
            return false;
    }
    // Check 3x3 subgrid
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = startRow; i < startRow + 3; i++)
    {
        for (int j = startCol; j < startCol + 3; j++)
        {
            if (M[i * 9 + j] == val)
                return false;
        }
    }
    return true;
}

// Function to find the next empty cell
bool findEmpty(const int M[81], int &row, int &col)
{
    for (int i = 0; i < 81; i++)
    {
        if (M[i] == 0)
        {
            row = i / 9;
            col = i % 9;
            return true;
        }
    }
    return false;
}

// Depth-First Search Sudoku solver
bool solveSudokuDFS(int M[81])
{
    int row, col;
    if (!findEmpty(M, row, col))
    {
        return true; // Puzzle solved
    }
    for (int num = 1; num <= 9; num++)
    {
        if (isValid(M, row, col, num))
        {
            M[row * 9 + col] = num;
            if (solveSudokuDFS(M))
                return true;
            M[row * 9 + col] = 0; // Backtrack
        }
    }
    return false; // Trigger backtracking
}

int expandtasks(vector<int> &task, queue<vector<int>> &tasks)
{
    int row, col;
    // change task to int array tas
    int tas[81];
    for (int i = 0; i < 81; i++)
    {
        tas[i] = task[i];
    }
    if (!findEmpty(tas, row, col))
    {
        return 0; // 0 solved
    }
    int expanded = 0;
    for (int num = 1; num <= 9; num++)
    {
        if (isValid(tas, row, col, num))
        {
            vector<int> newTask = task;
            newTask[row * 9 + col] = num;
            tasks.push(newTask);
            expanded++;
        }
    }
    if (expanded > 0)
        return 1; // expanded
    else
        return 2; // no solution
}

// Function to distribute tasks to workers
void distributeTasks(queue<vector<int>> &tasks, int p, double taskStartTime, ostream &outputFile, bool &solved)
{
    int activeWorkers = p;
    solved = false;
    while ((int)tasks.size() < p)
    {
        vector<int> task = tasks.front();
        tasks.pop();
        int flag = expandtasks(task, tasks);
        if (flag == 0)
        {
            // we have get the solution, task is the solution
            double end = MPI_Wtime();
            for (int i = 0; i < 81; i++)
            {
                outputFile << task[i];
            }
            outputFile << ',' << fixed << setprecision(3) << (end - taskStartTime) * 1000 << endl;
            return;
        }
    }

    // 第一批的题目发放
    MPI_Status status;
    int workingservents = 0;
    for (int w = 1; w <= p; w++)
    {
        if (!tasks.empty())
        {
            vector<int> task = tasks.front();
            tasks.pop();
            MPI_Send(&task[0], 81, MPI_INT, w, TAG_SEND_TASK, MPI_COMM_WORLD);
            workingservents++;
        }
    }

    // 确保有解
    while (workingservents>0)
    {
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int source = status.MPI_SOURCE;
        int tag = status.MPI_TAG;

        if (tag == TAG_SOLUTION_FOUND)
        {
            double end = MPI_Wtime();
            vector<int> solutionBoard(81);
            MPI_Recv(&solutionBoard[0], 81, MPI_INT, source, TAG_SOLUTION_FOUND, MPI_COMM_WORLD, &status);
            // Output the solution
            for (int i = 0; i < 81; i++)
            {
                outputFile << solutionBoard[i];
            }
            outputFile << ',' << fixed << setprecision(3) << (end - taskStartTime) * 1000 << endl;
            solved=true;
            // clear the tasks
            tasks = queue<vector<int>>();
        }
        else if (tag == TAG_SOLUTION_FAIL)
        {
            MPI_Recv(NULL, 1, MPI_INT, source, TAG_SOLUTION_FAIL, MPI_COMM_WORLD, &status);
            // Continue assigning tasks if available
            if (!tasks.empty())
            {
                vector<int> task = tasks.front();
                tasks.pop();
                MPI_Send(&task[0], 81, MPI_INT, source, TAG_SEND_TASK, MPI_COMM_WORLD);
            }else{
                workingservents--;
            }
        }
    }
}

// Function to clear any remaining messages in the queue
void clearMessageQueue()
{
    int flag = 1;
    MPI_Status status;
    while (flag)
    {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag)
        {
            int dummy;
            MPI_Recv(&dummy, 0, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        }
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int p = size - 1; // Number of worker processes

    // Ensure at least one worker is available
    if (p <= 0)
    {
        if (rank == 0)
        {
            cout << "At least 1 worker is required." << endl;
        }
        MPI_Finalize();
        return 0;
    }

    if (rank == 0)
    {
        // Master process
        if (argc < 3)
        {
            cerr << "Usage: " << argv[0] << " <input_csv_file> <output_csv_file>" << endl;
            // Notify workers to terminate
            for (int w = 1; w <= p; w++)
            {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
            }
            MPI_Finalize();
            return 1;
        }

        string inputFilePath = argv[1];
        string outputFilePath = argv[2];

        ifstream inputFile(inputFilePath);
        if (!inputFile.is_open())
        {
            cerr << "Failed to open input file: " << inputFilePath << endl;
            // Notify workers to terminate
            for (int w = 1; w <= p; w++)
            {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
            }
            MPI_Finalize();
            return 1;
        }

        ofstream outputFile(outputFilePath);
        if (!outputFile.is_open())
        {
            cerr << "Failed to open output file: " << outputFilePath << endl;
            inputFile.close();
            // Notify workers to terminate
            for (int w = 1; w <= p; w++)
            {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
            }
            MPI_Finalize();
            return 1;
        }

        string headerLine;
        if (!getline(inputFile, headerLine))
        {
            cerr << "Input file is empty." << endl;
            inputFile.close();
            outputFile.close();
            // Notify workers to terminate
            for (int w = 1; w <= p; w++)
            {
                MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
            }
            MPI_Finalize();
            return 1;
        }

        outputFile << "puzzle,solution,clues,difficulty,difficulty_range,result,time\n";

        string line;
        // Process each puzzle in the input CSV
        while (getline(inputFile, line))
        {
            // Trim whitespace from both ends
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");
            if (start == string::npos)
            {
                // Line contains only whitespace
                continue;
            }
            line = line.substr(start, end - start + 1);

            // Parse CSV fields
            stringstream ss(line);
            string field;
            vector<string> fields;

            while (getline(ss, field, ','))
            {
                fields.push_back(field);
            }

            // Ensure there are at least 5 fields
            if (fields.size() < 5)
            {
                cerr << "Invalid input format. Expected at least 5 fields, got " << fields.size() << "." << endl;
                // Write to output with error
                outputFile << line << ",Invalid Format,0.000\n";
                continue;
            }

            string puzzle = fields[0];
            string solution = fields[1];
            string clues = fields[2];
            string difficulty = fields[3];
            string difficulty_range = fields[4];
            // 把这几个先输出
            outputFile << puzzle << "," << solution << "," << clues << "," << difficulty << "," << difficulty_range << ",";

            // Validate puzzle length
            if (puzzle.length() != 81)
            {
                cerr << "Invalid puzzle length. Expected 81 characters, got " << puzzle.length() << "." << endl;
                // Write to output with error
                outputFile << line << ",Invalid Puzzle Length,0.000\n";
                continue;
            }

            // Initialize the grid
            vector<int> initBoard(81, 0);
            bool invalidChar = false;
            for (int i = 0; i < 81; i++)
            {
                char c = puzzle[i];
                if (c >= '1' && c <= '9')
                {
                    initBoard[i] = c - '0';
                }
                else if (c == '0' || c == '.')
                { // Allow '0' or '.' for empty cells
                    initBoard[i] = 0;
                }
                else
                {
                    // Invalid character encountered
                    cerr << "Invalid character '" << c << "' in puzzle at position " << i + 1 << "." << endl;
                    initBoard[i] = 0; // Treat as empty
                    invalidChar = true;
                }
            }

            // If there were invalid characters, mark as invalid
            if (invalidChar)
            {
                outputFile << line << ",Invalid Characters,0.000\n";
                continue;
            }

            // Create a task queue with the initial puzzle
            queue<vector<int>> tasks;
            tasks.push(initBoard);

            // Record the start time
            double taskStartTime = MPI_Wtime();

            // Distribute tasks and solve
            bool solved = false;
            distributeTasks(tasks, p, taskStartTime, outputFile, solved);

            if (!solved)
            {
                // If no solution was found
                outputFile << line << ",No Solution," << fixed << setprecision(3) << (MPI_Wtime() - taskStartTime) * 1000 << "\n";
            }
            // Clear any remaining messages
            clearMessageQueue();
        }

        // After processing all puzzles, notify workers to terminate
        for (int w = 1; w <= p; w++)
        {
            MPI_Send(NULL, 0, MPI_INT, w, TAG_TERMINATE, MPI_COMM_WORLD);
        }

        inputFile.close();
        outputFile.close();
    }
    else
    {
        // Worker process
        bool running = true;
        while (running)
        {
            MPI_Status status;
            // Probe for incoming messages
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int tag = status.MPI_TAG;

            if (tag == TAG_SEND_TASK)
            {
                // Receive the task
                int taskData[81];
                MPI_Recv(&taskData, 81, MPI_INT, 0, TAG_SEND_TASK, MPI_COMM_WORLD, &status);
                // Solve the puzzle
                bool solved = solveSudokuDFS(taskData);
                if (solved)
                {
                    // Send the solution back
                    vector<int> solutionBoard(81, 0);
                    for (int i = 0; i < 81; i++)
                    {
                        solutionBoard[i] = taskData[i];
                    }
                    MPI_Send(&solutionBoard[0], 81, MPI_INT, 0, TAG_SOLUTION_FOUND, MPI_COMM_WORLD);
                }
                else
                {
                    // Indicate failure
                    int dummy = 0;
                    MPI_Send(&dummy, 1, MPI_INT, 0, TAG_SOLUTION_FAIL, MPI_COMM_WORLD);
                }
            }
            else if (tag == TAG_TERMINATE)
            {
                // Receive the termination signal
                // this is trigger when all tasks finished
                MPI_Recv(NULL, 0, MPI_INT, 0, TAG_TERMINATE, MPI_COMM_WORLD, &status);
                running = false;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
