#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
using namespace std;

typedef vector<vector<string>> str_mat;

void SplitString(const string& s, vector<string>& v, const string& c);
str_mat readcsv(string file_name);

str_mat readcsv(string file_name) {
    str_mat mat;
    ifstream file(file_name);
    string line;
    bool end = false;
    while (!file.eof()) {
        vector<string> newRow;
        // read a line
        getline(file, line);
        SplitString(line, newRow, ",");
        if (!newRow.empty()) {
            mat.push_back(newRow);
            delete& newRow;
        }
    }
    file.close();
    return mat;
}

void SplitString(const string& s, vector<string>& v, const string& c)
{
    string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while (pos2 != string::npos)
    {
        v.push_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != s.length())
        v.push_back(s.substr(pos1));
}

int main(){
    str_mat mat = readcsv("simple_data/arrivalTime.csv");

    for (str_mat::const_iterator iter_row = mat.cbegin(); iter_row != mat.cend(); iter_row++)
    {
        for (vector<string>::const_iterator iter_col = (*iter_row).cbegin(); iter_col != (*iter_row).cend(); iter_col++)
            {
                cout << atoi((*iter_col).c_str()) << " ";
            }
            cout << "\n";
    }

    return 0;
}


// int main(){
//     std::vector<std::vector<int>> startTrainInfo;

//     ifstream arrivalStationID_file;
//     string file_name("simple_data/arrivalTime.csv");
// 	arrivalStationID_file.open(file_name);
//     string line;
//     bool end = false;
//     int row = 0;
//     int col = 0;
//     while(!arrivalStationID_file.eof()){
//         std::vector<int> newTrainInfo;
//         // read the data ended with a comma
//         for(col = 0; col < 2 - 1; col++){
//             getline(arrivalStationID_file,line,',');
//             if(line.empty()){
//                 end = true;
//                 break;
//             }
//             // cout << line << "\t";
//             newTrainInfo.push_back(atoi(line.c_str()));
//         }
//         // read the final data ended with a \n
//         // arrivalStationID_file >> input_temp;
//         getline(arrivalStationID_file,line);
//         // cout << line << "\t";
//         newTrainInfo.push_back(atoi(line.c_str()));
//         // cout << line.c_str() << "--" << line << endl;
//         // getchar();
//         if(end){break;}
//         startTrainInfo.push_back(newTrainInfo);
//         delete &newTrainInfo;
//         row++;
//     }
//     arrivalStationID_file.close();

//     for (vector<vector<int>>::const_iterator iter = startTrainInfo.cbegin(); iter != startTrainInfo.cend(); iter++)
//     {
//         for (vector<int>::const_iterator iter_col = (*iter).cbegin(); iter_col != (*iter).cend(); iter_col++)
//             {
//                 cout << (*iter_col) << " ";
//             }
//             cout << "\n";
//     }
    

//     return 0;
// }