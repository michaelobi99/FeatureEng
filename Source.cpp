#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <regex>
#include <deque>
#include "DataFrame.h"

namespace fs = std::filesystem;

class Date {
private:
    int day{};
    int month{};
    int year{};
    std::string date_str{};

    bool is_leap(int y) const {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    std::tuple<int, int, int, int> split_date(const std::string& date_str) {
        const std::regex datePattern("(\\d{2})\\.(\\d{2})\\.(\\d{4}).");
        std::smatch matches;

        if (std::regex_match(date_str, matches, datePattern)) {
            try {
                int day_val = std::stoi(matches[1].str());
                int month_val = std::stoi(matches[2].str());
                int year_val = std::stoi(matches[3].str());

                return std::make_tuple(day_val, month_val, year_val, 0);
            }
            catch (const std::exception& e) {
                return std::make_tuple(0, 0, 0, 1);
            }
        }
        return std::make_tuple(0, 0, 0, 1);
    }

    long long to_day_number() const {
        long long y = year - 1;

        long long day_count = y * 365LL;
        day_count += y / 4 - y / 100 + y / 400;

        static const int DAYS_IN_MONTH[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

        for (int m = 1; m < month; ++m)
            day_count += DAYS_IN_MONTH[m - 1];

        if (month > 2 && is_leap(year))
            day_count += 1;

        day_count += day;
        return day_count;
    }

    bool operator> (const Date& other) const {
        if (year != other.year) return year > other.year;
        if (month != other.month) return month > other.month;
        return day > other.day;
    }

public:
    Date() {
        day = month = year = 0;
        date_str = "";
    }

    void set_date(const std::string& date_str_in) {
        this->date_str = date_str_in;
        auto [d, m, y, err] = split_date(date_str_in);

        if (err == 0) {
            this->day = d;
            this->month = m;
            this->year = y;
        }
        else {
            this->day = this->month = this->year = 0;
            std::cout << "Error: Invalid date format or structure provided: " << date_str_in << std::endl;
        }
    }

    int operator-(const Date& other) const {
        if (!(*this > other)) return 0;
        long long days1 = this->to_day_number();
        long long days2 = other.to_day_number();
        return static_cast<int>(days1 - days2);
    }
};

void menu() {
	std::cout << "\nUSAGE: BB_Feature_Engineering [filename.csv]\n";
}


int count = 0;

void copy_file(const std::string& source_filename, const std::string& destination_filename) {
    count += 1;
    std::fstream file1{ source_filename, std::ios::in };
    if (!file1.is_open()) {
        std::cout << "ERROR: Couldn't open source file\n";
        exit(1);
    }
    std::fstream file2{ destination_filename, std::ios::app };
    if (!file2.is_open()) {
        std::cout << "ERROR: Couldn't open destination file\n";
        exit(1);
    }

    std::string line;
    std::getline(file1, line);
    if (count == 1) {
        file2 << line << "\n";
    }
    while (std::getline(file1, line)) {
        file2 << line << "\n";
    }
}



void modify_dates(const std::string& filename, const std::string& modified_filename) {
	std::fstream file{ filename, std::ios::in };
	std::fstream file2{ modified_filename, std::ios::out };
	std::string line;
	std::getline(file, line); //read header
	file2 << line<<"\n";
	while (std::getline(file, line)) {
		size_t pos = line.find_first_of(',');
		std::string date = line.substr(0, pos);
		size_t date_pos = date.find_first_of('.');
		int day = std::stoi(date.substr(0, date_pos));
		date.erase(0, date_pos+1);
		date_pos = date.find_first_of('.');
		int month = std::stoi(date.substr(0, date_pos));
		std::string fn = fs::path{ filename }.filename().string();
		fn = fn.substr(0, fn.find_first_of('.'));
		size_t fn_pos = fn.find_first_of('-');
		int year{0};
		if (month > 8) year = std::stoi(fn.substr(0, fn_pos));
		else year = std::stoi(fn.substr(fn_pos+1));
		std::ostringstream stream;
		if (day < 10) 
			stream << "0" << day << ".";
		else 
			stream << day << ".";
		if (month < 10) 
			stream << "0" << month << "." << year << ".,";
		else 
			stream << month << "." << year << ".,";
		std::string modified_date = stream.str();
		line.erase(0, pos+1);
		line = modified_date + line;
		file2 << line<<"\n";
	}
	file.close();
	file2.close();
}

void calculate_and_insert_rest_days(const std::string& filename1, const std::string& filename2) {
    std::vector<std::string> spreadsheet;
    spreadsheet.reserve(1500);

	//key -> BB teams, Value -> last match date
	std::fstream file{ filename1, std::ios::in };
	std::fstream file2{ filename2, std::ios::out };
    if (!file.is_open() || !file2.is_open()) {
        std::cerr << "Error opening files!" << std::endl;
        return;
    }

    std::unordered_map<std::string, std::string> team_dates;
    std::string team1{}, team2{}, match_date{};
    int home_rest_days{}, away_rest_days{};

    std::ostringstream stream;
	std::string line;
	std::getline(file, line); //read header
	line += std::string{ ",H_REST_DAYS,A_REST_DAYS" };
	file2 << line << "\n";
    while (std::getline(file, line)) {
        spreadsheet.push_back(line);
    }
    spreadsheet.shrink_to_fit();
    std::vector<std::string>::reverse_iterator r_iter = spreadsheet.rbegin();
    while (r_iter != spreadsheet.rend()){
        line = *r_iter;
		std::string temp = line;
		size_t pos = temp.find_first_of(',');
		match_date = temp.substr(0, pos);
		temp.erase(0, pos + 1);
		pos = temp.find_first_of(',');
		team1 = temp.substr(0, pos);
		temp.erase(0, pos + 1);
		pos = temp.find_first_of(',');
		team2 = temp.substr(0, pos);

        if (team_dates.find(team1) == team_dates.end()) {
            home_rest_days = 50;
            team_dates[team1] = match_date;
        }
        else {
            std::string home_prev_date = std::exchange(team_dates[team1], match_date);
            Date d1, d2;
            d1.set_date(home_prev_date);
            d2.set_date(match_date);
            home_rest_days = d2 - d1;
        }
        
        if (team_dates.find(team2) == team_dates.end()) {
            away_rest_days = 50;
            team_dates[team2] = match_date;
        }
        else {
            std::string away_prev_date = std::exchange(team_dates[team2], match_date);
            Date d1, d2;
            d1.set_date(away_prev_date);
            d2.set_date(match_date);
            away_rest_days = d2 - d1;
        }
        stream << line << "," << home_rest_days << "," << away_rest_days;
        file2 << stream.str() << "\n";
        stream.str("");
        r_iter++;
	}
    file.close();
    file2.close();
}

double mean(const std::deque<double>& scores) {
    if (scores.empty()) return 0;
    double result{ 0.0 };
    int size = scores.size();
    for (unsigned i{ 0 }; i < size; ++i) {
        result += scores[i];
    }
    result /= double(size);
    return result;
}

auto exponential_smoothing(const std::deque<double>& vec, double alpha) -> double {

    if (vec.size() == 0) return 0;
    if (vec.size() < 2) return vec[0];

    int N = vec.size();
    int k = std::min(3, N);
    double level = mean(std::deque<double>(vec.begin(), vec.begin() + k));
    
    for (size_t i = 0; i < vec.size(); ++i) {
        level = alpha * vec[i] + (1 - alpha) * level;
    }
    return level;
}

float predict_next_score(const std::deque<double>& scores, double stddev = 10.0) {
    // Calculate variance to detect stability
    float avg = mean(scores);
    float variance = 0.0;
    for (int s : scores) {
        variance += (s - avg) * (s - avg);
    }
    variance /= scores.size();
    float std_dev = std::sqrt(variance);

    float exp_smooth = exponential_smoothing(scores, 0.35);

    // If team is stable (low variance), trust mean more
    // If team is volatile (high variance), trust exponential more
    if (std_dev < stddev) {
        // Stable: 70% mean, 30% exponential
        return 0.7 * avg + 0.3 * exp_smooth;
    }
    else {
        return 0.6 * exp_smooth + 0.4 * avg;
    }
}

void calculate_and_create_lagged_averages(const std::string& filename1, const std::string& filename2, int window_size = 5) {
    std::fstream file{ filename1, std::ios::in };
    std::fstream file2{ filename2, std::ios::out };
    if (!file.is_open() || !file2.is_open()) {
        std::cerr << "Error opening files!" << std::endl;
        return;
    }

    std::vector<std::unordered_map<std::string, std::deque<double>>> lagged_features_average(27);

    std::vector<std::string> first_three_features(3);
    std::vector<std::string> last_three_features(3);

    std::ostringstream stream;
    std::string header, line;
    std::getline(file, header); //read header

    file2 << header << R"(,H_FG%_ALLOWED,A_FG%_ALLOWED,H_2FG%_ALLOWED,A_2FG%_ALLOWED,H_3FG%_ALLOWED,A_3FG%_ALLOWED,H_TOV_ALLOWED,A_TOV_ALLOWED)";
    file2 << "\n";
    while (std::getline(file, line)) {
        size_t pos;
        int i;
        for (i = 0; i < 3; ++i) {
            pos = line.find_first_of(',');
            first_three_features[i] = line.substr(0, pos);
            line.erase(0, pos + 1);
        }
        std::string feature{};
        double home_fg_pct{ 0.0 }, away_fg_pct{ 0.0 };
        double home_2fg_pct{ 0.0 }, away_2fg_pct{ 0.0 };
        double home_3fg_pct{ 0.0 }, away_3fg_pct{ 0.0 };
        double home_tov{ 0.0 }, away_tov{ 0.0 };

        for (i = 0; i < 23; ++i) {
            pos = line.find_first_of(',');
            feature = line.substr(0, pos);
            lagged_features_average[i][first_three_features[1]].push_back(std::stod(feature));
            line.erase(0, pos + 1);
            if (i == 3)
                home_fg_pct = std::stod(feature);
            else if (i == 6)
                home_2fg_pct = std::stod(feature);
            else if (i == 9)
                home_3fg_pct = std::stod(feature);
            else if (i == 18)
                home_tov = std::stod(feature);


            pos = line.find_first_of(',');
            feature = line.substr(0, pos);
            lagged_features_average[i][first_three_features[2]].push_back(std::stod(feature));
            line.erase(0, pos + 1);
            if (i == 3)
                away_fg_pct = std::stod(feature);
            else if (i == 6)
                away_2fg_pct = std::stod(feature);
            else if (i == 9)
                away_3fg_pct = std::stod(feature);
            else if (i == 18)
                away_tov = std::stod(feature);
        }

        lagged_features_average[i][first_three_features[1]].push_back(away_fg_pct);
        lagged_features_average[i++][first_three_features[2]].push_back(home_fg_pct);

        lagged_features_average[i][first_three_features[1]].push_back(away_2fg_pct);
        lagged_features_average[i++][first_three_features[2]].push_back(home_2fg_pct);

        lagged_features_average[i][first_three_features[1]].push_back(away_3fg_pct);
        lagged_features_average[i++][first_three_features[2]].push_back(home_3fg_pct);

        lagged_features_average[i][first_three_features[1]].push_back(away_tov);
        lagged_features_average[i][first_three_features[2]].push_back(home_tov);


        for (i = 0; i < 2; ++i) {
            pos = line.find_first_of(',');
            last_three_features[i] = line.substr(0, pos);
            line.erase(0, pos + 1);
        }
        last_three_features[2] = line;


        if (lagged_features_average[0][first_three_features[1]].size() > (window_size + 1)) {
            for (i = 0; i < lagged_features_average.size(); ++i)
                lagged_features_average[i][first_three_features[1]].pop_front();
        }
        if (lagged_features_average[0][first_three_features[2]].size() > (window_size + 1)) {
            for (i = 0; i < lagged_features_average.size(); ++i)
                lagged_features_average[i][first_three_features[2]].pop_front();
        }


        if (lagged_features_average[0][first_three_features[1]].size() == (window_size + 1) && lagged_features_average[0][first_three_features[2]].size() == (window_size+1)) {
            for (const auto& elem : first_three_features) {
                stream << elem << ",";
            }
            for (i = 0; i < 23; ++i) {
                std::deque<double> home_window(
                    lagged_features_average[i][first_three_features[1]].begin(),
                    std::prev(lagged_features_average[i][first_three_features[1]].end())
                );
                std::deque<double> away_window(
                    lagged_features_average[i][first_three_features[2]].begin(),
                    std::prev(lagged_features_average[i][first_three_features[2]].end())
                );

                //double avg1 = predict_next_score(home_window, 7.0); //B-League
                //double avg2 = predict_next_score(away_window, 7.0);

                double avg1 = predict_next_score(home_window); //NBA
                double avg2 = predict_next_score(away_window);

                stream << avg1 << "," << avg2 << ",";
            }
            
            stream << last_three_features[0] << "," << last_three_features[1] << "," << last_three_features[2] << ",";

            for (; i < 27; ++i) {
                std::deque<double> home_window(
                    lagged_features_average[i][first_three_features[1]].begin(),
                    std::prev(lagged_features_average[i][first_three_features[1]].end())
                );
                std::deque<double> away_window(
                    lagged_features_average[i][first_three_features[2]].begin(),
                    std::prev(lagged_features_average[i][first_three_features[2]].end())
                );

                //double avg1 = predict_next_score(home_window, 7.0); //B-League
                //double avg2 = predict_next_score(away_window, 7.0);

                double avg1 = predict_next_score(home_window); //NBA
                double avg2 = predict_next_score(away_window);

                stream << avg1 << "," << avg2;
                if (i < 26) stream << ",";
            }

            line = stream.str() + "\n";
            file2 << line;
            stream.str("");
            line.clear();
        }
        
    }
    file.close();
    file2.close();
}

std::string get_modified_filePath(const std::string& originalPath, const std::string& modifier) {
    size_t lastDotPos = originalPath.find_last_of('.');
    if (lastDotPos != std::string::npos && lastDotPos > 0) {
        std::string path_and_stem = originalPath.substr(0, lastDotPos);
        std::string extension = originalPath.substr(lastDotPos);
        return path_and_stem + modifier + extension;
    }
    else {
        std::cout << "ERROR: No extension in filename." << std::endl;
		exit(1);
    }
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		menu();
		exit(1);
	}
	std::string filename = std::string(argv[1]);

	std::string modified_filename_1 = get_modified_filePath(filename, "_modified_date");
	modify_dates(filename, modified_filename_1);
	std::string modified_filename_2 = get_modified_filePath(filename, "_reversed_plus_rest_days");
	calculate_and_insert_rest_days(modified_filename_1, modified_filename_2);

   std::vector<std::string> files{
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2019-2020_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2020-2021_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2021-2022_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2022-2023_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2023-2024_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2024-2025_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\nba\2025-2026_reversed_plus_rest_days.csv)"
    };

