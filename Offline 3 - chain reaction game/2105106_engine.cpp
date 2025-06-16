#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <climits>
#include <queue>
#include <utility>

using namespace std;

const int ROWS = 9;
const int COLS = 6;

vector<vector<string>> board(ROWS, vector<string>(COLS, "0"));
char AI_PLAYER = 'B';
char HUMAN_PLAYER = 'R';

char getOpponent(char p) {
    return (p == 'R') ? 'B' : 'R';
}

// --- Read human move from file ---
bool read_human_move(const string& filename) {
    ifstream fin(filename);
    if (!fin) return false;

    string header;
    getline(fin, header);
    if (header != "Human Move:") return false;

    for (int i = 0; i < ROWS; ++i) {
        string line;
        getline(fin, line);
        istringstream ss(line);
        for (int j = 0; j < COLS; ++j) {
            ss >> board[i][j];
        }
    }

    cout << "Human move received.\n";
    return true;
}

// --- Critical mass based on position ---
int critical_mass(int i, int j) {
    if ((i == 0 || i == ROWS - 1) && (j == 0 || j == COLS - 1)) return 2;
    if (i == 0 || i == ROWS - 1 || j == 0 || j == COLS - 1) return 3;
    return 4;
}

// --- Get legal actions for a player ---
vector<pair<int, int>> getLegalActions(char player) {
    vector<pair<int, int>> moves;
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            string val = board[i][j];
            if (val == "0" || val.back() == player) {
                moves.push_back({i, j});
            }
        }
    }
    return moves;
}

void printState(const vector<vector<string>>& state) {
    for (const auto& row : state) {
        for (const auto& cell : row) {
            cout << cell << " ";
        }
        cout << endl;
    }
    cout << endl;
}
vector<vector<string>> applyAction(const vector<vector<string>>& state, pair<int, int> move, char player) {
    vector<vector<string>> newState = state;
    int x = move.first;
    int y = move.second;

    // Add orb to the chosen cell
    if (newState[x][y] == "0") {
        newState[x][y] = "1" + string(1, player);
    } else {
        string val = newState[x][y];
        string numberPart = val.substr(0, val.size() - 1);
        int count = stoi(numberPart);
        newState[x][y] = to_string(count + 1) + player;
    }

    // Handle explosions
    queue<pair<int, int>> q;
    q.push({x, y});

    while (!q.empty()) {
        int i = q.front().first;
        int j = q.front().second;
        q.pop();

        string val = newState[i][j];
        if (val == "0") continue;

        string numberPart = val.substr(0, val.size() - 1);
        char owner = val.back();
        int count = stoi(numberPart);


        if (count >= critical_mass(i, j)) {
            count -= critical_mass(i, j);
            if (count > 0)
                newState[i][j] = to_string(count) + owner;
            else
                newState[i][j] = "0";
        

            int dx[] = {-1, 1, 0, 0};
            int dy[] = {0, 0, -1, 1};

            for (int k = 0; k < 4; k++) {
                int ni = i + dx[k];
                int nj = j + dy[k];

                if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS) {
                    if (newState[ni][nj] == "0") {
                        newState[ni][nj] = "1" + string(1, owner);
                    } else {
                        string neighborVal = newState[ni][nj];
                        string neighborNum = neighborVal.substr(0, neighborVal.size() - 1);
                        int neighborCount = stoi(neighborNum);
                        newState[ni][nj] = to_string(neighborCount + 1) + owner;
                    }

                    // Check for next explosion
                    string updatedVal = newState[ni][nj];
                    int newCount = stoi(updatedVal.substr(0, updatedVal.size() - 1));
                    if (newCount >= critical_mass(ni, nj)) {
                        q.push({ni, nj});
                    }
                }
            }
        }
    }

    return newState;
}



char checkWinner(const vector<vector<string>>& board) {
    int red = 0, blue = 0;

    for (const auto& row : board) {
        for (const auto& cell : row) {
            if (!cell.empty() && cell.back() == 'R') {
                red++;
            } else if (!cell.empty() && cell.back() == 'B') {
                blue++;
            }
        }
    }

    int total = red + blue;
    if (total <= 1) {
        return 'N'; // No winner yet (too early)
    }
    if (red == 0) {
        return 'B'; // Blue wins
    }
    if (blue == 0) {
        return 'R'; // Red wins
    }
    return 'N'; 
}



// --- Heuristic function pointer ---
int (*heuristic)(const vector<vector<string>>&, char) = nullptr;


//



// --- Heuristic function: bonus for critical cells ---
int heuristic_critical_cells(const vector<vector<string>>& state, char player) {
    int score = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            string cell = state[r][c];
            if (cell == "0" || cell.back() != player) continue;

            int orbs = stoi(cell.substr(0, cell.size() - 1));
            int critical = critical_mass(r, c);

            if (orbs == critical - 1)
                score += 5;
            else if (orbs == critical - 2)
                score += 2;
        }
    }
    
    return score;
}

