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

// Optimized version that avoids creating temporary vectors
pair<double, double> bestThresholdScore(const vector<vector<string>> &data, int attrIndex, const string &criterion)
{
    vector<pair<double, string>> values;
    values.reserve(data.size()); // Reserve space to avoid reallocations
    
    for (const auto &row : data)
        values.emplace_back(stod(row[attrIndex]), row.back());

    sort(values.begin(), values.end());
    double bestScore = -1.0, bestThresh = 0.0;

    // Pre-compute class frequencies for the entire dataset
    unordered_map<string, int> totalFreq;
    for (const auto &[val, label] : values)
        totalFreq[label]++;

    for (int i = 1; i < values.size(); ++i)
    {
        if (values[i - 1].second != values[i].second)
        {
            double threshold = (values[i - 1].first + values[i].first) / 2;
            
            // Count classes on left and right sides without creating vectors
            unordered_map<string, int> leftFreq, rightFreq;
            int leftSize = 0, rightSize = 0;
            
            for (const auto &[val, label] : values)
            {
                if (val < threshold)
                {
                    leftFreq[label]++;
                    leftSize++;
                }
                else
                {
                    rightFreq[label]++;
                    rightSize++;
                }
            }

            // Calculate entropy directly from frequency maps
            double leftEntropy = 0.0, rightEntropy = 0.0;
            for (const auto &[cls, count] : leftFreq)
            {
                double p = (double)count / leftSize;
                leftEntropy -= p * log2(p);
            }
            for (const auto &[cls, count] : rightFreq)
            {
                double p = (double)count / rightSize;
                rightEntropy -= p * log2(p);
            }

            double total = data.size();
            double baseEntropy = 0.0;
            for (const auto &[cls, count] : totalFreq)
            {
                double p = (double)count / total;
                baseEntropy -= p * log2(p);
            }

            double ig = baseEntropy - (leftSize / total) * leftEntropy - (rightSize / total) * rightEntropy;
            double score = ig;

            if (criterion == "IGR")
            {
                double pLeft = (double)leftSize / total;
                double pRight = 1.0 - pLeft;
                double iv = 0.0;
                if (pLeft > 0) iv -= pLeft * log2(pLeft);
                if (pRight > 0) iv -= pRight * log2(pRight);
                score = (iv > 0) ? ig / iv : 0.0;
            }
            else if (criterion == "NWIG")
            {
                int k = 2; // binary split
                int n = data.size();
                if (k > 1 && n > 1)
                    score = (ig / log2(k + 1)) * (1.0 - (double)(k - 1) / n);
                else
                    score = 0.0;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestThresh = threshold;
            }
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
    // Check if all instances have the same class
    const string &firstClass = data[0].back();
    bool pure = true;
    for (const auto &row : data)
    {
        if (row.back() != firstClass)
        {
            pure = false;
            break;
        }
    }
    
    bool shouldStop = pure || attrIndices.empty() || (maxDepth > 0 && depth >= maxDepth);
    if (shouldStop) {
        node->isLeaf = true;
        node->label = pure ? firstClass : node->majorityClass;
        return node;
    }

    double threshold;
    bool numericSplit;
    int bestAttr = bestAttribute(data, attrIndices, criterion, isNumeric, threshold, numericSplit);
    cout << "depth : " << depth << endl;


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
        left.reserve(data.size() / 2);  // Reserve approximate space
        right.reserve(data.size() / 2);
        
        for (const auto &row : data)
        {
            double val = stod(row[bestAttr]);
            if (val < threshold)
                left.push_back(row);
            else
                right.push_back(row);
        }
        
        if (!left.empty())
            node->children["<"] = buildTree(left, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
        if (!right.empty())
            node->children[">="] = buildTree(right, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
    }
    else
    {
        unordered_map<string, vector<vector<string>>> splits;
        for (const auto &row : data)
            splits[row[bestAttr]].push_back(row);

        vector<int> newAttrs;
        newAttrs.reserve(attrIndices.size() - 1);
        for (int i : attrIndices)
            if (i != bestAttr)
                newAttrs.push_back(i);

        for (auto &[val, subset] : splits)
        {
            if (!subset.empty())
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
            if (isNumeric[i]) // Only check if still considered numeric
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
    }
    return isNumeric;
}

int main()
{
    ifstream file("Datasets/adult.data");
    if (!file.is_open())
    {
        cerr << "Error opening file\n";
        return 1;
    }

    vector<vector<string>> rawRows;
    vector<unordered_map<string, int>> columnValueCounts;

    string line;
    size_t numColumns = 0;

    // Step 1: Read all rows, detect numColumns, count values
    while (getline(file, line))
    {
        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ','))
        {
            // Remove leading/trailing whitespace
            size_t start = token.find_first_not_of(" \t");
            if (start == string::npos) {
                token = "";
            } else {
                size_t end = token.find_last_not_of(" \t");
                token = token.substr(start, end - start + 1);
            }
            tokens.push_back(token);
        }

        if (tokens.empty())
            continue;

        if (numColumns == 0)
        {
            numColumns = tokens.size();
            columnValueCounts.resize(numColumns);
        }

        // Skip inconsistent rows
        if (tokens.size() != numColumns)
            continue;

        rawRows.push_back(tokens);

        for (size_t i = 0; i < numColumns; ++i)
        {
            if (tokens[i] != "?")
            {
                columnValueCounts[i][tokens[i]]++;
            }
        }
    }
    file.close();

    // Step 2: Find column-wise majority values
    vector<string> columnMajority(numColumns);
    for (size_t i = 0; i < numColumns; ++i)
    {
        int maxCount = -1;
        string majority;
        for (const auto &[val, count] : columnValueCounts[i])
        {
            if (count > maxCount)
            {
                maxCount = count;
                majority = val;
            }
        }
        columnMajority[i] = majority;
    }

    // Step 3: Build cleaned dataset
    vector<vector<string>> dataset;
    dataset.reserve(rawRows.size()); // Reserve space
    
    for (auto &row : rawRows)
    {
        for (size_t i = 0; i < numColumns; ++i)
        {
            if (row[i] == "?")
            {
                row[i] = columnMajority[i];
            }
        }
        dataset.push_back(row);
    }

    cout << "Loaded " << dataset.size() << " cleaned rows with " << numColumns << " columns.\n";

    vector<bool> isNumeric = detectNumericColumns(dataset);
    vector<int> attributeIndices;
    for (int i = 0; i < numColumns - 1; ++i)
    { // Exclude last column (class label)
        attributeIndices.push_back(i);
    }

    double totalAccuracy = 0.0;

    // repeating the test 20 times to get a better estimate of accuracy
    for (int i = 0; i < 20; ++i)
    {
        cout << "Iteration " << (i + 1) << ":\n";
        auto [trainSet, testSet] = splitDataset(dataset);
        cout << "Train set size: " << trainSet.size() << ", Test set size: " << testSet.size() << "\n";

        int maxDepth = 0;       // Example max depth
        string criterion = "IG"; // Example criterion
        nodeCount = 0; // Reset node count for each iteration
        Node *root = buildTree(trainSet, attributeIndices, 0, maxDepth,
                               criterion, isNumeric);
        cout << "Total nodes created: " << nodeCount << "\n";
        
        int correct = 0;
        // Removed the row counting output for performance
        for (const auto &row : testSet)
        {
            string actual = row.back();
            string predicted = predict(root, row);
            if (predicted == actual)
            {
                correct++;
            }
        }
        
        double accuracy = (double)correct / testSet.size() * 100.0;
        totalAccuracy += accuracy;
        cout << "Accuracy: " << accuracy << "%\n";
    }

    cout << "Average Accuracy over 20 iterations: " << (totalAccuracy / 20) << "%\n";

    return 0;
}