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
pair<double, double> bestThresholdScore(const vector<vector<string>> &data, int attrIndex, const string &criterion)
{
    vector<pair<double, string>> values;
    unordered_map<string, int> totalClassCount;

    // Step 1: Collect numeric values and class labels
    for (int i = 0; i < data.size(); i++)
    {
        string valueStr = data[i][attrIndex];
        string classLabel = data[i].back();

        try
        {
            double value = stod(valueStr);
            values.push_back({value, classLabel});
            totalClassCount[classLabel]++;
        }
        catch (...)
        {
            // Skipping non-numeric values
        }
    }

    // Returning if we couldn’t find any numeric values
    if (values.empty())
    {
        return {-1.0, 0.0};
    }

    // Sorting values based on the numeric attribute
    sort(values.begin(), values.end());

    double bestScore = -1.0;
    double bestThreshold = 0.0;
    int totalSize = values.size();

    unordered_map<string, int> leftClassCount;
    int leftSize = 0;

    double globalEntropy = 0.0;
    for (auto it : totalClassCount)
    {
        double prob = (double)it.second / totalSize;
        globalEntropy -= prob * log2(prob);
    }

    for (int i = 1; i < values.size(); i++)
    {
        string previousClass = values[i - 1].second;
        double previousValue = values[i - 1].first;

        leftClassCount[previousClass]++;
        leftSize++;

        double currentValue = values[i].first;
        string currentClass = values[i].second;

        if (currentValue == previousValue || currentClass == previousClass)
        {
            continue;
        }

        int rightSize = totalSize - leftSize;
        if (leftSize == 0 || rightSize == 0)
        {
            continue;
        }

        double leftEntropy = 0.0;
        for (auto it : leftClassCount)
        {
            double prob = (double)it.second / leftSize;
            if (prob > 0)
                leftEntropy -= prob * log2(prob);
        }

        unordered_map<string, int> rightClassCount;
        for (auto it : totalClassCount)
        {
            string label = it.first;
            rightClassCount[label] = it.second - leftClassCount[label];
        }

        double rightEntropy = 0.0;
        for (auto it : rightClassCount)
        {
            double prob = (double)it.second / rightSize;
            if (prob > 0)
                rightEntropy -= prob * log2(prob);
        }

        double weightedEntropy = (leftSize / (double)totalSize) * leftEntropy +
                                 (rightSize / (double)totalSize) * rightEntropy;

        double informationGain = globalEntropy - weightedEntropy;
        double score = informationGain;

        if (criterion == "IGR")
        {
            double pLeft = (double)leftSize / totalSize;
            double pRight = 1.0 - pLeft;
            double intrinsicValue = 0.0;

            if (pLeft > 0)
                intrinsicValue -= pLeft * log2(pLeft);
            if (pRight > 0)
                intrinsicValue -= pRight * log2(pRight);

            if (intrinsicValue > 0)
                score = informationGain / intrinsicValue;
            else
                score = 0.0;
        }
        else if (criterion == "NWIG")
        {
            int k = 2; // binary split
            int n = totalSize;
            score = (informationGain / log2(k + 1)) * (1.0 - (double)(k - 1) / n);
        }

        double threshold = (previousValue + currentValue) / 2.0;

        if (score > bestScore)
        {
            bestScore = score;
            bestThreshold = threshold;
        }
    }

    return {bestScore, bestThreshold};
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

int bestAttribute(const vector<vector<string>> &data,
                  const vector<int> &attrIndices,
                  const string &criterion,
                  const vector<bool> &isNumeric,
                  double &outThreshold,
                  bool &outIsNumeric)
{
    double bestScore = -1.0;
    int bestAttrIndex = -1;

    outThreshold = 0.0;
    outIsNumeric = false;

    for (int i = 0; i < attrIndices.size(); i++)
    {
        int attr = attrIndices[i];
        double score = 0.0;
        double threshold = 0.0;
        bool isNum = isNumeric[attr];

        if (isNum)
        {
            pair<double, double> result = bestThresholdScore(data, attr, criterion);
            score = result.first;
            threshold = result.second;
        }
        else
        {
            if (criterion == "IG")
                score = infoGain(data, attr);
            else if (criterion == "IGR")
                score = infoGainRatio(data, attr);
            else if (criterion == "NWIG")
                score = normalizedWeightedIG(data, attr);
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestAttrIndex = attr;
            outThreshold = threshold;
            outIsNumeric = isNum;
        }
    }

    if (bestAttrIndex == -1)
    {
        outThreshold = 0.0;
        outIsNumeric = false;
    }

    return bestAttrIndex;
}

Node *buildTree(const vector<vector<string>> &data,
                vector<int> attrIndices,
                int depth,
                int maxDepth,
                const string &criterion,
                const vector<bool> &isNumeric)
{
    Node *node = new Node();
    nodeCount++;
    node->majorityClass = majorityClass(data);

    // Check if all rows have same class label (pure leaf)
    bool allSame = true;
    for (int i = 0; i < data.size(); i++)
    {
        if (data[i].back() != data[0].back())
        {
            allSame = false;
            break;
        }
    }

    if (allSame || attrIndices.empty() || (maxDepth != 0 && depth == maxDepth))
    {
        node->isLeaf = true;
        node->label = data[0].back(); // Since all are same or majority
        return node;
    }

    double bestThreshold = 0.0;
    bool isNumericSplit = false;
    int bestAttr = bestAttribute(data, attrIndices, criterion, isNumeric, bestThreshold, isNumericSplit);

    if (bestAttr == -1)
    {
        node->isLeaf = true;
        node->label = node->majorityClass;
        return node;
    }

    node->isLeaf = false;
    node->splitAttributeIndex = bestAttr;
    node->isNumericSplit = isNumericSplit;
    node->splitThreshold = bestThreshold;

    if (isNumericSplit)
    {
        vector<vector<string>> leftData;
        vector<vector<string>> rightData;

        for (int i = 0; i < data.size(); i++)
        {
            double value = stod(data[i][bestAttr]);
            if (value < bestThreshold)
                leftData.push_back(data[i]);
            else
                rightData.push_back(data[i]);
        }

        node->children["<"] = buildTree(leftData, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
        node->children[">="] = buildTree(rightData, attrIndices, depth + 1, maxDepth, criterion, isNumeric);
    }
    else
    {
        unordered_map<string, vector<vector<string>>> splitMap;
        for (int i = 0; i < data.size(); i++)
        {
            string val = data[i][bestAttr];
            splitMap[val].push_back(data[i]);
        }

        vector<int> newAttrIndices;
        for (int i = 0; i < attrIndices.size(); i++)
        {
            if (attrIndices[i] != bestAttr)
                newAttrIndices.push_back(attrIndices[i]);
        }

        for (auto it = splitMap.begin(); it != splitMap.end(); ++it)
        {
            string attrVal = it->first;
            vector<vector<string>> subset = it->second;
            node->children[attrVal] = buildTree(subset, newAttrIndices, depth + 1, maxDepth, criterion, isNumeric);
        }
    }

    return node;
}


string predict(Node *root, const vector<string> &instance)
{
    Node *current = root;

    while (current->isLeaf == false)
    {
        int attrIndex = current->splitAttributeIndex;

        if (current->isNumericSplit)
        {
            string valueStr = instance[attrIndex];
            double value = stod(valueStr);
            double threshold = current->splitThreshold;

            if (value < threshold)
            {
                current = current->children["<"];
            }
            else
            {
                current = current->children[">="];
            }
        }
        // for cateogirical attributes
        else
        {
            string attrValue = instance[attrIndex];

            // Check if the tree has a child for this value
            if (current->children.count(attrValue) > 0)
            {
                current = current->children[attrValue];
            }
            else
            {
                // If unseen value, use majority class of current node
                return current->majorityClass;
            }
        }
    }

    // Once we reach a leaf, return its class label
    return current->label;
}

pair<vector<vector<string>>, vector<vector<string>>> splitDataset(vector<vector<string>> &dataset)
{
    // Shuffle the dataset randomly
    random_device rd;
    mt19937 g(rd());
    shuffle(dataset.begin(), dataset.end(), g);

    // Computing split point
    size_t trainSize = dataset.size() * 0.8;

    // Creating train and test sets
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
                stod(row[i]); 
            }
            catch (...)
            {
                isNumeric[i] = false;
            }
        }
    }

    return isNumeric;
}


