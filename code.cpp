#include <bits/stdc++.h>
using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    int wrong_before = 0;
    int solved_time = 0;
    bool solved = false;
    int submissions_after_freeze = 0;
    bool frozen = false;
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    vector<int> solve_times;
    int solved_count = 0;
    int penalty = 0;
    int rank = 0;
};

int num_problems = 0;
int duration = 0;
bool competition_started = false;
bool frozen = false;
int freeze_time = 0;
map<string, Team> teams;
vector<string> team_order;
bool flushed = false;

bool compare_teams(const string& a, const string& b) {
    const Team& ta = teams[a];
    const Team& tb = teams[b];
    if (ta.solved_count != tb.solved_count)
        return ta.solved_count > tb.solved_count;
    if (ta.penalty != tb.penalty)
        return ta.penalty < tb.penalty;
    const vector<int>& sta = ta.solve_times;
    const vector<int>& stb = tb.solve_times;
    for (size_t i = 0; i < max(sta.size(), stb.size()); i++) {
        int ta_time = (i < sta.size()) ? sta[i] : 0;
        int tb_time = (i < stb.size()) ? stb[i] : 0;
        if (ta_time != tb_time)
            return ta_time < tb_time;
    }
    return a < b;
}

void calculate_rankings() {
    vector<string> sorted_teams = team_order;
    sort(sorted_teams.begin(), sorted_teams.end(), compare_teams);
    for (size_t i = 0; i < sorted_teams.size(); i++) {
        teams[sorted_teams[i]].rank = i + 1;
    }
}

void calculate_rankings_lexicographic() {
    vector<string> sorted_teams = team_order;
    sort(sorted_teams.begin(), sorted_teams.end());
    for (size_t i = 0; i < sorted_teams.size(); i++) {
        teams[sorted_teams[i]].rank = i + 1;
    }
}

void print_scoreboard() {
    vector<string> sorted_teams = team_order;
    if (flushed) {
        sort(sorted_teams.begin(), sorted_teams.end(), compare_teams);
    } else {
        sort(sorted_teams.begin(), sorted_teams.end());
    }
    for (const string& name : sorted_teams) {
        const Team& t = teams[name];
        cout << name << " " << t.rank << " " << t.solved_count << " " << t.penalty;
        for (char p = 'A'; p < 'A' + num_problems; p++) {
            auto it = t.problems.find(p);
            if (it == t.problems.end()) {
                cout << " .";
            } else {
                const ProblemStatus& ps = it->second;
                if (ps.frozen) {
                    if (ps.wrong_before == 0)
                        cout << " 0/" << ps.submissions_after_freeze;
                    else
                        cout << " -" << ps.wrong_before << "/" << ps.submissions_after_freeze;
                } else if (ps.solved) {
                    if (ps.wrong_before == 0)
                        cout << " +";
                    else
                        cout << " +" << ps.wrong_before;
                } else {
                    if (ps.wrong_before == 0)
                        cout << " .";
                    else
                        cout << " -" << ps.wrong_before;
                }
            }
        }
        cout << "\n";
    }
}

void do_freeze() {
    frozen = true;
    for (auto& [name, team] : teams) {
        for (auto& [p, ps] : team.problems) {
            if (!ps.solved) {
                ps.frozen = true;
                ps.submissions_after_freeze = 0;
            }
        }
    }
}

void unfreeze_problem(const string& team_name, char problem) {
    Team& t = teams[team_name];
    ProblemStatus& ps = t.problems[problem];
    ps.frozen = false;

    if (ps.solved) return;

    int total_wrong = ps.wrong_before;
    bool accepted = false;
    int accept_time = 0;

    for (const auto& sub : t.submissions) {
        if (sub.problem == string(1, problem)) {
            if (sub.status == "Accepted") {
                accepted = true;
                accept_time = sub.time;
                break;
            } else {
                total_wrong++;
            }
        }
    }

    if (accepted) {
        ps.solved = true;
        ps.solved_time = accept_time;
        t.solved_count++;
        t.penalty += 20 * ps.wrong_before + accept_time;
        t.solve_times.push_back(accept_time);
        sort(t.solve_times.rbegin(), t.solve_times.rend());
    }
}

