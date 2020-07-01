#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
using namespace std;

typedef vector<vector<string>> str_mat;

void SplitString(const string& s, vector<string>& v, const string& c);
str_mat readcsv(string file_name);
bool str2bool(string str);