#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <climits>

using namespace std;

const int ROWS = 9;
const int COLS = 6;

vector<vector<string>> board(ROWS, vector<string>(COLS, "0"));
char AI_PLAYER = 'B';
char HUMAN_PLAYER = 'R';

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

int critical_mass(int i, int j) {
    if ((i == 0 || i == ROWS - 1) && (j == 0 || j == COLS - 1)) return 2;
    if (i == 0 || i == ROWS - 1 || j == 0 || j == COLS - 1) return 3;
    return 4;
}

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

vector<vector<string>> applyAction(const vector<vector<string>>& state, pair<int, int> move, char player) {
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

int evaluate(const vector<vector<string>>& state, char player) {
    int score = 0;
    for (auto& row : state) {
        for (auto& cell : row) {
            if (cell != "0") {
                int count = stoi(cell.substr(0, cell.size() - 1));
                if (cell.back() == player) score += count;
                else score -= count;
            }
        }
    }
    return score;
}

pair<int, pair<int, int>> minimax(vector<vector<string>> state, int depth, int alpha, int beta, bool maximizing, char player) {
    if (depth == 0) {
        return {evaluate(state, AI_PLAYER), {-1, -1}};
    }

    vector<pair<int, int>> moves = getLegalActions(maximizing ? player : HUMAN_PLAYER);
    pair<int, int> bestMove = {-1, -1};

    if (maximizing) {
        int maxEval = INT_MIN;
        for (auto move : moves) {
            auto newState = applyAction(state, move, player);
            int eval = minimax(newState, depth - 1, alpha, beta, false, player).first;
            if (eval > maxEval) {
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
            auto newState = applyAction(state, move, HUMAN_PLAYER);
            int eval = minimax(newState, depth - 1, alpha, beta, true, player).first;
            if (eval < minEval) {
                minEval = eval;
                bestMove = move;
            }
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }
        return {minEval, bestMove};
    }
}

void write_ai_move(const string& filename) {
    int depth_limit = 3;
    auto [score, move] = minimax(board, depth_limit, INT_MIN, INT_MAX, true, AI_PLAYER);

    if (move.first != -1) {
        board = applyAction(board, move, AI_PLAYER);
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

    cout << "AI move written.\n";
}

int main() {
    const string filename = "gamestate.txt";
    cout << "C++ AI Engine started. Waiting for Human Move...\n";

    while (true) {
        if (read_human_move(filename)) {
            this_thread::sleep_for(chrono::seconds(1));
            write_ai_move(filename);
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    return 0;
}
