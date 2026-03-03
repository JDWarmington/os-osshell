#include <climits>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

bool fileExecutableExists(std::string file_path);
void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);
void forkC(const std::string& executable_path, const std::vector<std::string>& command_list);
int countHistoryLines(const std::string& filename);

int main (int argc, char **argv)
{
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Create list to store history
    std::vector<std::string> history;

    // Create variables for storing command user types
    std::string user_command;               // to store command user types in
    std::vector<std::string> command_list;  // to store `user_command` split into its variour parameters

    //array is created and used in forkC() function
    //char **command_list_exec;               // to store `command_list` converted to an array of character arrays

    //if history exists, read into vector
    std::string file = "osshell_history.txt";
    if(std::filesystem::exists(file))
    {
        std::ifstream input_file(file);
        if(input_file.is_open())
        {
            while(std::getline(input_file, user_command))
            {
                history.push_back(user_command);
            }
        }
        else
        {
            std::cerr << "Error: Unable to open file" << std::endl;
        }
    }

    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

    while(true)
    {
        int total_history_lines = countHistoryLines(file);

        bool found = false;
        std::cout << "osshell> ";

        if (!std::getline(std::cin, user_command))
        {
            break; // EOF or error
        }

        if (user_command.empty())
        {
            continue;
        }

        // Creates the tokenized command list from the user command
        splitString(user_command, ' ', command_list);

        // Checks for empty command
        if (command_list.empty())
        {
            continue;
        }


        if (command_list[0] == "history")
        {
          if (command_list.size() > 1 && command_list[1] == "clear") {
            std::ofstream clearFile(file, std::ios::trunc);
            clearFile.close();
            history.clear();
          } else if (command_list.size() > 1) {
            // history with integer case
            try {
              int n = std::stoi(command_list[1]);
               if (n >= 1 && n <= 128)
              {
                std::ifstream inputFile(file);
                std::string line;
                int num = 1;
                while (std::getline(inputFile, line)) {
                  if (num >= total_history_lines - n + 1) {
                    std::cout << num << ": " << line << std::endl;
                  }
                  num++;
                }
               } else {
                std::cout << "Error, history expects integer > 0 (or 'clear')" << std::endl;
              }
            } catch (const std::exception &) {
              std::cout << user_command << ": Error command not found" << std::endl;
            }
          } else {
            std::ifstream inputFile(file);

            std::string line;
            int num = 1;
            while (std::getline(inputFile, line)) {
              std::cout << num << ": " << line << std::endl;
              num++;
            }
          }
          found = true;
          continue;
        }

        // Follows command list, if the first command is exit, save history and break to quit the program
        if (command_list[0] == "exit")
        {
            std::ofstream output_file(file);
            for(std::string_view line: history)
            {
                output_file << line << std::endl;
            }
            break;
        }

        //filesystem::path covers both ./ and ../ cases
        if(command_list[0][0] == '.')
        {
            std::filesystem::path wd = std::filesystem::current_path();
            wd /= std::filesystem::path(command_list[0]);
            wd = wd.lexically_normal();
            if(fileExecutableExists(wd.string()))
            {
                forkC(wd.string(), command_list);
            }
            else
            {
                std::cout << user_command << ": Error command not found" << std::endl;
            }
            continue;
        }

        if(command_list[0][0] == '/')
        {
            if(fileExecutableExists(command_list[0]))
            {
                forkC(command_list[0],command_list);
            }
            else
            {
                std::cout << user_command << ": Error command not found" << std::endl;
            }
            continue;
        }

        std::string executable_path;

        for (const std::string& dir : os_path_list)
        {
            std::string candidate = dir;
            if (!candidate.empty() && candidate.back() != '/')
            {
                candidate += '/';
            }
            candidate += command_list[0];

            if (fileExecutableExists(candidate))
            {
                executable_path = candidate;
                found = true;
                break;
            }
        }

        history.push_back(user_command);
        std::ofstream outFile(file, std::ios::app);
        if (outFile.is_open()) {
          outFile << user_command << std::endl;
          outFile.close();
        }

        if (history.size() > 128) {
          history.erase(history.begin());
          std::ofstream outFile(file);
          for (const auto& cmd : history)
          {
              outFile << cmd << std::endl;
          }
        }

        if (!found)
        {
            std::cout << user_command << ": Error command not found" << std::endl;
            continue;
        }


        forkC(executable_path, command_list);

    }

    // Repeat:
    //  Print prompt for user input: "osshell> " (no newline)
    //  Get user input for next command
    //  If command is `exit` exit loop / quit program
    //  If command is `history` print previous N commands
    //  For all other commands, check if an executable by that name is in one of the PATH directories
    //   If yes, execute it
    //   If no, print error statement: "<command_name>: Error command not found" (do include newline)



    /************************************************************************************
     *   Example code - remove in actual program                                        *
     ************************************************************************************/

    // Shows how to loop over the directories in the PATH environment variable
    /*int i;
    for (i = 0; i < os_path_list.size(); i++)
    {
        printf("PATH[%2d]: %s\n", i, os_path_list[i].c_str());
    }
    printf("------\n");

    // Shows how to split a command and prepare for the execv() function
    std::string example_command = "ls -lh";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");

    // Second example command - reuse the `command_list` and `command_list_exec` variables
    example_command = "echo \"Hello world\" I am alive!";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");
    */

    /************************************************************************************
     *   End example code                                                               *
     ************************************************************************************/

    return 0;
}

/*
   file_path: path to a file
   RETURN: true/false - whether or not that file exists and is executable
*/
bool fileExecutableExists(std::string file_path)
{
    bool exists = true;
    struct stat path_stat;
    if (access(file_path.c_str(), X_OK) != 0)
    {
        exists = false;
    }
     else if (stat(file_path.c_str(), &path_stat) != 0)
    {
        exists = false;
    } else if (S_ISDIR(path_stat.st_mode))
    {
        exists = false;
    }

    return exists;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
    delete[] array;
}


void forkC(const std::string& executable_path, const std::vector<std::string>& command_list)
{
    std::vector<std::string> command_list_copy = command_list;;

    char **command_list_exec = nullptr;

    vectorOfStringsToArrayOfCharArrays(command_list_copy, &command_list_exec);

    pid_t pid = fork();

    if (pid == 0)
    {
        execv(executable_path.c_str(), command_list_exec);

        freeArrayOfCharArrays(command_list_exec, command_list.size());
        _exit(1);
    }
    else if (pid > 0)
    {
        int status = 0;
        waitpid(pid, &status, 0);
        freeArrayOfCharArrays(command_list_exec, command_list.size());
    }
    else
    {
        freeArrayOfCharArrays(command_list_exec, command_list.size());
    }
}

int countHistoryLines(const std::string& filename)
{
    std::ifstream file(filename);
    int count = 0;
    std::string line;
    while (std::getline(file, line))
    {
        count++;
    }
    return count;
}