void do_scroll() {
    cout << "[Info]Scroll scoreboard.\n";
    calculate_rankings();
    print_scoreboard();

    while (true) {
        vector<string> sorted_teams = team_order;
        sort(sorted_teams.begin(), sorted_teams.end(), compare_teams);

        string target_team;
        char target_problem = 'Z' + 1;

        for (const string& name : sorted_teams) {
            const Team& t = teams[name];
            for (char p = 'A'; p < 'A' + num_problems; p++) {
                auto it = t.problems.find(p);
                if (it != t.problems.end() && it->second.frozen && it->second.submissions_after_freeze > 0) {
                    target_team = name;
                    target_problem = p;
                    break;
                }
            }
            if (!target_team.empty()) break;
        }

        if (target_team.empty()) break;

        int old_rank = teams[target_team].rank;
        unfreeze_problem(target_team, target_problem);
        calculate_rankings();
        int new_rank = teams[target_team].rank;

        if (new_rank < old_rank) {
            vector<string> sorted_after = team_order;
            sort(sorted_after.begin(), sorted_after.end(), compare_teams);
            string displaced_team = sorted_after[old_rank - 1];
            cout << target_team << " " << displaced_team << " "
                 << teams[target_team].solved_count << " " << teams[target_team].penalty << "\n";
        }
    }

    frozen = false;
    for (auto& [name, team] : teams) {
        for (auto& [p, ps] : team.problems) {
            ps.frozen = false;
            ps.submissions_after_freeze = 0;
        }
    }

    print_scoreboard();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            if (competition_started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams.count(team_name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                teams[team_name].name = team_name;
                team_order.push_back(team_name);
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string d, p;
            iss >> d >> duration >> p >> num_problems;
            if (competition_started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                competition_started = true;
                calculate_rankings_lexicographic();
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string problem_name, by, with, at;
            string team_name, status;
            int time;
            iss >> problem_name >> by >> team_name >> with >> status >> at >> time;
            char problem = problem_name[0];

            Team& t = teams[team_name];
            ProblemStatus& ps = t.problems[problem];

            t.submissions.push_back({problem_name, status, time});

            if (frozen && !ps.solved) {
                ps.submissions_after_freeze++;
            } else if (!ps.solved) {
                if (status == "Accepted") {
                    ps.solved = true;
                    ps.solved_time = time;
                    t.solved_count++;
                    t.penalty += 20 * ps.wrong_before + time;
                    t.solve_times.push_back(time);
                    sort(t.solve_times.rbegin(), t.solve_times.rend());
                } else {
                    ps.wrong_before++;
                }
            }
        } else if (cmd == "FLUSH") {
            calculate_rankings();
            flushed = true;
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                do_freeze();
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                do_scroll();
            }
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            if (!teams.count(team_name)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                cout << "[" << team_name << "] NOW AT RANKING " << teams[team_name].rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, problem_part, status_part;
            string problem_name = "ALL", status = "ALL";
            iss >> team_name >> where >> problem_part >> status_part;

            size_t p_pos = problem_part.find('=');
            if (p_pos != string::npos) problem_name = problem_part.substr(p_pos + 1);

            size_t s_pos = status_part.find('=');
            if (s_pos != string::npos) status = status_part.substr(s_pos + 1);

            if (!teams.count(team_name)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                const Team& t = teams[team_name];
                Submission result = {"", "", -1};
                for (const auto& sub : t.submissions) {
                    bool match_problem = (problem_name == "ALL" || sub.problem == problem_name);
                    bool match_status = (status == "ALL" || sub.status == status);
                    if (match_problem && match_status) {
                        result = sub;
                    }
                }
                if (result.time == -1) {
                    cout << "Cannot find any submission.\n";
                } else {
                    cout << "[" << team_name << "] " << result.problem << " " << result.status << " " << result.time << "\n";
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }

    return 0;
}
