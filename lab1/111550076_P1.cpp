/*
Student No.: 111550076
Student Name: 楊子賝
Email: zichen55.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <sstream>

using namespace std;

void sigchld_handler(int sig)
{
    int stat;
    while (waitpid(-1, &stat, WNOHANG) > 0)
    {
        // Child process terminated, do nothing
    }
}

void execute_command(vector<char *> &args, int input_fd, int output_fd)
{
    if (input_fd != -1)
    {
        if (dup2(input_fd, STDIN_FILENO) == -1)
        {
            perror("dup2 input_fd failed");
            exit(1);
        }
        close(input_fd);
    }
    if (output_fd != -1)
    {
        if (dup2(output_fd, STDOUT_FILENO) == -1)
        {
            perror("dup2 output_fd failed");
            exit(1);
        }
        close(output_fd);
    }
    execvp(args[0], args.data());
    perror("execvp failed");
    exit(1);
}

void solve(string program)
{
    vector<string> cmd;
    vector<string> cmd_r;
    bool wait_child = true;
    stringstream ss(program);
    string token;
    int type = 0;
    bool bonus = false;

    while (ss >> token)
    {
        if (token == "|" || token == ">" || token == "<")
        {
            if (token == "|")
                type = 1; // pipe
            else if (token == ">")
                type = 2; // output redirect
            else
                type = 3; // input redirect
            bonus = true;
            continue;
        }
        if (!bonus)
        {
            cmd.push_back(token);
        }
        else
        {
            cmd_r.push_back(token);
        }
    }

    if (cmd.empty())
        return;

    if (cmd.back() == "&")
    {
        wait_child = false;
        cmd.pop_back();
    }
    if (bonus && !cmd_r.empty() && cmd_r.back() == "&")
    {
        cmd_r.pop_back();
    }

    vector<char *> args, args_r;
    for (auto &arg : cmd)
    {
        args.push_back((char *)arg.c_str());
    }
    args.push_back(NULL);

    if (bonus)
    {
        
        for (auto &arg : cmd_r)
        {
            args_r.push_back((char *)arg.c_str());
        }
        args_r.push_back(NULL);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        switch (type)
        {
        case 1: // pipe
        {
            int pipefd[2];
            // pipefd[0] for read, pipefd[1] for write
            if (pipe(pipefd) == -1)
            {
                perror("pipe failed");
                exit(1);
            }

            pid_t pid_tmp = fork();
            if (pid_tmp == 0)
            {
                close(pipefd[0]);
                execute_command(args, -1, pipefd[1]); // execute left command
            }
            else if (pid_tmp < 0)
            {
                perror("fork failed");
                exit(1);
            }

            pid_tmp = fork();
            if (pid_tmp == 0)
            {
                close(pipefd[1]);
                execute_command(args_r, pipefd[0], -1); // execute right command
            }
            else if (pid_tmp < 0)
            {
                perror("fork failed");
                exit(1);
            }

            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
            break;
        }
        case 2: // output redirect
        {
            int fd = open(args_r[0], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd == -1)
            {
                perror("open failed");
                exit(1);
            }
            execute_command(args, -1, fd);
            break;
        }
        case 3: // input redirect
        {
            int fd = open(args_r[0], O_RDONLY);
            if (fd == -1)
            {
                perror("open failed");
                exit(1);
            }
            execute_command(args, fd, -1);
            break;
        }
        default: // else
        {
            execute_command(args, -1, -1);
            break;
        }
        }
    }
    else if (pid > 0)
    {
        if (wait_child)
        {
            wait(NULL);
        }
        else
        {
            waitpid(-1, NULL, WNOHANG);
        }
    }
    else
    {
        perror("fork failed");
        exit(1);
    }
}

int main()
{
    string program;
    signal(SIGCHLD, sigchld_handler);
    while (true)
    {
        cout << ">";
        if (!getline(cin, program) || program == "exit" || program == "exit &")
            break;
        solve(program);
        // system(program.c_str());
    }
    kill(0, SIGTERM);
    while (waitpid(-1, NULL, 0) > 0);
    return 0;
}