// int main() {
//     // Choose your dataset file
//     string filename = "Datasets/adult.data"; // Change to "Datasets/Iris.csv" for Iris dataset

//     // Prepare containers to hold the dataset and metadata
//     vector<vector<string>> rawRows;                  // Original rows before cleaning
//     vector<unordered_map<string, int>> valueCounts;  // Count of values per column (for majority handling)
//     vector<vector<string>> dataset;                  // Final cleaned dataset
//     vector<string> columnMajority;                   // Majority values to replace missing data
//     vector<string> headerTokens;                     // Column names (from Iris or generated for Adult)

//     // Try to open the file
//     ifstream file(filename);
//     if (!file.is_open()) {
//         cerr << "Could not open file: " << filename << "\n";
//         return 1;
//     }

//     string line;
//     size_t numColumns = 0;
//     bool isIris = filename.find("Iris.csv") != string::npos;

//     // ---------- Read and clean the dataset ----------
//     if (isIris) {
//         // For Iris: skip the header line
//         if (!getline(file, line)) {
//             cerr << "Iris file seems empty.\n";
//             return 1;
//         }

//         // Parse header to get column names
//         stringstream ss(line);
//         string token;
//         while (getline(ss, token, ',')) {
//             token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
//             headerTokens.push_back(token);
//         }

