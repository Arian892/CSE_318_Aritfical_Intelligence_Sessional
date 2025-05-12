#include <iostream>
#include<vector>
#include<algorithm>
#include <fstream>
#include<map>
using namespace std;


class Graph{
    int vertices;
    int edges ;
    vector<vector<int>> adj_list;
    vector<vector<int>> adj_matrix;

    public:
    Graph (int v, int e) : vertices(v), edges(e) {
        adj_list.resize(vertices+1);
        adj_matrix.resize(vertices+1, vector<int>(vertices+1, 0));


    }

    void add_edge(int u, int v, int w) {
        adj_list[u].push_back(v);
        adj_list[v].push_back(u);
        adj_matrix[u][v] = w;
        adj_matrix[v][u] = w;
    }
    void print_adj_list() {
        for (int i = 1; i <= vertices; i++) {
            cout << i << " -> ";
            for (int j : adj_list[i]) {
                cout << j << " ";
            }
            cout << endl;
        }
    }
    void print_adj_matrix() {
        for (int i = 1; i <= vertices; i++) {
            for (int j = 1; j <= vertices; j++) {
                cout << adj_matrix[i][j] << " ";
            }
            cout << endl;
        }
    }

    // randomized heiuristic for Max cut

   double randomized_heuristic(int n ) {
       vector<int> set1, set2;

       int total_cut_weight = 0;
       for (int k = 0; k < n; k++)
       {
        set1.clear();
        set2.clear();
           for (int i = 1; i <= vertices; i++)
           {
               if (rand() % 2 == 0)
               {
                   set1.push_back(i);
               }
               else
               {
                   set2.push_back(i);
               }
           }
            // finding cut weight
           int cut_weight = 0;
           for (int i = 0; i < set1.size(); i++)
           {
               for (int j = 0; j < set2.size(); j++)
               {
                   cut_weight += adj_matrix[set1[i]][set2[j]];
               }
           }
           total_cut_weight += cut_weight;
       }

       double avg_cut_weight = total_cut_weight*1.0 / n;

      return avg_cut_weight ; 
        
        
    }


    int calculate_w(int vertex, vector<int>u){
        int w = 0;
        for (int i = 0; i < u.size(); i++)
        {
            w += adj_matrix[vertex][u[i]];
        }
        return w;

    }
    int calculate_cut_weight(vector<int> &set1, vector<int> &set2) {
        int cut_weight = 0;
        for (int u : set1) {
            for (int v : set2) {
                cut_weight += adj_matrix[u][v];
            }
        }
        return cut_weight;
    }

    


    // greedy heuristic for Max cut
    double greedy_heuristic() {
        vector<int> set1, set2;
       
        vector<bool> visited(vertices + 1, false);
    
        //Finding the maximum edge
        int max_u = -1, max_v = -1, max_weight = -1;
        for (int u = 1; u <= vertices; u++) {
            for (int v = u + 1; v <= vertices; v++) {
                if (adj_matrix[u][v] > max_weight) {
                    max_weight = adj_matrix[u][v];
                    max_u = u;
                    max_v = v;
                }
            }
        }

        //Adding the maximum edge to set1
        set1.push_back(max_u);
        set1.push_back(max_v);
        visited[max_u] = true;
        visited[max_v] = true;

        // for each vertex have 
        for (int i= 1 ; i<= vertices ; i++){
            if (!visited[i]){

                int w_x = calculate_w(i, set2);
                int w_y = calculate_w(i, set1);
                if (w_x > w_y){
                    set1.push_back(i);
                    visited[i] = true;
                }
                else{
                    set2.push_back(i);
                    visited[i] = true;
                }

            }

        }  



        // finding cut weight
        int cut_weight = calculate_cut_weight(set1, set2);
       

        return cut_weight;
    }


