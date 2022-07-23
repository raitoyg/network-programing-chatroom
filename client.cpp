#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LEN 256
#define SERVER_PORT 1488
using namespace std;

bool exit_flag = false;
thread threadSend, threadRecieve;
int clientSocket;
string defaultColor = "\033[0m";

//ANSI Escape code colors
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

void catchExitKey(int signal);

void eraseText(int cnt);

void sendMsg(int clientSocket);

void recieveMsg(int clientSocket);

int main() {
    //SOCK_STREAM = TCP(reliable, connection oriented)
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket: ");
        exit(-1);
    }

    //Set up connection
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(SERVER_PORT); // Port no. of server
    client.sin_addr.s_addr = INADDR_ANY;
    bzero(&client.sin_zero, 0);

    //Catch error =0 equal success, -1 error, and start connection
    if ((connect(clientSocket, (struct sockaddr *) &client, sizeof(struct sockaddr_in))) == -1) {
        perror("connect: ");
        exit(-1);
    }

    //Catching the Ctrl+C exit thing
    signal(SIGINT, catchExitKey);


    char name[MAX_LEN];
    cout << "Enter your name : ";
    cin.getline(name, MAX_LEN);
    send(clientSocket, name, sizeof(name), 0);

    cout << colors[1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << defaultColor;

    thread t1(sendMsg, clientSocket);
    thread t2(recieveMsg, clientSocket);

    threadSend = move(t1);
    threadRecieve = move(t2);

    if (threadSend.joinable())
        threadSend.join();
    if (threadRecieve.joinable())
        threadRecieve.join();

    return 0;
}

// Handler for Ctr C
void catchExitKey(int signal) {
    //Send exit key to server
    char str[MAX_LEN] = "exit()";
    send(clientSocket, str, sizeof(str), 0);
    exit_flag = true;
    threadSend.detach();
    threadRecieve.detach();
    close(clientSocket);
    exit(signal);
}

// Erase text from terminal
void eraseText(int cnt) {
    //ASCII for backspace
    char back_space = 8;
    for (int i = 0; i < cnt; i++) {
        cout << back_space;
    }
}

// Send message to everyone
void sendMsg(int clientSocket) {
    while (1) {
        cout << colors[1] << "You : " << defaultColor;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);

        send(clientSocket, str, sizeof(str), 0);
        //if user type exit() then exit
        if (strcmp(str, "exit()") == 0) {
            exit_flag = true;
            //Detach thread and close socket
            threadRecieve.detach();
            close(clientSocket);
            return;
        }
    }
}

// Receive message
void recieveMsg(int clientSocket) {
    while (1) {
        if (exit_flag)
            return;
        char name[MAX_LEN], str[MAX_LEN];
        int color_code;
        int bytes_received = recv(clientSocket, name, sizeof(name), 0);
        if (bytes_received <= 0)
            continue;

        //Recieving color codes of other users
        recv(clientSocket, &color_code, sizeof(color_code), 0);
        recv(clientSocket, str, sizeof(str), 0);

        //Deleting the "You :", leaving behind only the name of the other user
        eraseText(6);

        if (strcmp(name, "#NULL") != 0)
            cout << colors[color_code] << name << " : " << defaultColor << str << endl;
        else
            cout << colors[color_code] << str << endl;
        cout << colors[1] << "You : " << defaultColor;
        fflush(stdout);
    }
}
