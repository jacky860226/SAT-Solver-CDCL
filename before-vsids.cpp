#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <random>

using namespace std;

enum CState
{
    satisfied,
    unsatisfied,
    unit,
    unresolved
};

enum RetVal
{
    r_satisfied,
    r_unsatisfied,
    r_normal,
    r_restart
};

class SATSolverCDCL
{
    public:
        vector<int> literals;
        vector< vector<int> > literal_list_per_clause;
        vector<int> literal_frequency;
        vector<int> literal_polarity;
        vector<int> original_literal_frequency;
        int literal_count;
        int clause_count;
        int kappa_decision_level;
        int kappa_antecedent;
        vector<int> literal_decision_level;
        vector<int> literal_antecedent;
        int assigned_literal_count;
        bool already_unsatisfied;
        int pick_counter;
        int global_pick_counter;
        int restart_counter;
        SATSolverCDCL() {}
        void initialize();
        int CDCL();
        int unit_propagate(int);
        void assign_literal(int,int,int);
        void unassign_literal(int);
        int variable_to_literal_index(int);
        int conflict_analysis_and_backtrack(int);
        vector<int>& resolve(vector<int>&,int);
        int pick_branching_variable();
        bool all_variables_assigned();
        void show_result(int);
        void solve();
};

void SATSolverCDCL::initialize()
{
    char c; // store first character
    string s; // dummy string

    while(true)
    {
        cin>>c;
        if(c=='c') // if comment
        {
            getline(cin,s); // ignore
        }
        else // else, if would be a p
        {
            cin >> s; // this would be cnf
            break;
        }
    }
    cin >> literal_count;
    cin >> clause_count;
    assigned_literal_count = 0;
    kappa_antecedent = -1;
    kappa_decision_level = -1;
    pick_counter = 0;
    global_pick_counter = 0;
    restart_counter = 0;
    already_unsatisfied = false;
    // set the vectors to their appropriate sizes and initial values
    literals.clear();
    literals.resize(literal_count,-1);
    literal_frequency.clear();
    literal_frequency.resize(literal_count,0);
    literal_polarity.clear();
    literal_polarity.resize(literal_count,0);
    literal_list_per_clause.clear();
    literal_list_per_clause.resize(clause_count);
    literal_antecedent.clear();
    literal_antecedent.resize(literal_count,-1); // for kappa
    literal_decision_level.clear();
    literal_decision_level.resize(literal_count,-1); // we will set the decision level of kappa as the level of conflict
    
    int literal; // store the incoming literal value
    int literal_count_in_clause = 0;
    // iterate over the clauses
    for(int i = 0; i < clause_count; i++)
    {
        literal_count_in_clause = 0;
        while(true) // while the ith clause gets more literals
        {
            cin>>literal;     
            if(literal > 0) // if the variable has positive polarity
            {
                literal_list_per_clause[i].push_back(literal); // store it in the form 2n
                // increment frequency and polarity of the literal
                literal_frequency[literal-1]++; 
                literal_polarity[literal-1]++;
            }
            else if(literal < 0) // if the variable has negative polarity
            {
                literal_list_per_clause[i].push_back(literal); // store it in the form 2n+1
                // increment frequency and decrement polarity of the literal
                literal_frequency[-1-literal]++;
                literal_polarity[-1-literal]--;
            }
            else
            {
                if(literal_count_in_clause == 0)
                {
                    already_unsatisfied = true;
                }
                break; // read 0, so move to next clause
            }    
            literal_count_in_clause++;
        }       
    }
    original_literal_frequency = literal_frequency; // backup for restoring when backtracking
//    cout << "Done initializing" << endl;
}

int SATSolverCDCL::CDCL()
{
    int decision_level = 0;
    if(already_unsatisfied)
    {
        return RetVal::r_unsatisfied;
    }
    int unit_propagate_result = unit_propagate(decision_level);
    if(unit_propagate_result == RetVal::r_unsatisfied)
    {
        return unit_propagate_result;
    }
    while(!all_variables_assigned())
    {
    /*    if(global_pick_counter == 20*literal_count && restart_counter < literal_count/20)
        {
            global_pick_counter = 0;
            restart_counter++;
            return RetVal::r_restart;
        } */
        int picked_variable = pick_branching_variable();
        decision_level++;
        assign_literal(picked_variable,decision_level,-1);
        while(true)
        {
            unit_propagate_result = unit_propagate(decision_level);
            if(unit_propagate_result == RetVal::r_unsatisfied)
            {
                if(decision_level < 0)
                {
                    return unit_propagate_result;
                }
                decision_level = conflict_analysis_and_backtrack(decision_level);
            }
            else
            {
                break;
            }
        }
    }
    return RetVal::r_satisfied;    
}

