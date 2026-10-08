#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdint>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

using namespace std;

// Цвета интерфейса
const COLORREF EZHIK_BG      = RGB(24, 26, 32);
const COLORREF EZHIK_PANEL   = RGB(32, 35, 43);
const COLORREF EZHIK_TEXT    = RGB(240, 240, 245);
const COLORREF EZHIK_GREEN   = RGB(70, 190, 125);
const COLORREF EZHIK_INPUT   = RGB(42, 45, 54);

const int ID_CHAT_LIST = 101;
const int ID_NEW_CHAT  = 102;
const int ID_DEL_CHAT  = 103;
const int ID_MESSAGES  = 104;
const int ID_INPUT     = 105;
const int ID_SEND      = 106;

HWND mainWindow;
HWND chatList;
HWND messagesBox;
HWND inputBox;
HWND titleLabel;

HFONT appFont;
HFONT titleFont;

HBRUSH backgroundBrush;
HBRUSH panelBrush;
HBRUSH inputBrush;

struct Message {
    wstring author;
    wstring text;
};

struct Chat {
    wstring title;
    vector<Message> messages;
};

vector<Chat> chats;
int currentChat = -1;

// --------------------------------------------------
// ПРАВИЛЬНАЯ ОБРАБОТКА РУССКИХ БУКВ
// --------------------------------------------------

wstring lowerText(wstring text) {
    for (wchar_t& c : text) {
        if (c >= L'A' && c <= L'Z') {
            c = c - L'A' + L'a';
        }
        else if (c >= L'А' && c <= L'Я') {
            c = c - L'А' + L'а';
        }
        else if (c == L'Ё') {
            c = L'ё';
        }
    }

    return text;
}

bool contains(const wstring& text, const wstring& part) {
    return text.find(part) != wstring::npos;
}

// --------------------------------------------------
// СОХРАНЕНИЕ ЧАТОВ
// --------------------------------------------------

void writeString(ofstream& file, const wstring& text) {
    uint32_t length = static_cast<uint32_t>(text.size());

    file.write(
        reinterpret_cast<const char*>(&length),
        sizeof(length)
    );

    if (length > 0) {
        file.write(
            reinterpret_cast<const char*>(text.data()),
            length * sizeof(wchar_t)
        );
    }
}

bool readString(ifstream& file, wstring& text) {
    uint32_t length = 0;

    if (!file.read(
        reinterpret_cast<char*>(&length),
        sizeof(length)
    )) {
        return false;
    }

    // Защита от повреждённых файлов
    if (length > 100000) {
        return false;
    }

    text.resize(length);

    if (length > 0) {
        if (!file.read(
            reinterpret_cast<char*>(&text[0]),
            length * sizeof(wchar_t)
        )) {
            return false;
        }
    }

    return true;
}

void saveChats() {
    ofstream file("ezhik_chats.dat", ios::binary);

    if (!file) {
        return;
    }

    uint32_t count = static_cast<uint32_t>(chats.size());

    file.write(
        reinterpret_cast<const char*>(&count),
        sizeof(count)
    );

    for (const Chat& chat : chats) {
        writeString(file, chat.title);

        uint32_t messageCount =
            static_cast<uint32_t>(chat.messages.size());

        file.write(
            reinterpret_cast<const char*>(&messageCount),
            sizeof(messageCount)
        );

        for (const Message& message : chat.messages) {
            writeString(file, message.author);
            writeString(file, message.text);
        }
    }
}

void loadChats() {
    ifstream file("ezhik_chats.dat", ios::binary);

    if (!file) {
        return;
    }

    uint32_t count = 0;

    if (!file.read(
        reinterpret_cast<char*>(&count),
        sizeof(count)
    )) {
        return;
    }

    if (count > 1000) {
        return;
    }

    vector<Chat> loadedChats;

    for (uint32_t i = 0; i < count; i++) {
        Chat chat;

        if (!readString(file, chat.title)) {
            return;
        }

        uint32_t messageCount = 0;

        if (!file.read(
            reinterpret_cast<char*>(&messageCount),
            sizeof(messageCount)
        )) {
            return;
        }

        if (messageCount > 10000) {
            return;
        }

        for (uint32_t j = 0; j < messageCount; j++) {
            Message message;

            if (!readString(file, message.author)) {
                return;
            }

            if (!readString(file, message.text)) {
                return;
            }

            chat.messages.push_back(message);
        }

        loadedChats.push_back(chat);
    }

    chats = loadedChats;
}