    pair <vector<int>,vector<int>> semi_greedy_heuristic(double alpha) {
        vector<int> set1, set2;
        vector<bool> visited(vertices + 1, false);
    
        // Step 1: Find the maximum weight edge
        int max_u = -1, max_v = -1, max_weight = -1;
        for (int u = 1; u <= vertices; u++) {
            for (int v = u + 1; v <= vertices; v++) {
                if (adj_matrix[u][v] > max_weight) {
                    max_weight = adj_matrix[u][v];
                    max_u = u;
                    max_v = v;
                }
            }
        }
    
        set1.push_back(max_u);
        set2.push_back(max_v);
        visited[max_u] = true;
        visited[max_v] = true;
    
        while (set1.size() + set2.size() < vertices) {
            vector<int> candidates;
            for (int i = 1; i <= vertices; i++) {
                if (!visited[i]) candidates.push_back(i);
            }
    
            vector<int> greedy_values(candidates.size());
            for (int i = 0; i < candidates.size(); i++) {
                int v = candidates[i];
                int sigma1 = 0, sigma2 = 0;
                sigma1 = calculate_w(v, set1);
                sigma2 = calculate_w(v, set2);
                // for (int u : set1) sigma1 += adj_matrix[v][u];
                // for (int u : set2) sigma2 += adj_matrix[v][u];
                greedy_values[i] = max(sigma1, sigma2);
            }
    
            int w_min = greedy_values[0], w_max = greedy_values[0];
            for (int val : greedy_values) {
                if (val < w_min) w_min = val;
                if (val > w_max) w_max = val;
            }
    
            double mu = w_min + alpha * (w_max - w_min);
    
            // Build RCL
            vector<int> RCL;
            for (int i = 0; i < candidates.size(); i++) {
                if (greedy_values[i] >= mu) {
                    RCL.push_back(candidates[i]);
                }
            }
    
            // Randomly select from RCL
            int chosen_index = rand() % RCL.size();
            int chosen_vertex = RCL[chosen_index];
            visited[chosen_vertex] = true;
    
            // Recompute sigma for placement
            int sigma1 = 0, sigma2 = 0;
            sigma1 = calculate_w(chosen_vertex, set1);
            sigma2 = calculate_w(chosen_vertex, set2);
            // for (int u : set1) sigma1 += adj_matrix[chosen_vertex][u];
            // for (int u : set2) sigma2 += adj_matrix[chosen_vertex][u];
    
            if (sigma1 >= sigma2)
                set2.push_back(chosen_vertex);
            else
                set1.push_back(chosen_vertex);
        }
    
        // Calculate cut value
        int cut_value = 0;
        for (int u : set1) {
            for (int v : set2) {
                cut_value += adj_matrix[u][v];
            }
        }
        // cout << "semi_greedy_heuristic: " << cut_value << endl;


    
        return make_pair(set1, set2);
    }

    
    // local search heuristic for Max cut

    pair<vector<int>, vector<int>> local_search_heuristic(vector<int> &set1,vector<int> &set2)
    {
        while (true)
        {
            int max_delta = 0 ;
            bool isSet1 = false;
            int element = -1;
            for(auto v : set1){
                int sigma_current = calculate_w(v, set1);
                int sigma_opposite = calculate_w(v, set2);
                int delta =sigma_current - sigma_opposite;
                if (delta > max_delta)
                {
                    max_delta = delta;
                    isSet1 = true;
                    element = v;
                }


            }
            for(auto v : set2){
                int sigma_current = calculate_w(v, set2);
                int sigma_opposite = calculate_w(v, set1);
                int delta =sigma_current - sigma_opposite;
                if (delta > max_delta)
                {
                    max_delta = delta;
                    isSet1 = false;
                    element = v;
                }


            
           }
            if (max_delta <= 0)
            {
                break;
            }
            else
            {
                if (isSet1)
                {
                    set1.erase(remove(set1.begin(), set1.end(), element), set1.end());
                    set2.push_back(element);
                }
                else
                {
                    set2.erase(remove(set2.begin(), set2.end(), element), set2.end());
                    set1.push_back(element);
                }
            }


        

    }


        return make_pair(set1, set2);
    
}


double Local_search_for_csv (int maxIterations, double alpha) {
    vector<int> set1, set2;
    double total_cut_weight = 0;

    for (int i = 0; i < maxIterations; i++) {
        pair<vector<int>, vector<int>> sets = semi_greedy_heuristic(alpha);
        set1 = sets.first;
        set2 = sets.second;

        pair<vector<int>, vector<int>> local_sets = local_search_heuristic(set1, set2);
        set1 = local_sets.first;
        set2 = local_sets.second;

        int cut = calculate_cut_weight(set1, set2);
         total_cut_weight += cut;
    }

    return total_cut_weight / maxIterations;

}




pair<vector<int>, vector<int>> GRASP(int MaxIterations, double alpha) {
    vector<int> best_set1, best_set2;
    int best_cut = -1;

    for (int i = 0; i < MaxIterations; i++) {
        // --- Construction Phase ---
        vector<int> set1, set2;

        pair<vector<int>, vector<int>> sets = semi_greedy_heuristic(alpha);
        set1 = sets.first;
        set2 = sets.second;
        // set1 = semi_greedy_heuristic(alpha).first; // modifies set1 and set2
        // set2 = semi_greedy_heuristic(alpha).second;
      
        // --- Local Search Phase ---
        pair<vector<int>, vector<int>> local_sets = local_search_heuristic(set1, set2);
        set1 = local_sets.first;
        set2 = local_sets.second;

        // --- Update best solution ---
        int cut = calculate_cut_weight(set1, set2);
        if (cut > best_cut) {
            best_cut = cut;
            best_set1 = set1;
            best_set2 = set2;
        }
    }

    

    return {best_set1, best_set2};
}



   

};