//         numColumns = headerTokens.size();
//         size_t numFeatures = numColumns - 1; // Skip the first column (ID)
//         valueCounts.resize(numFeatures);

//         // Read the actual data
//         while (getline(file, line)) {
//             stringstream ss(line);
//             vector<string> tokens;
//             while (getline(ss, token, ',')) {
//                 token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
//                 tokens.push_back(token);
//             }

//             if (tokens.size() != numColumns) continue;

//             vector<string> row;
//             for (size_t i = 1; i < tokens.size(); ++i)
//                 row.push_back(tokens[i]);

//             rawRows.push_back(row);

//             for (size_t i = 0; i < row.size(); ++i) {
//                 if (row[i] != "?") valueCounts[i][row[i]]++;
//             }
//         }

//         // Replace missing values using majority
//         columnMajority.resize(numFeatures);
//         for (size_t i = 0; i < numFeatures; ++i) {
//             int maxCount = -1;
//             for (auto &[val, cnt] : valueCounts[i]) {
//                 if (cnt > maxCount) {
//                     maxCount = cnt;
//                     columnMajority[i] = val;
//                 }
//             }
//         }

//         for (auto &row : rawRows) {
//             for (size_t i = 0; i < row.size(); ++i)
//                 if (row[i] == "?") row[i] = columnMajority[i];
//             dataset.push_back(row);
//         }

//         // Remove ID from header tokens
//         headerTokens.erase(headerTokens.begin());

//     } else {
//         // For Adult dataset: no header row
//         while (getline(file, line)) {
//             stringstream ss(line);
//             string token;
//             vector<string> tokens;

//             while (getline(ss, token, ',')) {
//                 token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
//                 tokens.push_back(token);
//             }

//             if (tokens.empty()) continue;

//             if (numColumns == 0) {
//                 numColumns = tokens.size();
//                 valueCounts.resize(numColumns);
//                 for (size_t i = 0; i < numColumns; ++i)
//                     headerTokens.push_back("Attr" + to_string(i));
//             }

