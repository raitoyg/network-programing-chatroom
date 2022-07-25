#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#define MAX_LEN 256
#define PORT 1488
using namespace std;

struct Client {
    int id;
    string name;
    int socket;
    thread th;
};

struct Room {
    int id;
    string name;
};


vector <Client> clients;
vector <Room> rooms;

vector <string> commands{
    "/send",
    "/listUsers",
    "/exit"
};

//ANSI Escape code colors
string defaultColor = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
mutex coutMutex, clientMutex;

int setName(int id, char name[]);

void sharedPrint(string str, bool endLine);

void sendMsg(string message, int sender_id);

void sendColorCode(int num, int sender_id);

void sendPm(string message, int sender_id);

void sendPmColorCode(int num, int sender_id);

void endConnection(int id);

void handleClient(int clientSocket, int id);

//Utilities Functions
Client getClient(int id);
Client getClient(string id);

vector<string> explode( const string &delimiter, const string &str);

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
        clients.push_back({seed, string("Anon"), clientSocket, (move(t))});
    }
}



vector<string> explode( const string &delimiter, const string &str)
{
    vector<string> arr;

    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng==0)
        return arr;//no change

    int i=0;
    int k=0;
    while( i<strleng )
    {
        int j=0;
        while (i+j<strleng && j<delleng && str[i+j]==delimiter[j])
            j++;
        if (j==delleng)//found delimiter
        {
            arr.push_back(  str.substr(k, i-k) );
            i+=delleng;
            k=i;
        }
        else
        {
            i++;
        }
    }
    arr.push_back(  str.substr(k, i-k) );
    return arr;
}

// Set name of client
int setName(int id, char name[]) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].name == string(name)) {
            return -1;
        }
    }
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == id) {
            if (strlen(name) != 0) {
                clients[i].name = string(name);
            } else clients[i].name = "Anon";
        }
    }
    return 0;
}

//Get client object from vector using id
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
//Get client object using string
Client getClient(string name) {
    Client tmp;
    for (int i = 0; i < clients.size(); i++) {
        if (string(clients[i].name) == name) {
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
    char tmp[MAX_LEN];
    strcpy(tmp, message.c_str());
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id != sender_id) {
            send(clients[i].socket, tmp, sizeof(tmp), 0);
        }
    }
}

// Broadcast a number to all clients except the sender
void sendColorCode(int num, int sender_id) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id != sender_id) {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}

//Broadcast msg to specific client
void sendPm(string message, int sender_id) {
    char tmp[MAX_LEN];
    strcpy(tmp, message.c_str());
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == sender_id) {
            send(clients[i].socket, tmp, sizeof(tmp), 0);
        }
    }
}

void sendPmColorCode(int num, int sender_id) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == sender_id) {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}

//Get online users
void getOnlineClients(int receiverId) {
    string tmp="Online users:\n";
    for (int j = 0; j < clients.size(); j++) {
        tmp+=clients[j].name+" : "+to_string(clients[j].id)+"\n";
    }
    sendPm(tmp.c_str(),receiverId);
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
    if (setName(id, name)==-1){
        cout<<"Name taken";
    }

    client = getClient(id);
    // Welcome msg
    string welcome_message = client.name + string(" has joined");
    sendMsg("#NULL", id);
    sendColorCode(id, id);
    sendMsg(welcome_message, id);
    sharedPrint(colors[0] + welcome_message + defaultColor);

    while (1) {
        int bytes_received = recv(clientSocket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;

        //Ekusplosion
        vector<string> strVector = explode("|", str);

        if (strVector[0]==commands[2]) {
            // Display leaving message
            string message = client.name + string(" has left");
            sendMsg("#NULL", id);
            sendColorCode(id, id);
            sendMsg(message, id);
            sharedPrint(colors[0] + message + defaultColor);
            endConnection(id);
            return;
        }

        if (strVector[0]==commands[1]) {
            // Send response list of online users on server
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            getOnlineClients(id);
        }

        if (strVector[0]==commands[0]) {
            //Command format /send|UserID|Message
            Client sender = getClient(id);
            //Handle Error
            Client receiver = getClient(stoi(strVector[1]));

            sendPm(sender.name,receiver.id);
            //Set the color here
            sendPmColorCode(3, receiver.id);
            sendPm(strVector[2],receiver.id);
        } else {
            sendMsg(client.name, id);
            sendColorCode(id, id);
            sendMsg(string(str), id);
            sharedPrint(colors[0] + name + " : " + defaultColor + str);
        }
    }
}