// --------------------------------------------------
// ОТВЕТЫ ЁЖИКА
// --------------------------------------------------

wstring aiAnswer(const wstring& originalText) {
    wstring text = lowerText(originalText);

    if (text.empty()) {
        return L"Напиши сообщение, и я постараюсь ответить!";
    }

    if (contains(text, L"привет") ||
        contains(text, L"здравствуй") ||
        contains(text, L"хай") ||
        contains(text, L"hello") ||
        contains(text, L"hi")) {

        return L"Привет! Я Ёжик AI. Чем могу помочь?";
    }

    if (contains(text, L"как дела") ||
        contains(text, L"как ты")) {

        return L"Всё хорошо! Спасибо, что спросил. А у тебя как дела?";
    }

    if (contains(text, L"как тебя зовут") ||
        contains(text, L"кто ты")) {

        return L"Я Ёжик AI — небольшой чат-помощник.";
    }

    if (contains(text, L"спасибо")) {
        return L"Пожалуйста! Рад помочь.";
    }

    if (contains(text, L"пока")) {
        return L"Пока! Возвращайся, когда понадоблюсь.";
    }

    if (contains(text, L"godot") ||
        contains(text, L"гадот")) {

        return L"Godot — игровой движок. На нём можно создавать "
               L"2D- и 3D-игры. Для начала изучи сцены, узлы "
               L"и основы GDScript.";
    }

    if (contains(text, L"c++") ||
        contains(text, L"си++") ||
        contains(text, L"программирован")) {

        return L"C++ — язык программирования. Начни с переменных, "
               L"условий if, циклов и функций.";
    }

    if (contains(text, L"компьютер") ||
        contains(text, L"windows")) {

        return L"Могу помочь с вопросами о компьютерах и Windows. "
               L"Напиши, что именно хочешь узнать.";
    }

    if (contains(text, L"что такое") ||
        contains(text, L"объясни") ||
        contains(text, L"расскажи") ||
        contains(text, L"почему") ||
        contains(text, L"как сделать")) {

        return L"Я пока умею отвечать только на некоторые вопросы. "
               L"Попробуй сформулировать вопрос по-другому.";
    }

    if (contains(text, L"кто создал") ||
        contains(text, L"кто тебя сделал")) {

        return L"Меня создали как учебный проект на C++. "
               L"Пока мои ответы основаны на простых правилах.";
    }

    return L"Я прочитал твоё сообщение, но пока не знаю, "
           L"как правильно ответить. Я ещё простой помощник, "
           L"а не полноценная нейросеть.";
}

// --------------------------------------------------
// ОБНОВЛЕНИЕ ОКНА СООБЩЕНИЙ
// --------------------------------------------------

void updateChatList() {
    if (!chatList) {
        return;
    }

    SendMessageW(chatList, LB_RESETCONTENT, 0, 0);

    for (const Chat& chat : chats) {
        SendMessageW(
            chatList,
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(chat.title.c_str())
        );
    }

    if (currentChat >= 0 &&
        currentChat < static_cast<int>(chats.size())) {

        SendMessageW(
            chatList,
            LB_SETCURSEL,
            currentChat,
            0
        );
    }
}

void updateChatView() {
    if (!messagesBox) {
        return;
    }

    wstring output;

    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        output =
            L"Добро пожаловать в Ёжик AI!\r\n\r\n"
            L"Создай чат и напиши первое сообщение.\r\n"
            L"Я пока простой помощник с заранее заданными ответами.";
    }
    else {
        const Chat& chat = chats[currentChat];

        for (const Message& message : chat.messages) {
            output += message.author;
            output += L":\r\n";
            output += message.text;
            output += L"\r\n\r\n";
        }

        if (chat.messages.empty()) {
            output =
                L"Это новый чат!\r\n\r\n"
                L"Напиши сообщение внизу окна.";
        }
    }

    SetWindowTextW(messagesBox, output.c_str());

    SendMessageW(
        messagesBox,
        EM_SETSEL,
        static_cast<WPARAM>(-1),
        static_cast<LPARAM>(-1)
    );

    SendMessageW(
        messagesBox,
        EM_SCROLLCARET,
        0,
        0
    );
}