int SATSolverCDCL::unit_propagate(int decision_level)
{
    bool unit_clause_found = false;
    int false_count = 0;
    int unset_count = 0;
    int literal_index;
    bool satisfied_flag = false;
    int last_unset_literal = -1;
    do 
    {
        unit_clause_found = false;
        for(int i = 0; i < literal_list_per_clause.size() && !unit_clause_found; i++)
        {
            false_count = 0;
            unset_count = 0;
            satisfied_flag = false;
            for(int j = 0; j < literal_list_per_clause[i].size(); j++)
            {
                literal_index = variable_to_literal_index(literal_list_per_clause[i][j]);
                if(literals[literal_index] == -1)
                {
                    unset_count++;
                    last_unset_literal = j;
                }
                else if((literals[literal_index] == 0 && literal_list_per_clause[i][j] > 0) || (literals[literal_index] == 1 && literal_list_per_clause[i][j] < 0))
                {
                    false_count++;
                }
                else
                {
                    satisfied_flag = true;
                    break;
                }
            }
            if(satisfied_flag)
            {
                continue;
            }           
            if(unset_count == 1)
            {
                // unit clause
                assign_literal(literal_list_per_clause[i][last_unset_literal],decision_level,i);
                unit_clause_found = true;
                break;
            }
            else if(false_count == literal_list_per_clause[i].size())
            {
                // unsatisfied clause
                kappa_antecedent = i;
                return RetVal::r_unsatisfied;
            }
        }        
    }
    while(unit_clause_found);
    kappa_antecedent = -1;
    return RetVal::r_normal;
}

void SATSolverCDCL::assign_literal(int variable, int decision_level, int antecedent)
{
    int literal = variable_to_literal_index(variable);
    int value = (variable > 0) ? 1 : 0;
    literals[literal] = value;
    literal_decision_level[literal] = decision_level;
    literal_antecedent[literal] = antecedent;
    literal_frequency[literal] = -1;
    assigned_literal_count++;
}

void SATSolverCDCL::unassign_literal(int literal_index)
{
    literals[literal_index] = -1;
    literal_decision_level[literal_index] = -1;
    literal_antecedent[literal_index] = -1;
    literal_frequency[literal_index] = original_literal_frequency[literal_index];
    assigned_literal_count--;
}

int SATSolverCDCL::variable_to_literal_index(int variable)
{
    return (variable > 0) ? variable-1 : -variable-1;    
}

int SATSolverCDCL::conflict_analysis_and_backtrack(int decision_level)
{
    vector<int> learnt_clause = literal_list_per_clause[kappa_antecedent];
    int conflict_decision_level = decision_level;
    int this_level_count = 0;
    int resolver_literal;
    int literal;
    do 
    {
        this_level_count = 0;
        for(int i = 0; i < learnt_clause.size(); i++)
        {
            literal = variable_to_literal_index(learnt_clause[i]);
            if(literal_decision_level[literal] == conflict_decision_level)
            {
                this_level_count++;
            }
            if(literal_decision_level[literal] == conflict_decision_level && literal_antecedent[literal] != -1)
            {
                resolver_literal = literal;
            }
        }
        if(this_level_count == 1)
        {
            break;
        }
        learnt_clause = resolve(learnt_clause,resolver_literal);
    }
    while(true);
    literal_list_per_clause.push_back(learnt_clause);
    for(int i = 0; i < learnt_clause.size(); i++)
    {
        int literal_index = variable_to_literal_index(learnt_clause[i]);
        int update = (learnt_clause[i] > 0) ? 1 : -1;
        literal_polarity[literal_index] += update;
        if(literal_frequency[literal_index] != -1)
        {
            literal_frequency[literal_index]++;
        }
        original_literal_frequency[literal_index]++;        
    }
    clause_count++;
    int backtracked_decision_level = -1;
    for(int i = 0; i < learnt_clause.size(); i++)
    {
        int literal_index = variable_to_literal_index(learnt_clause[i]);
        int decision_level_here = literal_decision_level[literal_index];
        if(decision_level_here != conflict_decision_level && decision_level_here > backtracked_decision_level)
        {
            backtracked_decision_level = decision_level_here;
        }
    }
    for(int i = 0; i < literals.size(); i++)
    {
        if(literal_decision_level[i] > backtracked_decision_level)
        {
            unassign_literal(i);
        }
    }
    return backtracked_decision_level;
}