map<string, int> known_best = {
    {"G1", 12078}, {"G2", 12084}, {"G3", 12077}, {"G11", 627}, {"G12", 621}, {"G13", 645}, {"G14", 3187}, {"G15", 3169},
    {"G16", 3172}, {"G22", 14123}, {"G23", 14129}, {"G24", 14131}, {"G32", 1560}, {"G33", 1537}, {"G34", 1541},
    {"G35", 8000}, {"G36", 7996}, {"G37", 8009}, {"G43", 7027}, {"G44", 7022}, {"G45", 7020}, {"G48", 6000},
    {"G49", 6000}, {"G50", 5988}
};

int main() {
    srand(time(0)); // seed randomness

    ofstream fout("2105106.csv"); // change to your student ID
    fout << "Name,|V| or n,|E| or m ,Simple Randomized or Randomized 1,Simple Greedy or Greedy 1,Semi-Greedy 1,LocalSearch-Iterations,LocalSearch-Average value,GRASP-Iterations,GRASP-Best Value,Known Best\n";

    for (int i = 1; i <= 54; i++) {
        string file_name = "input_graphs/g"+ to_string(i) + ".rud";
        ifstream fin(file_name);
        if (!fin) {
            cerr << "Cannot open " << file_name << endl;
            continue;
        }

        int vertices, edges;
        fin >> vertices >> edges;
        Graph g(vertices, edges);

        for (int j = 0; j < edges; j++) {
            int u, v, w;
            fin >> u >> v >> w;
            g.add_edge(u, v, w);
        }

        int greedy_val = g.greedy_heuristic();
        int randomized_val = g.randomized_heuristic(10);
        auto semi_sets = g.semi_greedy_heuristic(0.5);
        int semi_val = g.calculate_cut_weight(semi_sets.first, semi_sets.second);
   
        

        int local_iters = 1;
        int grasp_iters = 1;


        if (vertices < 1000){
            local_iters = 50;
            grasp_iters = 50;
        }
       
        else{
            local_iters = 10;
            grasp_iters = 10;
        }
        int local_avg = g.Local_search_for_csv(local_iters ,  0.5); // Returns average or best
        

        
        auto grasp_sets = g.GRASP(grasp_iters, 0.5);
        int grasp_val = g.calculate_cut_weight(grasp_sets.first, grasp_sets.second);

        string graph_id = "G" + to_string(i);
        int known = known_best.count(graph_id) ? known_best[graph_id] : 0;

        fout << graph_id << "," << vertices << "," << edges << "," << randomized_val << "," << greedy_val << ","
             << semi_val << "," << local_iters << "," << local_avg << "," << grasp_iters << "," << grasp_val << "," << known << "\n";

        fin.close();
    }

    fout.close();
    return 0;
}