int heuristic_orb_count(const vector<vector<string>>& state, char player) {
    int score = 0;
    for (auto& row : state) {
        for (auto& cell : row) {
            if (cell != "0" && cell.back() == player) {
                score += stoi(cell.substr(0, cell.size() - 1));
            }
        }
    }
    return score;
}

int heuristic_controlled_cells(const vector<vector<string>>& state, char player) {
    int score = 0;
    for (auto& row : state) {
        for (auto& cell : row) {
            if (cell != "0" && cell.back() == player) {
                score++;
            }
        }
    }
    return score;
}


int heuristic_vulnerable_cells(const vector<vector<string>>& state, char player) {
    int penalty = 0;
    char opponent = getOpponent(player);

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            string cell = state[i][j];
            if (cell == "0" || cell.back() != player) continue;

            int orbs = stoi(cell.substr(0, cell.size() - 1));
            if (orbs >= critical_mass(i, j) - 1) {
                // Check neighbors for opponent presence
                vector<pair<int, int>> dirs = {{-1,0},{1,0},{0,-1},{0,1}};
                for (auto [dx, dy] : dirs) {
                    int ni = i + dx, nj = j + dy;
                    if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS) {
                        string neighbor = state[ni][nj];
                        if (neighbor != "0" && neighbor.back() == opponent) {
                            penalty += 4;
                            break;
                        }
                    }
                }
            }
        }
    }

    return -penalty;  // negative score
}



int heuristic_corner_bonus(const vector<vector<string>>& state, char player) {
    int score = 0;
    vector<pair<int, int>> safeCells = {{0,0},{0,COLS-1},{ROWS-1,0},{ROWS-1,COLS-1}};
    for (auto [i, j] : safeCells) {
        string cell = state[i][j];
        if (cell != "0" && cell.back() == player) {
            score += 3;
        }
    }
    return score;
}






// --- Minimax with alpha-beta pruning ---
pair<int, pair<int, int>> minimax(vector<vector<string>> state, int depth, int alpha, int beta, bool maximizing, char player) {
    if (depth == 0 || checkWinner(state) != 'N') {
        return {heuristic(state, AI_PLAYER), {-1, -1}};
    }

    char currentPlayer = maximizing ? player : getOpponent(player);
    vector<pair<int, int>> moves = getLegalActions(currentPlayer);
    pair<int, int> bestMove = {-1, -1};

    if (maximizing) {
        int maxEval = INT_MIN;
        for (auto move : moves) {
            auto newState = applyAction(state, move, currentPlayer);
            int eval = minimax(newState, depth - 1, alpha, beta, false, getOpponent(currentPlayer)).first;
            if (eval > maxEval) {
                // cout << " maximizing: " << endl;
                maxEval = eval;
                bestMove = move;
            }
            alpha = max(alpha, eval);
            if (beta <= alpha) break;
        }
        return {maxEval, bestMove};
    } else {
        int minEval = INT_MAX;
        for (auto move : moves) {
            auto newState = applyAction(state, move, currentPlayer);
            int eval = minimax(newState, depth - 1, alpha, beta, true, getOpponent(currentPlayer)).first;
            if (eval < minEval) {
                // cout << " minimizing: " << endl;
                minEval = eval;
                bestMove = move;
            }
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }
        return {minEval, bestMove};
    }
}


// prev apply action function
vector<vector<string>> PrevApplyAction(const vector<vector<string>>& state, pair<int, int> move, char player) {
    vector<vector<string>> newState = state;
    int i = move.first, j = move.second;
    string val = newState[i][j];

    if (val == "0") {
        newState[i][j] = "1" + string(1, player);
    } else {
        int count = stoi(val.substr(0, val.size()-1)) + 1;
        newState[i][j] = to_string(count) + player;
    }

    return newState;
}


// --- Write AI move to file ---
void write_ai_move(const string& filename) {
    int depth_limit = 3;
    auto [score, move] = minimax(board, depth_limit, INT_MIN, INT_MAX, true, AI_PLAYER);
    cout << "AI Move: " << move.first << ", " << move.second << " with score: " << score << endl;

    if (move.first != -1) {
        board = PrevApplyAction(board, move, AI_PLAYER);
    }

    ofstream fout(filename);
    fout << "AI Move:\n";
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            fout << board[i][j];
            if (j < COLS - 1) fout << " ";
        }
        fout << "\n";
    }

    cout << "AI move written                        .\n";
}

// --- Main loop ---
int main() {
    const string filename = "gamestate.txt";
    cout << "C++ AI Engine started. Waiting for Human Move...\n";
    heuristic = heuristic_critical_cells; // Select the heuristic to use

    while (true) {
        if (read_human_move(filename)) {
            this_thread::sleep_for(chrono::seconds(1));
            write_ai_move(filename);
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    return 0;
}
