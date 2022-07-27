#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>



#define MAX_LEN 256
#define PORT 1488
using namespace std;

//Todo: Split into multiple files if there is time.

struct Client {
    int id;
    int roomId;
    string name;
    int socket;
    thread th;
};

struct Room {
    int id;
    string name;
};


vector <Client> clients;
vector <Room> rooms{
        {0,"Global"}
};

vector <string> commands{
    "/send",
    "/listUsers",
    "/exit",
    "/createRoom",
    "/listRooms",
    "/join",
    "/currentRoom",
    "/help"

}
;
vector <string> guide{
        "No0. /send|userID|message\n",
        "No1. /listUsers\n",
        "No2. /exit\n",
        "No3. /createRoom|RoomID|RoomName\n",
        "No4. /listRooms\n",
        "No5. /join|roomID\n",
        "No6. /currentRoom\n",
        "No7. /help\n"

};

//ANSI Escape code colors
string defaultColor = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
mutex coutMutex, clientMutex;

//Commandlines and messages functions

void sharedPrint(string str, bool endLine);


//General chat
void sendMsg(string message, int senderId,int roomId);

void sendColorCode(int num, int senderId, int roomId);

void sendPm(string message, int senderId);


void sendPmColorCode(int num, int senderId);

void getOnlineClients(int receiverId);

void listRooms(int receiverId);


//Server functions
int setName(int id, char name[]);

void endConnection(int id);

void handleClient(int clientSocket, int id);

void createRoom(int roomId, string roomName);



//Utilities Functions
Client getClient(int id);
Client getClient(string id);
Room getRoom(int id);

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
        clients.push_back({seed,0, string("Anon"), clientSocket, (move(t))});
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
//Todo: No need to check if name already exist. Use id to connect, not user name

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
void sendMsg(string message, int senderId,int roomId) {
    char tmp[MAX_LEN];
    strcpy(tmp, message.c_str());
    for (int i = 0; i < clients.size(); i++) {
        cout<<"ClientID: "<<clients[i].id<<" ClientRoom:"<<clients[i].roomId<<" Roomid:"<<roomId<<"\n";
        if (clients[i].id != senderId&&clients[i].roomId==roomId) {
            send(clients[i].socket, tmp, sizeof(tmp), 0);
        }
    }
}

// Broadcast a number to all clients except the sender
void sendColorCode(int num, int senderId,int roomId) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id != senderId&&clients[i].roomId==roomId) {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}

//Broadcast msg to specific client
void sendPm(string message, int senderId) {
    char tmp[MAX_LEN];
    strcpy(tmp, message.c_str());
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == senderId) {
            send(clients[i].socket, tmp, sizeof(tmp), 0);
        }
    }
}

void sendPmColorCode(int num, int senderId) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].id == senderId) {
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

//List rooms
void listRooms(int receiverId) {
    string tmp="Chatrooms :\n";
    for (int j = 0; j < rooms.size(); j++) {
        tmp+=rooms[j].name+" : "+to_string(rooms[j].id)+"\n";
    }
    sendPm(tmp.c_str(),receiverId);
}

void sendHelp(int receiverId) {
    string tmp="/help Menu :\n";
    for (int j = 0; j < guide.size(); j++) {
        tmp+=guide[j];

    }
    sendPm(tmp.c_str(),receiverId);
}

void currentRoom(int receiverId, int roomId){
    string tmp="";
    stringstream ss;
    ss<<roomId;
    ss>>tmp;
    string name = rooms[roomId].name;
    string msg = "You are now in room: " + name +" ~ID: " +tmp+"\n";
    sendPm(msg,receiverId);

}

//Get Room
Room getRoom(int id){
    Room tmp;
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i].id == id) {
            tmp.id = clients[i].id;
            tmp.name = clients[i].name;
            return tmp;
        }
    }
    tmp.id = -1;
    return tmp;
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

void createRoom(int roomId,string roomName){
    rooms.push_back({roomId, roomName});

}



void handleClient(int clientSocket, int id) {
    char name[MAX_LEN], str[MAX_LEN];
    Client client;
    recv(clientSocket, name, sizeof(name), 0);
    setName(id, name);
    client = getClient(id);
    client.roomId = 0;
    // Welcome msg
    string welcome_message = client.name + string(" has joined");
    sendMsg("#NULL", id,client.roomId);
    sendColorCode(id, id,client.roomId);
    sendMsg(welcome_message, id,client.roomId);
    sharedPrint(colors[0] + welcome_message + defaultColor);

    while (1) {
        int bytes_received = recv(clientSocket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;

        //Ekusplosion
        //Todo: handle if input an incorrect command or string ie: /join/Nibber (/join|Userid), check if there are missing fields
        vector<string> strVector = explode("|", str);

        bool flag= false;
        //Exit from server
        if (strVector[0]==commands[2]) {
            // Display leaving message
            flag = true;
            string message = client.name + string(" has left");
            sendMsg("#NULL", id,client.roomId);
            sendColorCode(id, id,client.roomId);
            sendMsg(message, id,client.roomId);
            sharedPrint(colors[0] + message + defaultColor);
            endConnection(id);
            return;
        }

        //Get online users, use to get userId
        if (strVector[0]==commands[1]) {
            // Send response list of online users on server
            flag = true;
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            getOnlineClients(id);
            continue;
        }

        //Todo: add a support function (/help) to list all commands

        //get chatRooms, use to get chatroom id
        if (strVector[0]==commands[4]){
            // Send response list of online users on server
            flag = true;
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            listRooms(id);
            continue;
        }

        //Send Private msg
        if (strVector[0]==commands[0]) {
            //Command format: /send|UserID|Message
            flag = true;
            Client sender = getClient(id);
            //Handle Error
            Client receiver = getClient(stoi(strVector[1]));

            sendPm(sender.name,receiver.id);
            //Set the color here
            sendPmColorCode(3, receiver.id);
            sendPm(strVector[2],receiver.id);
            continue;
        }

        //Create chatroom
        if (strVector[0]==commands[3]) {
            //Command format: /createRoom|RoomID|RoomName
            flag = true;
            createRoom(stoi(strVector[1]),strVector[2]);
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            currentRoom(id,client.roomId);
            continue;
        }

        //Join chatroom
        if (strVector[0]==commands[5]){
            //Command format: /join|RoomID
            flag = true;
            //Todo: Handle if room not found, and add welcome message to room~~(Room will be created if not exist yet, not bug, feature)
            for (int i = 0; i < clients.size(); i++) {
                if (clients[i].id == id) {
                    clients[i].roomId=stoi(strVector[1]);
                }
            }

            //Change the local client id
            client.roomId = stoi(strVector[1]);
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
                currentRoom(id,client.roomId);
            continue;

        }

        //Get Current Room ID
        if (strVector[0]==commands[6]){
            // Send response list of online users on server
            flag = true;
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            currentRoom(id,client.roomId);
            continue;
        }


        if (strVector[0]==commands[7]){
            // Send response list of online users on server
            flag = true;
            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            sendHelp(id);
            continue;

        }
        string mpt = strVector[0];
        char mp = '/';
        if (flag == false)
            if (mpt[0]==mp){

            sendPm("#NULL",id);
            //Set the color here
            sendPmColorCode(3, id);
            sendPm("Wrong command!\n", id);

        }

        //Send normal messages
        sendMsg(client.name, id,client.roomId);
        sendColorCode(id, id,client.roomId);
        sendMsg(string(str), id,client.roomId);
        sharedPrint(colors[0] + name + " : " + defaultColor + str);


    }
}
