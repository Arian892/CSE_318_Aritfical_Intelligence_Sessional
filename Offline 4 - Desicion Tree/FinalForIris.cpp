#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

using namespace std;
int nodeCount = 0;

struct Node
{
    bool isLeaf;
    string label; // Class label if leaf
    string majorityClass;
    int splitAttributeIndex;
    bool isNumericSplit;
    double splitThreshold;
    unordered_map<string, Node *> children;
};

// ---------------- Helper Functions ----------------
double entropy(const vector<vector<string>> &data)
{
    unordered_map<string, int> freq;
    for (const auto &row : data)
        freq[row.back()]++;
    double total = data.size(), ent = 0.0;
    for (auto &[cls, count] : freq)
    {
        double p = count / total;
        ent -= p * log2(p);
    }
    return ent;
}


pair<double, double> bestThresholdScore(const vector<vector<string>> &data, int attrIndex, const string &criterion) {
    vector<pair<double, string>> values;
    unordered_map<string, int> classCounts;

    for (const auto &row : data) {
        try {
            double val = stod(row[attrIndex]);
            string cls = row.back();
            values.emplace_back(val, cls);
            classCounts[cls]++;
        } catch (...) {
            continue;
        }
    }

    if (values.empty()) return {-1.0, 0.0};

    sort(values.begin(), values.end());
    double bestScore = -1.0, bestThresh = 0.0;
    double total = values.size();

    unordered_map<string, int> leftCounts;
    int leftSize = 0;

    auto computeEntropy = [](const unordered_map<string, int>& counts, int size) {
        double e = 0.0;
        for (const auto& [cls, cnt] : counts) {
            if (cnt == 0) continue;
            double p = (double)cnt / size;
            e -= p * log2(p);
        }
        return e;
    };

    double globalEntropy = computeEntropy(classCounts, total);

    for (int i = 1; i < values.size(); ++i) {
        const auto& [prevVal, prevCls] = values[i - 1];
        leftCounts[prevCls]++;
        leftSize++;

        double currVal = values[i].first;
        if (currVal == prevVal || values[i].second == prevCls) continue;

        int rightSize = total - leftSize;
        if (leftSize == 0 || rightSize == 0) continue;

        double entropyLeft = computeEntropy(leftCounts, leftSize);

        unordered_map<string, int> rightCounts;
        for (const auto& [cls, totalCount] : classCounts) {
            rightCounts[cls] = totalCount - leftCounts[cls];
        }

        double entropyRight = computeEntropy(rightCounts, rightSize);
        double weightedEntropy = (leftSize / total) * entropyLeft + (rightSize / total) * entropyRight;
        double ig = globalEntropy - weightedEntropy;
        double score = ig;

        if (criterion == "IGR") {
            double pL = (double)leftSize / total, pR = 1.0 - pL;
            double iv = 0.0;
            if (pL > 0) iv -= pL * log2(pL);
            if (pR > 0) iv -= pR * log2(pR);
            score = (iv > 0) ? ig / iv : 0.0;
        } else if (criterion == "NWIG") {
            int k = 2, n = total;
            score = (ig / log2(k + 1)) * (1.0 - (double)(k - 1) / n);
        }

        double threshold = (prevVal + currVal) / 2;
        if (score > bestScore) {
            bestScore = score;
            bestThresh = threshold;
        }
    }

    return {bestScore, bestThresh};
}

double infoGain(const vector<vector<string>> &data, int attrIndex)
{
    double baseEntropy = entropy(data);
    unordered_map<string, vector<vector<string>>> splits;
    for (const auto &row : data)
        splits[row[attrIndex]].push_back(row);
    double weightedEntropy = 0.0;
    for (auto &[val, subset] : splits)
    {
        double weight = (double)subset.size() / data.size();
        weightedEntropy += weight * entropy(subset);
    }
    return baseEntropy - weightedEntropy;
}

double infoGainRatio(const vector<vector<string>> &data, int attrIndex)
{
    double ig = infoGain(data, attrIndex);
    unordered_map<string, int> valueCounts;
    for (const auto &row : data)
        valueCounts[row[attrIndex]]++;

    double iv = 0.0;
    for (const auto &[val, count] : valueCounts)
    {
        double p = (double)count / data.size();
        iv -= p * log2(p);
    }
    if (iv == 0)
        return 0.0;
    return ig / iv;
}

double normalizedWeightedIG(const vector<vector<string>> &data, int attrIndex)
{
    double ig = infoGain(data, attrIndex);
    unordered_set<string> uniqueVals;
    for (const auto &row : data)
        uniqueVals.insert(row[attrIndex]);
    int k = uniqueVals.size();
    int n = data.size();
    if (k <= 1 || n <= 1)
        return 0.0;
    return (ig / log2(k + 1)) * (1.0 - (double)(k - 1) / n);
}