   /*std::vector<std::string> files{
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\b-league\2023-2024_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\b-league\2024-2025_reversed_plus_rest_days.csv)",
        R"(C:\Users\HP\source\repos\Rehoboam\Rehoboam\Data\Basketball\b-league\2025-2026_reversed_plus_rest_days.csv)"
    };*/

    std::string combinedFile{ fs::path(files[0]).parent_path().string() + R"(\combined.csv)" };
    for (const auto& filename : files) {
        copy_file(filename, combinedFile);
    }

    std::string modified_filename_3 = get_modified_filePath(combinedFile, "_lagged_averages");
    //calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 5);
    //calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 8);
    calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 10); // 19.06, 19.02, 19.03
    //calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 12); //19.11, 19.02, 19.01
    //calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 13);  //19.09, 19.05, 19.04
    //calculate_and_create_lagged_averages(combinedFile, modified_filename_3, 15); //19.11. 19.08, 19.08

    dataframe basketball_data = load_data(modified_filename_3);

    basketball_data["H_2FG_RATE"] = basketball_data["H_2FGA"] / basketball_data["H_FGA"];
    basketball_data["A_2FG_RATE"] = basketball_data["A_2FGA"] / basketball_data["A_FGA"];
    basketball_data["H_3FG_RATE"] = basketball_data["H_3FGA"] / basketball_data["H_FGA"];
    basketball_data["A_3FG_RATE"] = basketball_data["A_3FGA"] / basketball_data["A_FGA"];
    basketball_data["H_FT_RATE"] = basketball_data["H_FTA"] / basketball_data["H_FGA"];
    basketball_data["A_FT_RATE"] = basketball_data["A_FTA"] / basketball_data["A_FGA"];
    basketball_data["H_TOV_RATE"] = basketball_data["H_TOV"] / (basketball_data["H_FGA"] + basketball_data["H_FTA"] * 0.44 + basketball_data["H_TOV"]);
    basketball_data["A_TOV_RATE"] = basketball_data["A_TOV"] / (basketball_data["A_FGA"] + basketball_data["A_FTA"] * 0.44 + basketball_data["A_TOV"]);
    basketball_data["H_OREB_RATE"] = basketball_data["H_OREB"] / (basketball_data["H_OREB"] + basketball_data["A_DREB"]);
    basketball_data["A_OREB_RATE"] = basketball_data["A_OREB"] / (basketball_data["A_OREB"] + basketball_data["H_DREB"]);
    basketball_data["H_DREB_RATE"] = basketball_data["H_DREB"] / (basketball_data["H_DREB"] + basketball_data["A_OREB"]);
    basketball_data["A_DREB_RATE"] = basketball_data["A_DREB"] / (basketball_data["A_DREB"] + basketball_data["H_OREB"]);

    // Effective Field Goal Percentage(accounts for 3PT value)
    basketball_data["H_EFG%"] = (basketball_data["H_FG"] + (basketball_data["H_3FG"] * 0.5)) / basketball_data["H_FGA"];
    basketball_data["A_EFG%"] = (basketball_data["A_FG"] + (basketball_data["A_3FG"] * 0.5)) / basketball_data["A_FGA"];

    // Pace(possessions estimate)
    basketball_data["H_POSS"] = basketball_data["H_FGA"] + (basketball_data["H_FTA"] * 0.44) - basketball_data["H_OREB"] + basketball_data["H_TOV"];
    basketball_data["A_POSS"] = basketball_data["A_FGA"] + (basketball_data["A_FTA"] * 0.44) - basketball_data["A_OREB"] + basketball_data["A_TOV"];
    basketball_data["GAME_PACE"] = (basketball_data["H_POSS"] + basketball_data["A_POSS"]) * 0.5;

    // Points Per Possession
    basketball_data["H_PPP"] = basketball_data["H_SCORE"] / basketball_data["H_POSS"];
    basketball_data["A_PPP"] = basketball_data["A_SCORE"] / basketball_data["A_POSS"];

    // TS% 
    basketball_data["H_TS%"] = basketball_data["H_SCORE"] / ((basketball_data["H_FGA"] + basketball_data["H_FTA"] * 0.44) * 2.0);
    basketball_data["A_TS%"] = basketball_data["A_SCORE"] / ((basketball_data["A_FGA"] + basketball_data["A_FTA"] * 0.44) * 2.0);
    basketball_data["AVG_TS%"] = (basketball_data["H_TS%"] + basketball_data["A_TS%"]) * 0.5;


    ////Matchup stats
    basketball_data["H_OFF_VS_A_DEF"] = basketball_data["H_OFF_RATING"] - basketball_data["A_DEF_RATING"];
    basketball_data["A_OFF_VS_H_DEF"] = basketball_data["A_OFF_RATING"] - basketball_data["H_DEF_RATING"];
    basketball_data["H_FG%_VS_A_ALLOWED"] = basketball_data["H_FG%"] - basketball_data["A_FG%_ALLOWED"];
    basketball_data["A_FG%_VS_H_ALLOWED"] = basketball_data["A_FG%"] - basketball_data["H_FG%_ALLOWED"];
    basketball_data["H_2FG%_VS_A_ALLOWED"] = basketball_data["H_2FG%"] - basketball_data["A_2FG%_ALLOWED"];
    basketball_data["A_2FG%_VS_H_ALLOWED"] = basketball_data["A_2FG%"] - basketball_data["H_2FG%_ALLOWED"];
    basketball_data["H_3FG%_VS_A_ALLOWED"] = basketball_data["H_3FG%"] - basketball_data["A_3FG%_ALLOWED"];
    basketball_data["A_3FG%_VS_H_ALLOWED"] = basketball_data["A_3FG%"] - basketball_data["H_3FG%_ALLOWED"];

    // Expected score based on matchup
    basketball_data["H_EXPECTED_SCORE"] = (basketball_data["H_OFF_RATING"] + basketball_data["A_DEF_RATING"]) * 0.5;
    basketball_data["A_EXPECTED_SCORE"] = (basketball_data["A_OFF_RATING"] + basketball_data["H_DEF_RATING"]) * 0.5;
    basketball_data["EXPECTED_TOTAL"] = basketball_data["H_EXPECTED_SCORE"] + basketball_data["A_EXPECTED_SCORE"];

    // Net ratings
    basketball_data["H_NET_RATING"] = basketball_data["H_OFF_RATING"] - basketball_data["H_DEF_RATING"];
    basketball_data["A_NET_RATING"] = basketball_data["A_OFF_RATING"] - basketball_data["A_DEF_RATING"];
    basketball_data["NET_RATING_DIFF"] = basketball_data["H_NET_RATING"] - basketball_data["A_NET_RATING"];

    // More advanced metrics
    basketball_data["REST_DIFF"] = basketball_data["H_REST_DAYS"] - basketball_data["A_REST_DAYS"];
    basketball_data["PACE_X_NET_RATING"] = basketball_data["GAME_PACE"] * basketball_data["NET_RATING_DIFF"];
    basketball_data["PACE_X_EFFICIENCY"] = basketball_data["GAME_PACE"] * basketball_data["AVG_TS%"];


    std::vector<std::string> features = {
        "DATE", "HOME", "AWAY", "H_SCORE", "A_SCORE",
        "H_FGA", "A_FGA", "H_FG", "A_FG", "H_FG%", "A_FG%", "H_2FGA", "A_2FGA",	"H_2FG", "A_2FG", "H_2FG%", "A_2FG%", "H_3FGA", "A_3FGA", "H_3FG", "A_3FG", "H_3FG%",
        "A_3FG%", "H_FTA", "A_FTA", "H_FT", "A_FT", "H_FT%", "A_FT%", "H_OREB", "A_OREB", "H_DREB", "A_DREB", "H_TREB", "A_TREB", "H_AST", "A_AST", "H_BLKS", "A_BLKS",
        "H_TOV", "A_TOV", "H_STL", "A_STL", "H_P_FOULS", "A_P_FOULS", "H_OFF_RATING", "A_OFF_RATING", "H_DEF_RATING", "A_DEF_RATING", "H_REST_DAYS", "A_REST_DAYS",
        "H_FG%_ALLOWED", "A_FG%_ALLOWED", "H_2FG%_ALLOWED", "A_2FG%_ALLOWED", "H_3FG%_ALLOWED", "A_3FG%_ALLOWED", "H_TOV_ALLOWED", "A_TOV_ALLOWED",
        "H_2FG_RATE", "A_2FG_RATE", "H_3FG_RATE", "A_3FG_RATE", "H_FT_RATE", "A_FT_RATE",
        "H_TOV_RATE", "A_TOV_RATE", "H_OREB_RATE", "A_OREB_RATE", "H_DREB_RATE", "A_DREB_RATE",
        "H_EFG%", "A_EFG%",
        "H_POSS", "A_POSS", "GAME_PACE",
        "H_PPP", "A_PPP",
        "H_TS%", "A_TS%", "AVG_TS%", 
        "H_OFF_VS_A_DEF", "A_OFF_VS_H_DEF", "H_FG%_VS_A_ALLOWED", "A_FG%_VS_H_ALLOWED", "H_2FG%_VS_A_ALLOWED", "A_2FG%_VS_H_ALLOWED", 
        "H_3FG%_VS_A_ALLOWED", "A_3FG%_VS_H_ALLOWED",
        "H_EXPECTED_SCORE", "A_EXPECTED_SCORE", "EXPECTED_TOTAL",
        "H_NET_RATING", "A_NET_RATING", "NET_RATING_DIFF",
        "REST_DIFF", "PACE_X_NET_RATING", "PACE_X_EFFICIENCY",
        "TOTAL",
    };

    std::string data_file = std::string{ fs::path(files[0]).parent_path().string() + R"(\data_file.csv)" };
    save_to_csv(basketball_data, data_file, features);
    if (!fs::remove(fs::path(combinedFile)) || !fs::remove(fs::path(modified_filename_1)) || !fs::remove(fs::path(modified_filename_3))) {
        std::cerr << "Error deleting temporary files\n";
    }
}