// --------------------------------------------------
// СОЗДАНИЕ И УДАЛЕНИЕ ЧАТОВ
// --------------------------------------------------

void newChat() {
    Chat chat;

    chat.title = L"Новый чат";

    chats.push_back(chat);

    currentChat = static_cast<int>(chats.size()) - 1;

    updateChatList();
    updateChatView();
    saveChats();

    SetFocus(inputBox);
}

void deleteChat() {
    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {
        return;
    }

    chats.erase(chats.begin() + currentChat);

    if (chats.empty()) {
        currentChat = -1;
    }
    else if (currentChat >= static_cast<int>(chats.size())) {
        currentChat = static_cast<int>(chats.size()) - 1;
    }

    updateChatList();
    updateChatView();
    saveChats();
}

// --------------------------------------------------
// ОТПРАВКА СООБЩЕНИЯ
// --------------------------------------------------

void sendMessage() {
    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        newChat();
    }

    int length = GetWindowTextLengthW(inputBox);

    if (length <= 0) {
        return;
    }

    vector<wchar_t> buffer(length + 1, L'\0');

    GetWindowTextW(
        inputBox,
        buffer.data(),
        length + 1
    );

    wstring text(buffer.data());

    if (text.empty()) {
        return;
    }

    Chat& chat = chats[currentChat];

    if (chat.messages.empty()) {
        chat.title = text;

        if (chat.title.size() > 25) {
            chat.title = chat.title.substr(0, 25) + L"...";
        }
    }

    chat.messages.push_back({L"Ты", text});

    wstring answer = aiAnswer(text);

    chat.messages.push_back({L"Ёжик AI", answer});

    SetWindowTextW(inputBox, L"");

    updateChatList();
    updateChatView();

    saveChats();

    SetFocus(inputBox);
}

// --------------------------------------------------
// ИЗМЕНЕНИЕ РАЗМЕРА ЭЛЕМЕНТОВ
// --------------------------------------------------

void layoutInterface(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    const int sidebarWidth = 220;
    const int margin = 14;
    const int inputHeight = 38;
    const int buttonWidth = 90;

    MoveWindow(
        titleLabel,
        margin,
        12,
        sidebarWidth - margin * 2,
        35,
        TRUE
    );

    MoveWindow(
        chatList,
        10,
        55,
        sidebarWidth - 20,
        max(50, height - 145),
        TRUE
    );

    MoveWindow(
        GetDlgItem(hwnd, ID_NEW_CHAT),
        10,
        height - 80,
        (sidebarWidth - 25) / 2,
        35,
        TRUE
    );

    MoveWindow(
        GetDlgItem(hwnd, ID_DEL_CHAT),
        15 + (sidebarWidth - 25) / 2,
        height - 80,
        (sidebarWidth - 25) / 2,
        35,
        TRUE
    );

    MoveWindow(
        messagesBox,
        sidebarWidth + margin,
        15,
        max(100, width - sidebarWidth - margin * 2),
        max(100, height - inputHeight - 45),
        TRUE
    );

    MoveWindow(
        inputBox,
        sidebarWidth + margin,
        height - inputHeight - 12,
        max(100, width - sidebarWidth - margin * 3 - buttonWidth),
        inputHeight,
        TRUE
    );

    MoveWindow(
        GetDlgItem(hwnd, ID_SEND),
        width - buttonWidth - margin,
        height - inputHeight - 12,
        buttonWidth,
        inputHeight,
        TRUE
    );
}

// --------------------------------------------------
// СОЗДАНИЕ ИНТЕРФЕЙСА
// --------------------------------------------------

