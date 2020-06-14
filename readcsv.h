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