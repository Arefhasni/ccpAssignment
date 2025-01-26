#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>

using namespace std;

// Function prototypes
bool has_substring(const string& line, const string& substring);
pair<string, ofstream> create_database();
pair<vector<pair<string, string>>, stringstream> create_table(ifstream &inFile);
void insert_into_table(const vector<pair<string, string>> columnData, const string& queryInput, ofstream& tempFile);
void write_to_terminal_and_file(const string& outputText, ofstream &outputFile);
stringstream select_all_from_table(ofstream&);
void delete_from_table(ofstream& tableCSV);

void trim_string(string& str);
string get_table_name(string currentLine);
string get_output_file_name(string line);

bool isFirstTimeInserting = true;

int main()
{
    ifstream fileInput;
    string fileInputPath = "InputFiles\\fileInput1.mdb";

    fileInput.open(fileInputPath);
    if (!fileInput.is_open())
    {
        cerr << "Unable to open input file" << endl;
        exit(-1);
    }

    ofstream fileOutput;
    string fileOutputName = "OutputFiles\\fileOutput.mdb";

    //* temp csv file <filename, filestream>
    pair<string, ofstream> tempFile;

    //* table
    vector<vector<string>> table;
    string tableName;
    tableName = "HARD CODED TEST NAME"; //* temporary name

    //* column meta data that holds column name and column type
    vector<pair<string, string>> columnMetaData;
    columnMetaData = {{"col1", "INT"}, {"col2", "TEXT"}, {"col3", "TEXT"}, {"col4", "TEXT"}};

    string line;
    string outputTexts;
    stringstream sStringPrint;

    #pragma region loop
    while (getline(fileInput, line))
    {
        outputTexts = "";
        //* prints line
        //#cout << "LINE: " << line << endl;;

        //* create table
        if (has_substring(line, "CREATE TABLE"))
        {
            //* get table name here
            tableName = get_table_name(line);

            //* get column metadata
            pair<vector<pair<string, string>>, stringstream> res = create_table(fileInput);
            columnMetaData = res.first;
            sStringPrint = move(res.second);

            //#for (pair<string, string> dataPair : columnMetaData)
            //#{cout << "FIRST PAIR >" << dataPair.first << "< SECOND PAIR >" << dataPair.second << "<" << endl;}

            outputTexts = "> " + line + sStringPrint.str();
        }
        //* create output file
        else if (has_substring(line, "CREATE"))
        {
            fileOutputName = get_output_file_name(line);
            fileOutput.open(fileOutputName);

            if (!fileOutput.is_open())
            {
                cout << "Unable to open output file" << endl;
                exit (-1);
            }
            outputTexts = "> " + line;
        }
        //* database
        else if (has_substring(line, "DATABASES;"))
        {
            tempFile = create_database();
            outputTexts = "> DATABASES;\n" + fileInputPath;
        }
        //* prints table name
        else if (has_substring(line, "TABLES;"))
        {
            outputTexts = "> " + line + "\n" + tableName;
        }
        //* insert values into table
        else if (has_substring(line, "INSERT INTO"))
        {
            outputTexts = "> " + line;
            insert_into_table(columnMetaData, line, tempFile.second);
        }
        else if (has_substring(line, "SELECT"))
        {
            outputTexts = "> " + line;
            sStringPrint.clear();
            sStringPrint << endl;
            sStringPrint = move(select_all_from_table(fileOutput));
            outputTexts = outputTexts + sStringPrint.str ();
        }
        else
        {
            outputTexts = line;
        }
        #pragma endregion
        write_to_terminal_and_file(outputTexts, fileOutput);
    }

    cout << endl;

    fileInput.close();
    fileOutput.close();
    tempFile.second.close();


    //delete tempfile
    cout << tempFile.first << endl;
    /*if (remove(tempFile.first.c_str()) != 0)
    {
        cout << "error deleting file" << endl;
    }
    else
    {
        cout << "temp file deleted successfully" << endl;
    }*/

    return 0;
}

#pragma region function defs

// finds substring from a string
bool has_substring(const string& line, const string& substring)
{
    if (line.find(substring) != string::npos)
    {
        return true; // substring found
    }
    else
    {
        return false; // substring not found
    }
}

void write_to_terminal_and_file(const string& outputText, ofstream &outputFile)
{
    // Write to terminal
    cout << outputText << endl;

    // Write to output file if it is open
    if (outputFile.is_open()) // Use the correct parameter: outputFile
    {
        outputFile << outputText << endl;
    }
    else
    {
        cerr << "Output file isn't open for writing" << endl;
    }
}

pair<string, ofstream> create_database()
{
    string tempFileName = "temp.csv";
    ofstream tempFile (tempFileName);

    //check if the temp.csv opened
    if (!tempFile.is_open())
    {
        cout << "tempfile fails open" << endl;
        exit(-1);
    }

    else
    {
        cout << "tempFile open" << endl;
    }

    //* move ofstream to tempfile
    return make_pair(tempFileName, move(tempFile));
}