string majorityClass(const vector<vector<string>> &data)
{
    unordered_map<string, int> freq;
    for (const auto &row : data)
        freq[row.back()]++;
    string maj;
    int maxC = -1;
    for (auto &[label, count] : freq)
    {
        if (count > maxC)
        {
            maj = label;
            maxC = count;
        }
    }
    return maj;
}

int bestAttribute(const vector<vector<string>> &data, const vector<int> &attrIndices, const string &criterion,
                  const vector<bool> &isNumeric, double &outThreshold, bool &outIsNumeric)
{

    double bestScore = -1.0;
    int bestAttr = -1;
    outThreshold = 0.0;
    outIsNumeric = false;

    for (int index : attrIndices)
    {
        double score = 0.0, threshold = 0.0;
        bool numeric = isNumeric[index];

        if (numeric)
        {
            auto [s, t] = bestThresholdScore(data, index, criterion);
            score = s;
            threshold = t;
        }

        else
        {
            if (criterion == "IG")
                score = infoGain(data, index);
            else if (criterion == "IGR")
                score = infoGainRatio(data, index);
            else if (criterion == "NWIG")
                score = normalizedWeightedIG(data, index);
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestAttr = index;
            outThreshold = threshold;
            outIsNumeric = numeric;
        }
    }

    if (bestAttr == -1) {
    outIsNumeric = false;
    outThreshold = 0.0;
}
    return bestAttr;
}