//             if (tokens.size() != numColumns) continue;

//             rawRows.push_back(tokens);

//             for (size_t i = 0; i < tokens.size(); ++i) {
//                 if (tokens[i] != "?") valueCounts[i][tokens[i]]++;
//             }
//         }

//         // Replace missing values using majority
//         columnMajority.resize(numColumns);
//         for (size_t i = 0; i < numColumns; ++i) {
//             int maxCount = -1;
//             for (auto &[val, cnt] : valueCounts[i]) {
//                 if (cnt > maxCount) {
//                     maxCount = cnt;
//                     columnMajority[i] = val;
//                 }
//             }
//         }

//         for (auto &row : rawRows) {
//             for (size_t i = 0; i < row.size(); ++i)
//                 if (row[i] == "?") row[i] = columnMajority[i];
//             dataset.push_back(row);
//         }
//     }

//     file.close();
//     cout << "Loaded " << dataset.size() << " cleaned rows from " << filename << "\n";

//     vector<bool> isNumeric = detectNumericColumns(dataset);
//     for (int i = 0; i < isNumeric.size(); ++i) {
//         string colName = (i < headerTokens.size()) ? headerTokens[i] : "Attr" + to_string(i);
//         cout << "Column " << i << " (" << colName << ") is "
//              << (isNumeric[i] ? "numeric" : "categorical") << ".\n";
//     }

//     // Prepare attribute indices (exclude last column: class label)
//     vector<int> attributeIndices;
//     for (int i = 0; i < dataset[0].size() - 1; ++i)
//         attributeIndices.push_back(i);

//     vector<string> criteria = {"IG", "IGR", "NWIG"};
//     for (const string &criterion : criteria) {
//         string outFile = "accuracy_vs_depth_" + criterion + ".csv";
//         ofstream out(outFile);
//         out << "depth,avg_accuracy,avg_node_count\n";

//         for (int maxDepth = 0; maxDepth <= 10; ++maxDepth) {
//             double totalAccuracy = 0.0;
//             int totalNodes = 0;

//             for (int rep = 0; rep < 20; ++rep) {
//                 auto [trainSet, testSet] = splitDataset(dataset);
//                 nodeCount = 0;

//                 Node *root = buildTree(trainSet, attributeIndices, 0, maxDepth, criterion, isNumeric);
//                 totalNodes += nodeCount;

//                 int correct = 0;
//                 for (const auto &row : testSet) {
//                     string actual = row.back();
//                     string predicted = predict(root, row);
//                     if (predicted == actual) correct++;
//                 }

//                 totalAccuracy += (double)correct / testSet.size() * 100.0;
//             }

//             double avgAcc = totalAccuracy / 20.0;
//             double avgNodes = totalNodes / 20.0;

//             cout << "[" << criterion << "] Depth " << maxDepth
//                  << " → Accuracy: " << avgAcc << "%, Nodes: " << avgNodes << "\n";

//             out << maxDepth << "," << avgAcc << "," << avgNodes << "\n";
//         }

//         out.close();
//     }