pair<vector<pair<string, string>>, stringstream> create_table(ifstream& inFile)
 {
    vector<pair<string, string>> columnMetaData;
    string line;
    stringstream sstring;

    sstring << endl;
    while (getline(inFile, line))
    {
        sstring << line;
        // Stop reading if the line contains ");"
        if (line.find(");") != string::npos) {
            break;
        }
        //* don't add endline if );
        sstring << endl;

        // Skip empty lines
        if (line.empty())
        {
            continue;
        }

        // Find the position of space to separate column name and type
        size_t spacePos = line.find(' ');
        if (spacePos == string::npos) {
            // Invalid line format, skip to the next line
            continue;
        }

        // Extract column name and type
        string columnName = line.substr(0, spacePos);       // Everything before the space
        string columnType = line.substr(spacePos + 1);     // Everything after the space

        // Remove trailing spaces and commas
        while (!columnType.empty() && isspace(columnType.back()))
        {
            columnType.pop_back(); // Remove trailing spaces
        }
        if (!columnType.empty() && columnType.back() == ',')
        {
            columnType.pop_back(); // Remove trailing comma
        }

        // Add the column name and type to the metadata vector
        columnMetaData.push_back({columnName, columnType});

        // Stop adding columns if the maximum limit (10) is reached
        if (columnMetaData.size() >= 10) {
            break;
        }
    }

    return make_pair(columnMetaData, move(sstring));

}

#pragma region insert
void insert_into_table(const vector<pair<string, string>> columnData, const string& queryInput, ofstream& tempFile)
{
    regex pattern(R"(\(([^)]+)\)\s+VALUES\s+\(([^)]+)\))");
    smatch match;

    stringstream sstring;
    string queryData;

    vector<string> columnName;
    vector<string> insertValues;
    vector<size_t> columnNumber;

    //* get column name and values to insert from query
    if (regex_search(queryInput, match, pattern))
    {
        for (size_t i = 1; i < match.size(); i++)
        {
            //#cout << "MATCHES[" << i << "]: " << match[i] << endl;

            sstring.str("");
            sstring.clear();
            sstring << match[i];

            //* add to matches to vectors
            while (getline(sstring, queryData, ','))
            {
                trim_string(queryData);
                if (i == 1)
                    columnName.push_back(queryData);
                else
                    insertValues.push_back(queryData);
            }
        }
    }
    else
    {
        cout << "THERE ARE NO MATCH" << endl;
    }

    //* we add \n before inserting values into temp.csv. if it is the first time inserting
    //* then we don't add the new line
    if (isFirstTimeInserting)
        isFirstTimeInserting = false;
    else
        tempFile << endl;

    
    //* check if column name exists in table
    bool isColNameCorrect = false;
    for (auto colName: columnName)
    {
        isColNameCorrect = false;

        //* check if column name is in columndata
        for (int i = 0; i < columnData.size(); i++)
        {
            //#cout << "? " << colName << "=?" << columnData[i].first << endl;

            if (columnData[i].first == colName)
            {
                //#cout << "MATCH: " << columnData[i].first << "==" << colName << endl;
                isColNameCorrect = true;

                columnNumber.push_back(i);
                break;
            }
        }

        if (!isColNameCorrect)
        {
            cout << "ERROR 'INSERT INTO': No column named \"" << colName << "\" in table" << endl;
            exit(-1);
        }
    }
    /*
    cout << endl;
    for (auto num:columnNumber)
    {
        cout << num << endl;
    }
    cout << endl;*/

    //#cout << "max col: " << columnData.size() << endl;
    //* write to file
    for (size_t i = 0, j = 0; i < columnData.size(); i++)
    {
        //#cout << "count: " << i <<endl;
        //* insert values in the correct column
        if (find(columnNumber.begin(), columnNumber.end(), i) != columnNumber.end())
        {
            tempFile << insertValues[j];
            j++;
        }  
        else
        {
            //#cout << i << " isn't in columnNumber" << endl;
        } 
        
        //* don't write commaa after last column
        if (i != columnData.size()-1)
        {
            tempFile << ",";
        }
    }
}

#pragma insert endregion

stringstream select_all_from_table(ofstream& outFile)
{
    stringstream ss;
    ifstream tempFile("temp.csv");
    
    string s;

    ss << endl;
    // Read each line from temp.csv
    while (getline(tempFile, s))
    {
        ss << s << endl;     
        outFile << s << endl;  
    }

    tempFile.close();  
    return ss;         
}

// DELETE FUNCTION
void delete_from_table(ofstream& tableCSV)
{
    return;
}

void update_table()
{
    return;
}

string get_table_name(string currentLine)
{
    regex pattern(R"(CREATE TABLE\s+\n?([^\s(]+))");
    smatch match;

    string line = currentLine;
    stringstream sString;
    
    sString << line;
    line = sString.str();
    
    if (regex_search(line, match, pattern))
        return match[1];
    else
    {
        cout << "ERROR: No table name provided" << endl;
        exit(-1);
    }
    return "";
}

string get_output_file_name(string line)
{
    regex pattern(R"(CREATE +([^;]+))");
    smatch match;

    //* return output filename
    if (regex_search(line, match, pattern))
    {
        return match[1];
    }   
    else
    {
        cerr << "ERROR: No output file name provided" << endl;
        exit(-1);
    }
}

void trim_string(string& str)
{
    if (!str.empty())
    {
        //* remove trailing whitespaces
        str.erase(str.find_last_not_of(" \t\n\r\f\v") + 1);
        str = str.substr(str.find_first_not_of(" \t\n\r\f\v"));

        //* remove single quotes
        if (str.front() == '\'' && str.back() == '\'')
        {
            if (str.size() < 2)
                str = "";
            else
                str = str.substr(1, str.size()-2);
        }
    }
    
}

#pragma endregion