Node *buildTree(const vector<vector<string>> &data, vector<int> attrIndices, int depth, int maxDepth,
                const string &criterion, const vector<bool> &isNumeric)
{

    Node *node = new Node();
    nodeCount++;
    node->majorityClass = majorityClass(data);


    bool pure = true;
    for (const auto &row : data)
    {
        if (row.back() != data[0].back())
        {
            pure = false;
            break;
        }
    }
    bool shouldStop = pure || attrIndices.empty() || (maxDepth != 0 && depth == maxDepth);
    if (shouldStop) {
        node->isLeaf = true;
        node->label = data[0].back();
        return node;
    }
    


    double threshold;
    bool numericSplit;
    int bestAttr = bestAttribute(data, attrIndices, criterion, isNumeric, threshold, numericSplit);


    if (bestAttr == -1)
    {
        node->isLeaf = true;
        node->label = node->majorityClass;
        return node;
    }

    node->isLeaf = false;
    node->splitAttributeIndex = bestAttr;
    node->isNumericSplit = numericSplit;
    node->splitThreshold = threshold;

    if (numericSplit)
    {
        vector<vector<string>> left, right;
        for (const auto &row : data)
        {
            double val = stod(row[bestAttr]);
            if (val < threshold)
                left.push_back(row);
            else
                right.push_back(row);
        }
        node->children["<"] = buildTree(left, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
        node->children[">="] = buildTree(right, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
    }
    else
    {
        unordered_map<string, vector<vector<string>>> splits;
        for (const auto &row : data)
            splits[row[bestAttr]].push_back(row);

        vector<int> newAttrs;
        for (int i : attrIndices)
            if (i != bestAttr)
                newAttrs.push_back(i);

        for (auto &[val, subset] : splits)
        {
            node->children[val] = buildTree(subset, newAttrs, depth + 1, maxDepth, criterion, isNumeric);
        }
    }

    return node;
}

string predict(Node *root, const vector<string> &instance)
{
    Node *current = root;
    while (!current->isLeaf)
    {
        int attr = current->splitAttributeIndex;
        if (current->isNumericSplit)
        {
            double val = stod(instance[attr]);
            current = (val < current->splitThreshold) ? current->children["<"] : current->children[">="];
        }
        else
        {
            string val = instance[attr];
            if (current->children.count(val))
            {
                current = current->children[val];
            }
            else
            {
                return current->majorityClass;
            }
        }
    }
    return current->label;
}

pair<vector<vector<string>>, vector<vector<string>>> splitDataset(vector<vector<string>> &dataset)
{
    // Shuffle the dataset randomly
    random_device rd;
    mt19937 g(rd());
    shuffle(dataset.begin(), dataset.end(), g);

    // Compute split point
    size_t trainSize = dataset.size() * 0.8;

    // Create train and test sets
    vector<vector<string>> trainSet(dataset.begin(), dataset.begin() + trainSize);
    vector<vector<string>> testSet(dataset.begin() + trainSize, dataset.end());

    return {trainSet, testSet};
}

vector<bool> detectNumericColumns(const vector<vector<string>> &data)
{
    int numCols = data[0].size() - 1; // exclude class label
    vector<bool> isNumeric(numCols, true);

    for (const auto &row : data)
    {
        for (int i = 0; i < numCols; ++i)
        {
            try
            {
                stod(row[i]); // attempt conversion
            }
            catch (...)
            {
                isNumeric[i] = false;
            }
        }
    }
    return isNumeric;
}


int main() {
ifstream file("Datasets/Iris.csv");
if (!file.is_open()) {
    cerr << "Error opening file\n";
    return 1;
}

vector<vector<string>> rawRows;
vector<unordered_map<string, int>> columnValueCounts;

string line;
size_t numColumns = 0;

// Step 0: Read and discard header line
if (!getline(file, line)) {
    cerr << "Empty file or read error\n";
    return 1;
}
stringstream headerSS(line);
string headerToken;
vector<string> headerTokens;
while (getline(headerSS, headerToken, ',')) {
    headerToken.erase(remove_if(headerToken.begin(), headerToken.end(), ::isspace), headerToken.end());
    headerTokens.push_back(headerToken);
}
numColumns = headerTokens.size();

if (numColumns < 2) {
    cerr << "Not enough columns\n";
    return 1;
}

// We ignore first column (ID), so prepare for remaining columns only:
size_t numFeatures = numColumns - 1;  // exclude ID column only, keep label column

// Initialize value counts only for features and label (skip ID column at index 0)
columnValueCounts.resize(numFeatures);

while (getline(file, line)) {
    stringstream ss(line);
    string token;
    vector<string> tokens;

    while (getline(ss, token, ',')) {
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        tokens.push_back(token);
    }

    if (tokens.size() != numColumns) continue; // skip inconsistent rows

    // Extract columns except the ID (index 0)
    vector<string> filteredRow;
    for (size_t i = 1; i < numColumns; ++i) {
        filteredRow.push_back(tokens[i]);
    }

    rawRows.push_back(filteredRow);

    // Update counts for columns excluding ID (already filtered)
    for (size_t i = 0; i < numFeatures; ++i) {
        if (filteredRow[i] != "?") {
            columnValueCounts[i][filteredRow[i]]++;
        }
    }
}
file.close();

// Find majority values for each column (feature + label)
vector<string> columnMajority(numFeatures);
for (size_t i = 0; i < numFeatures; ++i) {
    int maxCount = -1;
    string majority;
    for (const auto& [val, count] : columnValueCounts[i]) {
        if (count > maxCount) {
            maxCount = count;
            majority = val;
        }
    }
    columnMajority[i] = majority;
}

// Build cleaned dataset with replaced missing values
vector<vector<string>> dataset;
for (auto& row : rawRows) {
    for (size_t i = 0; i < numFeatures; ++i) {
        if (row[i] == "?") {
            row[i] = columnMajority[i];
        }
    }
    dataset.push_back(row);
}

// Now dataset rows contain only feature columns (SepalLengthCm ... PetalWidthCm) and label as last column
// ID column is excluded entirely


    cout << "Loaded " << dataset.size() << " cleaned rows with " << numColumns << " columns.\n";
    // printDataset(dataset);



    vector<bool> isNumeric = detectNumericColumns(dataset);

    for (int i = 0; i < isNumeric.size(); ++i) {
        cout << "Column " << i << " (" << headerTokens[i + 1] << ") is "
             << (isNumeric[i] ? "numeric" : "categorical") << ".\n";
    }
    vector<int> attributeIndices;
    for (int i = 0; i < numColumns - 2; ++i) { // Exclude last column (class label)
        attributeIndices.push_back(i);
    }

vector<string> criteria = {"IG", "IGR", "NWIG"};

for (const string& criterion : criteria)
{
    string csvFile = "(IRIS)accuracy_vs_depth_" + criterion + ".csv";

    ofstream outCsv(csvFile);

    outCsv << "depth,avg_accuracy,avg_node_count\n";

    for (int maxDepth = 0; maxDepth <= 10; ++maxDepth)
    {
        double totalAccuracy = 0.0;
        int totalNodes = 0;

        for (int i = 0; i < 20; ++i)
        {
            auto [trainSet, testSet] = splitDataset(dataset);

            nodeCount = 0;
            Node *root = buildTree(trainSet, attributeIndices, 0, maxDepth, criterion, isNumeric);
            totalNodes += nodeCount;

            int correct = 0;
            for (const auto &row : testSet)
            {
                string actual = row.back();
                string predicted = predict(root, row);
                if (predicted == actual)
                    correct++;
            }

            double accuracy = (double)correct / testSet.size() * 100.0;
            totalAccuracy += accuracy;

            // cout << "[" << criterion << "] Depth " << maxDepth
            //      << ", Iteration " << (i + 1) << ": Accuracy = "
            //      << accuracy << "%\n";
        }

        double avgAccuracy = totalAccuracy / 20;
        double avgNodes = (double)totalNodes / 20;

        cout << "[" << criterion << "] Depth " << maxDepth
             << ": Avg Accuracy = " << avgAccuracy
             << "%, Avg Nodes = " << avgNodes << "\n";

        outCsv << maxDepth << "," << avgAccuracy << "," << avgNodes << "\n";
    }

    outCsv.close();
}
    return 0;
}