//     return 0;
// }


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <criterion> <maxDepth>\n";
        return 1;
    }

    string criterion = argv[1];            // e.g., "IG", "IGR", or "NWIG"
    int maxDepth = stoi(argv[2]);         // e.g., 3

    string filename = "Datasets/Iris.csv"; // or "Datasets/Iris.csv"

    vector<vector<string>> rawRows;
    vector<unordered_map<string, int>> valueCounts;
    vector<vector<string>> dataset;
    vector<string> columnMajority;
    vector<string> headerTokens;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open file: " << filename << "\n";
        return 1;
    }

    string line;
    size_t numColumns = 0;
    bool isIris = filename.find("Iris.csv") != string::npos;

    if (isIris) {
        if (!getline(file, line)) {
            cerr << "Iris file is empty.\n";
            return 1;
        }

        stringstream ss(line);
        string token;
        while (getline(ss, token, ',')) {
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
            headerTokens.push_back(token);
        }

        numColumns = headerTokens.size();
        size_t numFeatures = numColumns - 1;
        valueCounts.resize(numFeatures);

        while (getline(file, line)) {
            stringstream ss(line);
            vector<string> tokens;
            while (getline(ss, token, ',')) {
                token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
                tokens.push_back(token);
            }
            if (tokens.size() != numColumns) continue;

            vector<string> row(tokens.begin() + 1, tokens.end());
            rawRows.push_back(row);
            for (size_t i = 0; i < row.size(); ++i)
                if (row[i] != "?") valueCounts[i][row[i]]++;
        }

        columnMajority.resize(numFeatures);
        for (size_t i = 0; i < numFeatures; ++i) {
            int maxCount = -1;
            for (auto &[val, cnt] : valueCounts[i]) {
                if (cnt > maxCount) {
                    maxCount = cnt;
                    columnMajority[i] = val;
                }
            }
        }

        for (auto &row : rawRows) {
            for (size_t i = 0; i < row.size(); ++i)
                if (row[i] == "?") row[i] = columnMajority[i];
            dataset.push_back(row);
        }

        headerTokens.erase(headerTokens.begin()); // remove ID

    } else {
        while (getline(file, line)) {
            stringstream ss(line);
            string token;
            vector<string> tokens;
            while (getline(ss, token, ',')) {
                token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
                tokens.push_back(token);
            }

            if (tokens.empty()) continue;

            if (numColumns == 0) {
                numColumns = tokens.size();
                valueCounts.resize(numColumns);
                for (size_t i = 0; i < numColumns; ++i)
                    headerTokens.push_back("Attr" + to_string(i));
            }

            if (tokens.size() != numColumns) continue;

            rawRows.push_back(tokens);
            for (size_t i = 0; i < tokens.size(); ++i)
                if (tokens[i] != "?") valueCounts[i][tokens[i]]++;
        }

        columnMajority.resize(numColumns);
        for (size_t i = 0; i < numColumns; ++i) {
            int maxCount = -1;
            for (auto &[val, cnt] : valueCounts[i]) {
                if (cnt > maxCount) {
                    maxCount = cnt;
                    columnMajority[i] = val;
                }
            }
        }

        for (auto &row : rawRows) {
            for (size_t i = 0; i < row.size(); ++i)
                if (row[i] == "?") row[i] = columnMajority[i];
            dataset.push_back(row);
        }
    }

    file.close();
    cout << "Loaded " << dataset.size() << " cleaned rows from " << filename << "\n";

    // Detect numeric columns
    vector<bool> isNumeric = detectNumericColumns(dataset);

    vector<int> attributeIndices;
    for (int i = 0; i < dataset[0].size() - 1; ++i)
        attributeIndices.push_back(i);

    // Train & evaluate
    double totalAccuracy = 0.0;
    int totalNodes = 0;

    for (int rep = 0; rep < 20; ++rep) {
        auto [trainSet, testSet] = splitDataset(dataset);
        nodeCount = 0;

        Node *root = buildTree(trainSet, attributeIndices, 0, maxDepth, criterion, isNumeric);
        totalNodes += nodeCount;

        int correct = 0;
        for (const auto &row : testSet) {
            string actual = row.back();
            string predicted = predict(root, row);
            if (predicted == actual) correct++;
        }

        totalAccuracy += (double)correct / testSet.size() * 100.0;
    }

    double avgAcc = totalAccuracy / 20.0;
    double avgNodes = totalNodes / 20.0;

    cout << "\nCriterion: " << criterion << ", Max Depth: " << maxDepth << "\n";
    cout << "Avg Accuracy: " << avgAcc << "%\n";
    cout << "Avg Nodes: " << avgNodes << "\n";

    return 0;
}
