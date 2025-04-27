#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <algorithm>
using namespace std;

class SearchNode
{
public:
    vector<vector<int>> current_configuration;
    int g;
    SearchNode *prev_node;
    SearchNode(vector<vector<int>> current_configuration, int g, SearchNode *prev_node)
    {
        this->current_configuration = current_configuration;
        this->g = g;
        this->prev_node = prev_node;
    }
};

// fucntion pointer for heruristic function
typedef float (*heuristicFunction)(vector<vector<int>> &grid);
heuristicFunction heuristic;
int expanded_nodes = 0;
int explored_nodes = 0;



float Hamming_Distance(vector<vector<int>> &grid)
{
    int k = grid[0].size();
    int count = 0;

    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < k; j++)
        {

            if (grid[i][j] == i * k + j + 1)
            {
            }
            else
            {
                count++;
            }
        }
    }

    return count - 1; // last index obviously not counter as it  is config for blank/0
}

float Manhattan_Distance(vector<vector<int>> &grid)
{
    int k = grid[0].size();
    int total_sum = 0;

    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < k; j++)
        {
            if (grid[i][j] == 0)
            {
                continue;
            }
            else
            {
                int x = grid[i][j] - 1;
                int x1 = x / k;

                int y1 = x % k;
                total_sum += abs(i - x1) + abs(j - y1);
            }
        }
    }
    return total_sum;
}

float Euclidean_Distance(vector<vector<int>> &grid)
{
    int k = grid[0].size();
    float total_sum = 0;
    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < k; j++)
        {
            if (grid[i][j] == 0)
            {
                continue;
            }
            else
            {
                int x = grid[i][j] - 1;
                int x1 = x / k;

                int y1 = x % k;
                total_sum += sqrt(pow(i - x1, 2) + pow(j - y1, 2));
            }
        }
    }
    return total_sum;
}

int count_Linear_conflict(vector<vector<int>> &grid)
{

    int k = grid.size();
    int conflict = 0;

    // row conflicts
    for (int row = 0; row < k; row++)
    {
        for (int j = 0; j < k; j++)
        {

            if (grid[row][j] != 0)
            {
                int x = grid[row][j] - 1;
                int x1 = x / k;

                int y1 = x % k;

                if (x1 == row)
                {
                    for (int j1 = j + 1; j1 < k; j1++)
                    {
                        if (grid[row][j1] != 0)
                        {

                            int x2 = (grid[row][j1] - 1) / k;
                            int y2 = (grid[row][j1] - 1) % k;

                            if (x2 == row && y2 < y1)
                            {
                                conflict++;
                            }
                        }
                    }
                }
            }
        }
    }

    // column conflicts
    for (int col = 0; col < k; col++)
    {
        for (int i = 0; i < k; i++)
        {

            if (grid[i][col] != 0)
            {
                int x = grid[i][col] - 1;
                int x1 = x / k;

                int y1 = x % k;
                if (y1 == col)
                {
                    for (int i1 = i + 1; i1 < k; i1++)
                    {
                        if (grid[i1][col] != 0)
                        {

                            int x2 = (grid[i1][col] - 1) / k;
                            int y2 = (grid[i1][col] - 1) % k;

                            if (x2 < x1 && y2 == col)
                            {
                                conflict++;
                            }
                        }
                    }
                }
            }
        }
    }
    return conflict;
}
float Linear_conflict(vector<vector<int>> &grid)
{

    return (2 * count_Linear_conflict(grid) + Manhattan_Distance(grid)); // default spec heuristic function
}

struct Compare
{
    bool operator()(SearchNode *a, SearchNode *b)
    {
        return (a->g + heuristic(a->current_configuration)) > (b->g + heuristic(b->current_configuration));
    }
};

int Count_Inversions(vector<vector<int>> &grid)
{
    int n = grid.size();
    vector<int> flat;

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (grid[i][j] != 0)
            {
                flat.push_back(grid[i][j]);
            }
        }
    }

    int inversions = 0;
    int size = flat.size();

    // Count inversions
    for (int i = 0; i < size; i++)
    {
        for (int j = i + 1; j < size; j++)
        {
            if (flat[i] > flat[j])
            {
                inversions++;
            }
        }
    }

    return inversions;
}

bool isSolvable(vector<vector<int>> &grid)
{
    int inversions = Count_Inversions(grid);
    int k = grid[0].size();
    int blank_position = 0;

    if (k % 2 == 0)
    {

        for (int i = 0; i < k; i++)
        {
            for (int j = 0; j < k; j++)
            {
                if (grid[i][j] == 0)
                {
                    blank_position = i;
                    break;
                }
            }
        }
        int blank_row_from_bottom = k - blank_position; // row from bottom
        if (blank_row_from_bottom % 2 == 0)
        {
            return inversions % 2 != 0; // even blank row from bottom
        }
        else
        {
            return inversions % 2 == 0; // odd blank row from bottom
        }
    }
    else
    {
        return inversions % 2 == 0;
    }
}

// Function to print the grid
void printGrid(vector<vector<int>> &grid)
{
    for (int i = 0; i < grid.size(); i++)
    {
        for (int j = 0; j < grid.size(); j++)
        {
            cout << grid[i][j] << " ";
        }
        cout << endl;
    }
}

