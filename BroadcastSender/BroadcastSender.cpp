// Broadcast.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
#include <iostream>
#include <WS2tcpip.h>
#include <vector>
#include <conio.h>
#include <string>

#pragma comment (lib, "ws2_32.lib")

// Константы
#define PORT 25565
#define BUFFER_SIZE 8192
#define KEY_ARROW_UP 72
#define KEY_ARROW_RIGHT 77
#define KEY_ARROW_DOWN 80
#define KEY_ARROW_LEFT 75
#define REFRESH 114


// Переменные
int stuctureLength;
char buffer[BUFFER_SIZE];
bool redraw = true;
short index = 0;
SOCKET sock;
sockaddr_in local_addr, other_addr;
std::string message;
int counter[100];
std::vector<sockaddr_in> ipAddreses;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
HANDLE hStdOut;
DWORD cWrittenChars;
COORD wOldSize;
WORD wOldColorAttrs;
COORD wOldPosMouse;

class Logger {
public:
    Logger() {
        hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    Logger(HANDLE hStdOut) {
        this->hStdOut = hStdOut;
    }

    void danger(std::string text) {
        SetConsoleTextAttribute(hStdOut, 4);
        std::cout << text << std::endl;
        SetConsoleTextAttribute(hStdOut, wOldColorAttrs);
    }

    void success(std::string text) {
        SetConsoleTextAttribute(hStdOut, 2);
        std::cout << text << std::endl;
        SetConsoleTextAttribute(hStdOut, wOldColorAttrs);
    }

    void info(std::string text) {
        SetConsoleTextAttribute(hStdOut, 3);
        std::cout << text << std::endl;
        SetConsoleTextAttribute(hStdOut, wOldColorAttrs);
    }

    void custom(short sColor, std::string text) {
        SetConsoleTextAttribute(hStdOut, sColor);
        std::cout << text;
    }
private:
    HANDLE hStdOut;
};

class TCP {
public:
    TCP(SOCKET sock, sockaddr_in other, int buffer_size, Logger log) {
        this->sock = sock;
        this->other = other;
        this->log = log;
        this->buffer_size = buffer_size;
    }

    int connection() {
        if (sock == INVALID_SOCKET)
        {
            log.danger("Ошибка: Не удалось создать TCP сокет");
            return -1;
        }

        if (connect(sock, (sockaddr*)&other, sizeof(other)) == SOCKET_ERROR) {
            log.danger("Ошибка: Не удалось подключиться к серверу");
            return -1;
        }
    }

    void sendmsg(std::string text) {
        send(sock, text.c_str(), text.size(), 0);
    }