vector<int>& SATSolverCDCL::resolve(vector<int> &input_clause, int literal)
{
    vector<int> second_input = literal_list_per_clause[literal_antecedent[literal]];
    input_clause.insert(input_clause.end(),second_input.begin(),second_input.end());
    for(int i = 0; i < input_clause.size(); i++)
    {
        if(input_clause[i] == literal+1 || input_clause[i] == -literal-1)
        {
            input_clause.erase(input_clause.begin()+i);
            i--;
        }
    }
    sort(input_clause.begin(),input_clause.end());
    input_clause.erase(unique(input_clause.begin(),input_clause.end()),input_clause.end());
    return input_clause;
}

random_device rd;
mt19937 rng(rd());

int SATSolverCDCL::pick_branching_variable()
{
    
    uniform_int_distribution<int> uid(1,10);
    int random_val = uid(rng);
    if(true && (random_val > 4 || assigned_literal_count < literal_count/2))
    {
        pick_counter++;
        global_pick_counter++;
        if(pick_counter == 20*literal_count)
        {
            for(int i = 0; i < literals.size(); i++)
            {
                original_literal_frequency[i] /= 2;
                if(literal_frequency[i] != -1)
                {
                    literal_frequency[i] /= 2;
                }
            }
            pick_counter = 0;
        } 
        int variable = distance(literal_frequency.begin(),max_element(literal_frequency.begin(),literal_frequency.end()));
        if(literals[variable] != -1)
        {
            cout << "ERROR : v : " << variable << " " << literal_frequency[variable] << endl;
            for(int i = 0; i < literals.size(); i++)
            {
                cout << literals[i] << " " << literal_frequency[i] << " " << literal_decision_level[i] << literal_antecedent[i] << endl;
            }
            exit(0);
        }
        if(literal_polarity[variable] >= 0)
        {
            return variable+1;
        }
        return -variable-1; 
    }
    else
    {
        uniform_int_distribution<int> uid2(0,literal_count-1);
        while(true)
        {
            int variable = uid2(rng);
            if(literal_frequency[variable] != -1)
            {
                if(literal_polarity[variable] >= 0)
                {
                    return variable+1;
                }
                return -variable-1; 
            }
        }
    }
    
}

bool SATSolverCDCL::all_variables_assigned()
{
    return literal_count == assigned_literal_count;
}

void SATSolverCDCL::show_result(int result_status)
{
    if(result_status == RetVal::r_satisfied) // if the formula is satisfiable
    {
        cout<<"SAT"<<endl;
        if(!all_variables_assigned())
        {
            cout << "NOT!" << endl;
        }
        for(int i = 0; i < literals.size(); i++)
        {
            if(i != 0)
            {
                cout<<" ";
            }
            if(literals[i] != -1)
            {
                cout<<pow(-1,(literals[i]+1))*(i+1);
            }
            else // for literals which can take either value, arbitrarily assign them to be true
            {
                cout<<(i+1);
            }
        }
    }
    else // if the formula is unsatisfiable
    {
        cout<<"UNSAT";
    }
}

void SATSolverCDCL::solve()
{
    int result_status = CDCL();
  /*  if(result_status == RetVal::r_restart)
    {
        cout << "RESTARTING : " << clause_count << endl;
        for(int i = 0; i < literals.size(); i++)
        {
            unassign_literal(i);
        }
        solve();
        return;
    }*/
    show_result(result_status);
}

int main()
{
    SATSolverCDCL solver;
    solver.initialize();
    solver.solve();
    return 0;
}
