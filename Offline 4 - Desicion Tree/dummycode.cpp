#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <string>
#include <algorithm>

using namespace std;

const int NUM_COLUMNS = 15;

// Column names for readability
const vector<string> columnNames = {
    "age", "workclass", "fnlwgt", "education", "education_num", "marital_status",
    "occupation", "relationship", "race", "sex", "capital_gain", "capital_loss",
    "hours_per_week", "native_country", "income"
};

int main() {
    ifstream file("Datasets/adult.data");
    if (!file.is_open()) {
        cerr << "Error opening file\n";
        return 1;
    }

    ofstream out("column_values.txt");
    if (!out.is_open()) {
        cerr << "Error creating output file\n";
        return 1;
    }

    vector<unordered_set<string>> uniqueValues(NUM_COLUMNS);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        vector<string> tokens;
        
        while (getline(ss, token, ',')) {
            // Trim whitespace
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
            tokens.push_back(token);
        }

        // Skip incomplete rows
        if (tokens.size() != NUM_COLUMNS) continue;

        for (int i = 0; i < NUM_COLUMNS; ++i) {
            uniqueValues[i].insert(tokens[i]);
        }
    }

    // Write output to file
    for (int i = 0; i < NUM_COLUMNS; ++i) {
        out << "Column [" << columnNames[i] << "] has " 
            << uniqueValues[i].size() << " unique values:\n";
        // for (const string& val : uniqueValues[i]) {
        //     out << "  " << val << "\n";
        // }
        out << "-----------------------\n";
    }

    file.close();
    out.close();

    return 0;
}
