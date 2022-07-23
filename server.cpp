#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define MAX_LEN 256
#define PORT 1488
using namespace std;

struct Client {
    int id;
    string name;
    int socket;
    thread th;
};


vector <Client> clients;
//ANSI Escape code colors
string defaultColor = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
mutex coutMutex, clientMutex;

void setName(int id, char name[]);

void sharedPrint(string str, bool endLine);

void sendMsg(string message, int sender_id);

void sendMsg(int num, int sender_id);

void endConnection(int id);

void handleClient(int clientSocket, int id);

Client getClient(int id);

int main() {
    int serverSocket;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);

    //Error Handling
    if ((bind(serverSocket, (struct sockaddr *) &server, sizeof(struct sockaddr_in))) == -1) {
        perror("bind error: ");
        exit(-1);
    }

    if ((listen(serverSocket, 8)) == -1) {
        perror("listen error: ");
        exit(-1);
    }

    struct sockaddr_in client;
    int clientSocket;
    unsigned int len = sizeof(sockaddr_in);

    cout << colors[1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << defaultColor;

    while (1) {
        if ((clientSocket = accept(serverSocket, (struct sockaddr *) &client, &len)) == -1) {
            perror("accept error: ");
            exit(-1);
        }
        //Upon a new client connected start a new thread change seed
        seed++;
        thread t(handleClient, clientSocket, seed);

        //Mutex lock here for Linux synchronization, ensuring two or more concurrent threads not executing the same chunk of code
        lock_guard <mutex> guard(clientMutex);
        clients.push_back({seed, string("Anonymous"), clientSocket, (move(t))});
    }
}

// Set name of client
void setName(int id, char name[]) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == id) {
            if (strlen(name) != 0) {
                clients[i].name = string(name);
            } else clients[i].name = "Anon";
        }
    }
}

//Get client object from vector
Client getClient(int id) {
    Client tmp;
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == id) {
            tmp.id = clients[i].id;
            tmp.name = clients[i].name;
            return tmp;
        }
    }
    return tmp;
}

// Synching the cout thing
void sharedPrint(string str, bool endLine = true) {
    lock_guard <mutex> guard(coutMutex);
    cout << str;
    if (endLine)
        cout << endl;
}

// Send msgs to all except the sender
void sendMsg(string message, int sender_id) {
    char temp[MAX_LEN];
    strcpy(temp, message.c_str());
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id != sender_id) {
            send(clients[i].socket, temp, sizeof(temp), 0);
        }
    }
}

// Broadcast a number to all clients except the sender
void sendMsg(int num, int sender_id) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id != sender_id) {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}

void endConnection(int id) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == id) {
            lock_guard <mutex> guard(clientMutex);
            clients[i].th.detach();
            clients.erase(clients.begin() + i);
            close(clients[i].socket);
            break;
        }
    }
}

void handleClient(int clientSocket, int id) {
    char name[MAX_LEN], str[MAX_LEN];
    Client client;
    recv(clientSocket, name, sizeof(name), 0);
    setName(id, name);

    client = getClient(id);
    // Welcome msg
    string welcome_message = client.name + string(" has joined");
    sendMsg("#NULL", id);
    sendMsg(id, id);
    sendMsg(welcome_message, id);
    sharedPrint(colors[0] + welcome_message + defaultColor);

    while (1) {
        int bytes_received = recv(clientSocket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;
        if (strcmp(str, "exit()") == 0) {
            // Display leaving message
            string message = client.name  + string(" has left");
            sendMsg("#NULL", id);
            sendMsg(id, id);
            sendMsg(message, id);
            sharedPrint(colors[0] + message + defaultColor);
            endConnection(id);
            return;
        }
        sendMsg(client.name, id);
        sendMsg(id, id);
        sendMsg(string(str), id);
        sharedPrint(colors[0] + name + " : " + defaultColor + str);
    }
}