HWND createControl(
    HWND parent,
    const wchar_t* className,
    const wchar_t* text,
    DWORD style,
    int id
) {
    HWND control = CreateWindowExW(
        0,
        className,
        text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 100, 30,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (control && appFont) {
        SendMessageW(
            control,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(appFont),
            TRUE
        );
    }

    return control;
}

void createInterface(HWND hwnd) {
    titleLabel = createControl(
        hwnd,
        L"STATIC",
        L"Ёжик AI",
        SS_LEFT,
        110
    );

    SendMessageW(
        titleLabel,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(titleFont),
        TRUE
    );

    chatList = createControl(
        hwnd,
        L"LISTBOX",
        L"",
        LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
        ID_CHAT_LIST
    );

    messagesBox = createControl(
        hwnd,
        L"EDIT",
        L"",
        ES_MULTILINE |
        ES_AUTOVSCROLL |
        ES_READONLY |
        WS_VSCROLL,
        ID_MESSAGES
    );

    inputBox = createControl(
        hwnd,
        L"EDIT",
        L"",
        ES_AUTOHSCROLL,
        ID_INPUT
    );

    createControl(
        hwnd,
        L"BUTTON",
        L"+ Чат",
        BS_PUSHBUTTON,
        ID_NEW_CHAT
    );

    createControl(
        hwnd,
        L"BUTTON",
        L"Удалить",
        BS_PUSHBUTTON,
        ID_DEL_CHAT
    );

    createControl(
        hwnd,
        L"BUTTON",
        L"Отправить",
        BS_DEFPUSHBUTTON,
        ID_SEND
    );

    updateChatList();
    updateChatView();

    layoutInterface(hwnd);
}

// --------------------------------------------------
// ОБРАБОТКА СОБЫТИЙ ОКНА
// --------------------------------------------------

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (msg) {
    case WM_CREATE:
        createInterface(hwnd);
        return 0;

    case WM_SIZE:
        layoutInterface(hwnd);
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notification = HIWORD(wParam);

        if (id == ID_SEND && notification == BN_CLICKED) {
            sendMessage();
            return 0;
        }

        if (id == ID_NEW_CHAT && notification == BN_CLICKED) {
            newChat();
            return 0;
        }

        if (id == ID_DEL_CHAT && notification == BN_CLICKED) {
            deleteChat();
            return 0;
        }

        if (id == ID_CHAT_LIST && notification == LBN_SELCHANGE) {
            int selected = static_cast<int>(
                SendMessageW(chatList, LB_GETCURSEL, 0, 0)
            );

            if (selected >= 0 &&
                selected < static_cast<int>(chats.size())) {

                currentChat = selected;
                updateChatView();
            }

            return 0;
        }

        if (id == ID_INPUT && notification == EN_MAXTEXT) {
            return 0;
        }

        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);

        if (control == titleLabel) {
            SetTextColor(dc, EZHIK_GREEN);
            SetBkColor(dc, EZHIK_PANEL);
            return reinterpret_cast<LRESULT>(panelBrush);
        }

        if (control == inputBox || control == messagesBox) {
            SetTextColor(dc, EZHIK_TEXT);
            SetBkColor(dc, EZHIK_INPUT);
            return reinterpret_cast<LRESULT>(inputBrush);
        }

        SetTextColor(dc, EZHIK_TEXT);
        SetBkColor(dc, EZHIK_PANEL);

        return reinterpret_cast<LRESULT>(panelBrush);
    }

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wParam);

        RECT rect;
        GetClientRect(hwnd, &rect);

        FillRect(dc, &rect, backgroundBrush);

        RECT sidebar = {
            0,
            0,
            220,
            rect.bottom
        };

        FillRect(dc, &sidebar, panelBrush);

        return 1;
    }

    case WM_DESTROY:
        saveChats();

        if (appFont) DeleteObject(appFont);
        if (titleFont) DeleteObject(titleFont);

        if (backgroundBrush) DeleteObject(backgroundBrush);
        if (panelBrush) DeleteObject(panelBrush);
        if (inputBrush) DeleteObject(inputBrush);

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// --------------------------------------------------
// ЗАПУСК ПРОГРАММЫ
// --------------------------------------------------

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand
) {
    backgroundBrush = CreateSolidBrush(EZHIK_BG);
    panelBrush = CreateSolidBrush(EZHIK_PANEL);
    inputBrush = CreateSolidBrush(EZHIK_INPUT);

    appFont = CreateFontW(
        18, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    titleFont = CreateFontW(
        25, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    loadChats();

    if (chats.empty()) {
        Chat firstChat;
        firstChat.title = L"Первый чат";
        chats.push_back(firstChat);
    }

    currentChat = 0;

    const wchar_t CLASS_NAME[] = L"EzhikAIWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = backgroundBrush;

    if (!RegisterClassW(&wc)) {
        MessageBoxW(
            nullptr,
            L"Не удалось зарегистрировать окно.",
            L"Ошибка",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    mainWindow = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Ёжик AI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        650,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!mainWindow) {
        MessageBoxW(
            nullptr,
            L"Не удалось создать окно программы.",
            L"Ошибка",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