string grid_to_string(vector<vector<int>> &grid)
{
    string res;
    for (auto &row : grid)
    {
        for (int num : row)
        {
            res += to_string(num) + ',';
        }
    }
    return res;
}

// Function to check if a string is in the closed list
bool is_in_closed_list(const string &str, const vector<string> &closed_list)
{
    for (const auto &s : closed_list)
    {
        if (s == str)
        {
            return true;
        }
    }
    return false;
}

// get neighbours of the current node
vector<vector<vector<int>>> get_neighbours(vector<vector<int>> &grid)
{
    vector<vector<vector<int>>> neighbours;
    int k = grid.size();
    int blank_row, blank_col;

    // Find the position of the blank tile (0)
    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < k; j++)
        {
            if (grid[i][j] == 0)
            {
                blank_row = i;
                blank_col = j;
                break;
            }
        }
    }

    // Possible moves: up, down, left, right
    vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto &dir : directions)
    {
        int new_row = blank_row + dir.first;
        int new_col = blank_col + dir.second;

        if (new_row >= 0 && new_row < k && new_col >= 0 && new_col < k)
        {
            vector<vector<int>> new_grid = grid;
            swap(new_grid[blank_row][blank_col], new_grid[new_row][new_col]);
            neighbours.push_back(new_grid);
        }
    }

    return neighbours;
}

bool isGoalState(vector<vector<int>> &grid)
{
    int k = grid[0].size();

    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < k; j++)
        {
            if (i == k - 1 && j == k - 1)
            {
                if (grid[i][j] != 0)
                    return false; // last cell must be blank (0)
            }
            else
            {
                if (grid[i][j] != i * k + j + 1)
                    return false;
            }
        }
    }

    return true;
}

void printSequentially(SearchNode *node)
{
    vector<SearchNode *> path;
    while (node != nullptr)
    {
        path.push_back(node);
        node = node->prev_node;
    }

    cout << "Minimum number of moves: " << path[0]->g << endl;
    cout << endl; 

    for (int i = path.size() - 1; i >= 0; i--)
    {
        // cout << "Step " << i << ":" << endl;
        printGrid(path[i]->current_configuration);
        cout << endl;
    }
}

void AstarSearch(vector<vector<int>> &grid)
{

    if (!isSolvable(grid))
    {
        cout << "Unsolvable Puzzle" << endl;
        return;
    }

    priority_queue<SearchNode *, vector<SearchNode *>, Compare> open_list;
    vector<string> closed_list;

    SearchNode *start_node = new SearchNode(grid, 0, nullptr);
    open_list.push(start_node);
    explored_nodes++;
    closed_list.push_back(grid_to_string(grid));
    int k = grid[0].size();

    while (!open_list.empty())
    {
        SearchNode *current_node = open_list.top();
        open_list.pop();
        expanded_nodes++;

        if (isGoalState(current_node->current_configuration))
        {

            printSequentially(current_node);
            return;
        }
        vector<vector<vector<int>>> neighbours = get_neighbours(current_node->current_configuration);

        for (auto neighbor : neighbours)
        {
            string neighbour_str = grid_to_string(neighbor);

            if (is_in_closed_list(neighbour_str, closed_list))
            {
                continue;
            }
            SearchNode *new_node = new SearchNode(neighbor, current_node->g + 1, current_node);
            open_list.push(new_node);
            explored_nodes++;
            closed_list.push_back(neighbour_str);
        }
    }
}

int main(int argc, char *argv[])
{

    string heuristic_choice = "linearConflict";
    // cout << argv[1] << endl;

    if (argc > 1)
    {
        heuristic_choice = argv[1];
    }
    if (heuristic_choice == "hamming")
    {
        heuristic = Hamming_Distance;
    }
    else if (heuristic_choice == "manhattan")
    {
        heuristic = Manhattan_Distance;
    }
  
    else if (heuristic_choice == "linearConflict")
    {
        heuristic = Linear_conflict;
    }
    else if (heuristic_choice == "euclidean")
    {
        heuristic = Euclidean_Distance;
        
    }
    else
    {
        cout << "Invalid heuristic choice. Defaulting to Linear Conflict." << endl;
        heuristic = Linear_conflict;
    }

    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    int grid_size;
    cin >> grid_size;
    vector<vector<int>> input_grid(grid_size, vector<int>(grid_size));

    for (int i = 0; i < grid_size; i++)
    {
        for (int j = 0; j < grid_size; j++)
        {
            cin >> input_grid[i][j];
        }
    }

    // cout << "Hamming distance " << Hamming_Distance(input_grid) << endl;
    // cout << "Manhattan Distance " << Manhattan_Distance(input_grid) << endl;
    // cout << "Euclidean Distance " << Euclidean_Distance(input_grid) << endl;
    // cout << "Count Inversions " << Count_Inversions(input_grid) << endl;
    // cout << "Linear Conflict " << count_Linear_conflict(input_grid) << endl;
    // cout << "Is Solvable  " << isSolvable(input_grid) << endl;

    AstarSearch(input_grid);
    cout << "Explored Nodes: " << explored_nodes << endl;
    cout << "Expanded Nodes: " << expanded_nodes << endl;

    return 0;

    fclose(stdin);
    fclose(stdout);
}