    void recvmsg(char* buffer) {
        recv(sock, buffer, buffer_size, 0);
    }
private:
    SOCKET sock;
    sockaddr_in other;
    Logger log;
    int buffer_size;
};

// Методы
int initWS();
int setSocketBroadcast(SOCKET sock);
int setSocketTimeout(int time);
void printer(std::string msg);
void sendBroadcastMessage();
void drawOptions(COORD pos, Logger log);
void connectToServer(Logger log);
void waitAnswer();
void selectedAddress(short i, short p);
void hideCursor();
void showCursor();

int main()
{
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    Logger log = Logger(hStdOut);
    GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
    wOldColorAttrs = csbiInfo.wAttributes;
    COORD curPos;

    setlocale(LC_CTYPE, "rus");
    system("cls");
    COORD bottomLeft = { csbiInfo.srWindow.Left, csbiInfo.srWindow.Bottom };


    if (initWS()) {
        printer("Ошибка: Возникли проблемы при инициализации...");
        return -1;
    }
    printer("Инициализация прошла успешна!");

    // Создание сокета
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Установка широковещательного сигнала
    if (setSocketBroadcast(sock)) {
        printer("Ошибка: Возникли проблемы при установки широковещательного сигнала...");
        return -1;
    }
    printer("Установка прошла успешна!");

    stuctureLength = sizeof(sockaddr);

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = INADDR_BROADCAST;

    message = "Привет мир!";

    char choice = 'x';
    // Отправка сообщения
    //HANDLE sender = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)sendBroadcastMessage, NULL, NULL, NULL);
    // Ждём ответа

    
    int iKey = 67;
    int i = 0;
    GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
    hideCursor();

    wOldPosMouse = csbiInfo.dwCursorPosition;
    wOldSize = { csbiInfo.srWindow.Right, csbiInfo.srWindow.Bottom };
    short p = 0;
    while (iKey != 27) {
        GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
        /*if (wOldSize.X != csbiInfo.srWindow.Right || wOldSize.Y != csbiInfo.srWindow.Bottom) {
            redraw = true;
        } */
        if (redraw) drawOptions(COORD({ csbiInfo.srWindow.Left, csbiInfo.srWindow.Bottom }), log);

        if (_kbhit()) {
            iKey = _getch();
            switch (iKey)
            {
            case REFRESH:
                sendto(sock, "R", 2, 0, (sockaddr*)&local_addr, sizeof(local_addr));
                log.info("Отправил");
                waitAnswer();
                
                FillConsoleOutputCharacter(hStdOut, (TCHAR)' ', csbiInfo.srWindow.Right * 4, wOldPosMouse, &cWrittenChars);
                SetConsoleCursorPosition(hStdOut, wOldPosMouse);
                if (!ipAddreses.empty()) {
                    char ipAddress[256];
                    int i = 0;
                    for (auto const& ip : ipAddreses) {
                        inet_ntop(AF_INET, &ip.sin_addr, ipAddress, 256);
                        SetConsoleTextAttribute(hStdOut, 3);
                        std::cout << std::string(ipAddress) << " (" << counter[i] << "/10)" << std::endl;
                        SetConsoleTextAttribute(hStdOut, wOldColorAttrs);
                        i++;
                    }
                    index = 0;
                    selectedAddress(index, 0);
                }
                break;
            case KEY_ARROW_UP:
                if (index > 0) {
                    index--;
                    p = index + 1;
                    selectedAddress(index, p);
                }
                break;
            case KEY_ARROW_DOWN:
                if ((index + 1) < ipAddreses.size()) {
                    index++;
                    p = index - 1;
                    selectedAddress(index, p);
                }
                break;
            case 13:
                connectToServer(log);
                break;
            default:
                break;
            }
        }
        wOldSize = { csbiInfo.srWindow.Right, csbiInfo.srWindow.Bottom };
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}

int initWS() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int setSocketBroadcast(SOCKET sock) {
    char broadcast = '1';
    return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
}

int setSocketTimeout(int time) {
    timeval tv;
    tv.tv_sec = time;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv.tv_sec, sizeof(timeval));
}

void connectToServer(Logger log) {
    if (!ipAddreses.empty()) {
        sendto(sock, "C", sizeof(char), 0, (sockaddr*)&ipAddreses[index], sizeof(sockaddr_in));
        setSocketTimeout(10000);

        if (recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&other_addr, &stuctureLength) <= 0) {
            printer("Ошибка: Сервер не доступен");
        }
        if (std::string(buffer) == "OK") {
            SOCKET tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            TCP tcp = TCP(tcp_sock, other_addr, BUFFER_SIZE, log);

            if (tcp.connection() < 0) {
                log.danger("Ошибка соединения");
            }

            std::string temp;

            FillConsoleOutputAttribute(hStdOut, wOldColorAttrs, csbiInfo.srWindow.Right * 4, wOldPosMouse, &cWrittenChars);
            FillConsoleOutputCharacter(hStdOut, (TCHAR)' ', csbiInfo.srWindow.Right * 4, wOldPosMouse, &cWrittenChars);
            SetConsoleCursorPosition(hStdOut, wOldPosMouse);
            tcp.recvmsg(buffer);
            log.success(buffer);
            tcp.recvmsg(buffer);
            log.success(buffer);
            showCursor();
            getline(std::cin, temp);
            hideCursor();
            tcp.sendmsg(temp);
        }
    }
}

void hideCursor() {
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hStdOut, &structCursorInfo);
    structCursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &structCursorInfo);
}

void showCursor() {
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hStdOut, &structCursorInfo);
    structCursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hStdOut, &structCursorInfo);
}

void selectedAddress(short i, short p) {
    SetConsoleCursorPosition(hStdOut, wOldPosMouse);
    GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
    FillConsoleOutputAttribute(hStdOut, wOldColorAttrs, csbiInfo.srWindow.Right + 1, { 0, wOldPosMouse.Y + p }, &cWrittenChars);
    FillConsoleOutputAttribute(hStdOut, 0b11110000, csbiInfo.srWindow.Right + 1, { 0, wOldPosMouse.Y + i }, &cWrittenChars);
}

void waitAnswer() {
    ipAddreses.clear();
    setSocketTimeout(3000);
    int other_size = sizeof(other_addr);
    int count;
    while (true) {
        if (recvfrom(sock, (char*)&count, sizeof(int), 0, (sockaddr*)&other_addr, &other_size) <= 0) {
            break;
        }
        counter[ipAddreses.size()] = count;
        ipAddreses.push_back(other_addr);
    }
    setSocketBroadcast(sock);
}

void sendBroadcastMessage() {
    while (true) {
        sendto(sock, message.c_str(), message.size(), 0, (sockaddr*)&local_addr, sizeof(local_addr));
        Sleep(5000);
    }
}

void printer(std::string msg) {
    std::cout << msg << std::endl;
}

void drawOptions(COORD pos, Logger log) {
    FillConsoleOutputAttribute(hStdOut, wOldColorAttrs, wOldSize.X + 1, { 0, wOldSize.Y }, &cWrittenChars);
    GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
    SetConsoleCursorPosition(hStdOut, { csbiInfo.srWindow.Left, csbiInfo.srWindow.Bottom });
    FillConsoleOutputAttribute(hStdOut, 0b11110000, csbiInfo.srWindow.Right + 2, pos, &cWrittenChars);
    
    log.custom(0b11110000, " REFRESH (");
    log.custom(0b11110001, "R");
    log.custom(0b11110000, ") |");
    log.custom(0b11110000, "EXIT (");
    log.custom(0b11110001, "ESC");
    log.custom(0b11110000, ") ");
    
    SetConsoleTextAttribute(hStdOut, wOldColorAttrs);
    SetConsoleCursorPosition(hStdOut, csbiInfo.dwCursorPosition);
    redraw = false;
}